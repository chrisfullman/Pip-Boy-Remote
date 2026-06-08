#pragma once

#include "Config.h"
#include "GameState.h"

#include <atomic>
#include <functional>
#include <string>
#include <thread>

// Forward-declare the uWS Loop so headers that include WebSocketServer.h do not
// need to pull in <uwebsockets/App.h>.
namespace uWS { class Loop; }

namespace PipBoyRemote
{
    // Singleton that owns the background WebSocket server thread.
    //
    // Threading contract:
    //   - Initialize() and Start() / Stop() are called from the game main thread.
    //   - Broadcast*() are safe to call from ANY thread; they post work to the
    //     uWS event loop via Loop::defer().
    //   - All uWS / game-engine mutations happen on their respective owner threads.
    class WebSocketServer
    {
    public:
        static WebSocketServer& GetSingleton();

        // Applies INI config. Must be called before Start().
        void Initialize(const Config& cfg);

        // Spawns the server thread and waits until the listen socket is bound.
        // Safe to call multiple times (no-op while already running).
        void Start();

        // Signals the event loop to exit and joins the thread.
        void Stop();

        // Serialise and broadcast a player state snapshot to all connected clients.
        // Thread-safe.
        void BroadcastStateUpdate(const PlayerState& state);

        // Serialise and broadcast an inventory snapshot to all connected clients.
        // Thread-safe.
        void BroadcastInventoryUpdate(const InventorySnapshot& snapshot);

        // Serialise and broadcast a map markers snapshot to all connected clients.
        // Thread-safe.
        void BroadcastMapMarkersUpdate(const MapMarkersSnapshot& snapshot);

        // Returns true when at least one WebSocket client is currently connected.
        [[nodiscard]] bool HasClients() const noexcept;

    private:
        WebSocketServer()  = default;
        ~WebSocketServer() = default;

        WebSocketServer(const WebSocketServer&)            = delete;
        WebSocketServer& operator=(const WebSocketServer&) = delete;
        WebSocketServer(WebSocketServer&&)                 = delete;
        WebSocketServer& operator=(WebSocketServer&&)      = delete;

        void RunEventLoop();

        Config                          _config;
        std::thread                     _thread;
        uWS::Loop*                      _loop{ nullptr };

        // Publisher function set by the uWS thread before _loopReady.
        // The acquire/release ordering of _loopReady provides the synchronisation
        // that makes _publisher visible to callers of Broadcast*() without a mutex.
        std::function<void(std::string)> _publisher;

        std::atomic<bool> _running{ false };
        std::atomic<bool> _loopReady{ false };
        std::atomic<int>  _clientCount{ 0 };
    };
}
