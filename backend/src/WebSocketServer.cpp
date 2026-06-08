#include "PCH.h"
#include "WebSocketServer.h"
#include "GameStatePoller.h"
#include "JsonMessages.h"

#include <REX/LOG.h>

// <wingdi.h> (pulled in by windows.h) defines ERROR as 0, which causes
// REX::ERROR to preprocess to REX::0 — an illegal token.  Undefine it
// after all Windows headers have been included via PCH.h.
#ifdef ERROR
#    undef ERROR
#endif

namespace PipBoyRemote
{
    // Per-connection user data stored inside each uWS WebSocket allocation.
    // Reserved for future use (rate limiting, per-client flags, etc.).
    struct PerSocketData {};

    // Publish / subscribe topic shared by all client WebSocket connections.
    static constexpr std::string_view BROADCAST_TOPIC = "game_state";

    // ──────────────────────────────────────────────────────────────────────────
    // Forward declarations for action-handling free functions (defined below).
    // They live in an anonymous namespace so they are not visible outside this TU.
    // ──────────────────────────────────────────────────────────────────────────
    namespace
    {
        using WS = uWS::WebSocket<false, true, PerSocketData>;

        void DispatchAction(WS* ws, std::string_view raw);

        void HandleSetWaypoint (WS* ws, const nlohmann::json& msg);
        void HandleFastTravel  (WS* ws, const nlohmann::json& msg);
        void HandleEquipItem   (WS* ws, const nlohmann::json& msg);
        void HandleUnequipItem (WS* ws, const nlohmann::json& msg);
        void HandleConsumeItem (WS* ws, const nlohmann::json& msg);
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Singleton
    // ──────────────────────────────────────────────────────────────────────────

    WebSocketServer& WebSocketServer::GetSingleton()
    {
        static WebSocketServer instance;
        return instance;
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Public interface
    // ──────────────────────────────────────────────────────────────────────────

    void WebSocketServer::Initialize(const Config& cfg)
    {
        _config = cfg;
    }

    void WebSocketServer::Start()
    {
        if (_running.exchange(true)) {
            return;  // already running
        }
        _loopReady.store(false, std::memory_order_relaxed);
        _thread = std::thread(&WebSocketServer::RunEventLoop, this);

        // Spin-wait (typically < 1 ms) until RunEventLoop signals readiness.
        while (!_loopReady.load(std::memory_order_acquire)) {
            std::this_thread::yield();
        }
        REX::INFO("[PipBoyRemote] WebSocket server listening on {}:{}", _config.bindIP, _config.port);
    }

    void WebSocketServer::Stop()
    {
        if (!_running.exchange(false)) {
            return;  // already stopped
        }
        if (_loop) {
            // Tear down inside the uWS thread: clear publisher first so no new
            // payloads are enqueued after the loop exits, then free the loop.
            _loop->defer([this]() {
                _publisher = nullptr;
                _loop->free();
            });
        }
        if (_thread.joinable()) {
            _thread.join();
        }
        _loop = nullptr;
        REX::INFO("[PipBoyRemote] WebSocket server stopped");
    }

    void WebSocketServer::BroadcastStateUpdate(const PlayerState& state)
    {
        if (!_running.load(std::memory_order_relaxed) || !_loop) { return; }
        auto payload = JsonMessages::BuildStateUpdate(state);
        _loop->defer([this, msg = std::move(payload)]() mutable {
            if (_publisher) { _publisher(std::move(msg)); }
        });
    }

    void WebSocketServer::BroadcastInventoryUpdate(const InventorySnapshot& snapshot)
    {
        if (!_running.load(std::memory_order_relaxed) || !_loop) { return; }
        auto payload = JsonMessages::BuildInventoryUpdate(snapshot);
        _loop->defer([this, msg = std::move(payload)]() mutable {
            if (_publisher) { _publisher(std::move(msg)); }
        });
    }

    void WebSocketServer::BroadcastMapMarkersUpdate(const MapMarkersSnapshot& snapshot)
    {
        if (!_running.load(std::memory_order_relaxed) || !_loop) { return; }
        auto payload = JsonMessages::BuildMapMarkersUpdate(snapshot);
        _loop->defer([this, msg = std::move(payload)]() mutable {
            if (_publisher) { _publisher(std::move(msg)); }
        });
    }

    bool WebSocketServer::HasClients() const noexcept
    {
        return _clientCount.load(std::memory_order_relaxed) > 0;
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Event loop (runs on _thread)
    // ──────────────────────────────────────────────────────────────────────────

    void WebSocketServer::RunEventLoop()
    {
        uWS::App app;

        // Wire up the publisher lambda before signalling readiness so the
        // acquire/release pair in Start() makes it visible to callers.
        _publisher = [&app](std::string payload) {
            app.publish(BROADCAST_TOPIC, payload, uWS::OpCode::TEXT, false);
        };

        // ── WebSocket route ────────────────────────────────────────────────
        app.ws<PerSocketData>("/*", {
            .compression      = uWS::DISABLED,
            .maxPayloadLength = 64u * 1024u,  // 64 KiB limit for incoming messages
            .idleTimeout      = 120,          // seconds before idle disconnect

            .open = [this](WS* ws) {
                ws->subscribe(BROADCAST_TOPIC);
                _clientCount.fetch_add(1, std::memory_order_relaxed);
                REX::INFO("[PipBoyRemote] Client connected ({} total)", _clientCount.load());

                // Immediately send a welcome heartbeat so the client does not wait.
                ws->send(JsonMessages::BuildHeartbeat(true), uWS::OpCode::TEXT);
            },

            .message = [](WS* ws, std::string_view message, uWS::OpCode opCode) {
                if (opCode == uWS::OpCode::TEXT) {
                    DispatchAction(ws, message);
                }
            },

            .close = [this](WS* /*ws*/, int /*code*/, std::string_view /*msg*/) {
                const int remaining = _clientCount.fetch_sub(1, std::memory_order_relaxed) - 1;
                REX::INFO("[PipBoyRemote] Client disconnected ({} remaining)", remaining);
            },
        });

        // ── Periodic heartbeat timer ───────────────────────────────────────
        // Fires every 5 seconds to confirm the server is alive when no state
        // updates are flowing (e.g. game is paused / on main menu).
        struct HeartbeatTimerData { uWS::App* app; WebSocketServer* self; };
        auto* hbData = new HeartbeatTimerData{ &app, this };

        // uWS::Loop and us_loop_t share the same memory but have no C++ inheritance
        // relationship in v20; reinterpret_cast is required (uWS uses the same cast
        // internally when calling libuSockets functions).
        auto* nativeLoop = reinterpret_cast<us_loop_t*>(uWS::Loop::get());
        auto* hbTimer    = us_create_timer(nativeLoop, 0, static_cast<unsigned int>(sizeof(HeartbeatTimerData*)));
        *reinterpret_cast<HeartbeatTimerData**>(us_timer_ext(hbTimer)) = hbData;

        us_timer_set(
            hbTimer,
            [](us_timer_t* t) {
                auto* d  = *reinterpret_cast<HeartbeatTimerData**>(us_timer_ext(t));
                const bool hasCli = d->self->HasClients();
                d->app->publish(
                    BROADCAST_TOPIC,
                    JsonMessages::BuildHeartbeat(hasCli),
                    uWS::OpCode::TEXT,
                    false
                );
            },
            5000,   // initial delay ms
            5000    // repeat interval ms
        );

        // ── Bind and listen ────────────────────────────────────────────────
        app.listen(_config.bindIP, _config.port, [this](us_listen_socket_t* listenSocket) {
            if (!listenSocket) {
                REX::ERROR("[PipBoyRemote] Failed to bind on {}:{}", _config.bindIP, _config.port);
                _running.store(false, std::memory_order_relaxed);
                // Signal Start() to stop waiting even on failure.
                _loopReady.store(true, std::memory_order_release);
                return;
            }
            _loop = uWS::Loop::get();
            // Release store: everything written above (including _publisher) is
            // now visible to threads that acquire-load _loopReady.
            _loopReady.store(true, std::memory_order_release);
        });

        app.run();  // blocks until Loop::free() is called

        us_timer_close(hbTimer);
        delete hbData;
        _publisher = nullptr;
        _loop      = nullptr;
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Action dispatch helpers (anonymous namespace, local to this TU)
    // ──────────────────────────────────────────────────────────────────────────

    namespace
    {
        // Safely posts a lambda to the game main thread via F4SE::GetTaskInterface().
        // Returns false and sends an error response if the interface is unavailable.
        template <class F>
        bool PostToGameThread(WS* ws, std::string_view action, F&& task)
        {
            const auto* ti = F4SE::GetTaskInterface();
            if (!ti) {
                ws->send(
                    JsonMessages::BuildActionResponse(action, false, "F4SE task interface unavailable"),
                    uWS::OpCode::TEXT
                );
                return false;
            }
            ti->AddTask(std::forward<F>(task));
            return true;
        }

        void DispatchAction(WS* ws, std::string_view raw)
        {
            nlohmann::json msg;
            try {
                msg = nlohmann::json::parse(raw);
            } catch (const std::exception& e) {
                REX::WARN("[PipBoyRemote] Malformed action JSON: {}", e.what());
                ws->send(JsonMessages::BuildActionResponse("unknown", false, "Invalid JSON"), uWS::OpCode::TEXT);
                return;
            }

            if (!msg.contains("action") || !msg["action"].is_string()) {
                ws->send(JsonMessages::BuildActionResponse("unknown", false, "Missing 'action' field"), uWS::OpCode::TEXT);
                return;
            }

            const std::string action = msg["action"].get<std::string>();

            if      (action == "set_waypoint") { HandleSetWaypoint (ws, msg); }
            else if (action == "fast_travel")  { HandleFastTravel  (ws, msg); }
            else if (action == "equip")        { HandleEquipItem   (ws, msg); }
            else if (action == "unequip")      { HandleUnequipItem (ws, msg); }
            else if (action == "consume")      { HandleConsumeItem (ws, msg); }
            else {
                ws->send(JsonMessages::BuildActionResponse(action, false, "Unknown action"), uWS::OpCode::TEXT);
            }
        }

        void HandleSetWaypoint(WS* ws, const nlohmann::json& msg)
        {
            if (!msg.contains("markerID") || !msg["markerID"].is_number_integer()) {
                ws->send(JsonMessages::BuildActionResponse("set_waypoint", false, "Missing 'markerID'"), uWS::OpCode::TEXT);
                return;
            }
            const auto markerFormID = msg["markerID"].get<std::uint32_t>();
            ws->send(JsonMessages::BuildActionResponse("set_waypoint", true), uWS::OpCode::TEXT);

            // Record immediately so SampleMapMarkers reflects the new waypoint on
            // the next broadcast, even before the game-thread task executes.
            GameStatePoller::GetSingleton().SetWaypointFormID(markerFormID);

            PostToGameThread(ws, "set_waypoint", [markerFormID]() {
                auto* player = RE::PlayerCharacter::GetSingleton();
                if (!player) { return; }
                auto* form = RE::TESForm::GetFormByID(markerFormID);
                if (!form)   { return; }
                auto* refr = form->As<RE::TESObjectREFR>();
                if (!refr)   { return; }
                // QueueFastTravel with allowAutoSave=false sets the compass waypoint
                // without triggering an actual fast travel.
                player->QueueFastTravel(refr->GetHandle(), false);
            });
        }

        void HandleFastTravel(WS* ws, const nlohmann::json& msg)
        {
            if (!msg.contains("markerID") || !msg["markerID"].is_number_integer()) {
                ws->send(JsonMessages::BuildActionResponse("fast_travel", false, "Missing 'markerID'"), uWS::OpCode::TEXT);
                return;
            }
            const auto markerFormID = msg["markerID"].get<std::uint32_t>();
            ws->send(JsonMessages::BuildActionResponse("fast_travel", true), uWS::OpCode::TEXT);

            PostToGameThread(ws, "fast_travel", [markerFormID]() {
                auto* player = RE::PlayerCharacter::GetSingleton();
                if (!player) { return; }
                auto* form = RE::TESForm::GetFormByID(markerFormID);
                if (!form)   { return; }
                auto* refr = form->As<RE::TESObjectREFR>();
                if (!refr)   { return; }
                player->QueueFastTravel(refr->GetHandle(), true);
            });
        }

        void HandleEquipItem(WS* ws, const nlohmann::json& msg)
        {
            if (!msg.contains("formID") || !msg["formID"].is_number_integer()) {
                ws->send(JsonMessages::BuildActionResponse("equip", false, "Missing 'formID'"), uWS::OpCode::TEXT);
                return;
            }
            const auto itemFormID = msg["formID"].get<std::uint32_t>();
            ws->send(JsonMessages::BuildActionResponse("equip", true), uWS::OpCode::TEXT);

            PostToGameThread(ws, "equip", [itemFormID]() {
                auto* player  = RE::PlayerCharacter::GetSingleton();
                auto* manager = RE::ActorEquipManager::GetSingleton();
                if (!player || !manager) { return; }
                auto* form = RE::TESForm::GetFormByID(itemFormID);
                if (!form)   { return; }
                auto* obj  = form->As<RE::TESBoundObject>();
                if (!obj)    { return; }
                const RE::BGSObjectInstance inst{ obj, nullptr };
                manager->EquipObject(player, inst, 0, 1, nullptr, false, false, true, true, false);
            });
        }

        void HandleUnequipItem(WS* ws, const nlohmann::json& msg)
        {
            if (!msg.contains("formID") || !msg["formID"].is_number_integer()) {
                ws->send(JsonMessages::BuildActionResponse("unequip", false, "Missing 'formID'"), uWS::OpCode::TEXT);
                return;
            }
            const auto itemFormID = msg["formID"].get<std::uint32_t>();
            ws->send(JsonMessages::BuildActionResponse("unequip", true), uWS::OpCode::TEXT);

            PostToGameThread(ws, "unequip", [itemFormID]() {
                auto* player  = RE::PlayerCharacter::GetSingleton();
                auto* manager = RE::ActorEquipManager::GetSingleton();
                if (!player || !manager) { return; }
                auto* form = RE::TESForm::GetFormByID(itemFormID);
                if (!form)   { return; }
                auto* obj  = form->As<RE::TESBoundObject>();
                if (!obj)    { return; }
                const RE::BGSObjectInstance inst{ obj, nullptr };
                manager->UnequipObject(player, &inst, 1, nullptr, 0, false, false, true, true, nullptr);
            });
        }

        void HandleConsumeItem(WS* ws, const nlohmann::json& msg)
        {
            if (!msg.contains("formID") || !msg["formID"].is_number_integer()) {
                ws->send(JsonMessages::BuildActionResponse("consume", false, "Missing 'formID'"), uWS::OpCode::TEXT);
                return;
            }
            const auto itemFormID = msg["formID"].get<std::uint32_t>();
            ws->send(JsonMessages::BuildActionResponse("consume", true), uWS::OpCode::TEXT);

            PostToGameThread(ws, "consume", [itemFormID]() {
                auto* player  = RE::PlayerCharacter::GetSingleton();
                auto* manager = RE::ActorEquipManager::GetSingleton();
                if (!player || !manager) { return; }
                auto* form = RE::TESForm::GetFormByID(itemFormID);
                if (!form)   { return; }
                // Aid items (food, drink, medication) are AlchemyItems in FO4.
                // Equipping an AlchemyItem via the manager causes the player to consume it.
                auto* alch = form->As<RE::AlchemyItem>();
                if (!alch)   { return; }
                const RE::BGSObjectInstance inst{ alch, nullptr };
                manager->EquipObject(player, inst, 0, 1, nullptr, false, false, true, true, false);
            });
        }
    }  // namespace (anonymous)
}
