#pragma once

#include <atomic>

// Forward-declare the RE worldspace type so the header is not polluted with
// game-engine includes.  GameStatePoller.cpp includes PCH.h for the full type.
namespace RE { class TESWorldSpace; }

namespace PipBoyRemote
{
    // Singleton that schedules a recurring task on the Fallout 4 game main thread
    // (via F4SE::GetTaskInterface()->AddTaskPermanent()) to read player state and
    // push it to WebSocketServer.
    //
    // Only one permanent task is registered for the lifetime of the plugin; it
    // checks an internal flag to decide whether to sample and broadcast this frame.
    class GameStatePoller
    {
    public:
        static GameStatePoller& GetSingleton();

        // Registers the permanent task with F4SE.  Safe to call once at plugin load.
        void Register();

        // Enables active sampling and broadcasting.
        void Start();

        // Pauses sampling (e.g. while a save game is loading).
        void Pause();

        // Resumes sampling after a pause.
        void Resume();

        // Permanently disables sampling.
        void Stop();

        // Records the FormID of the map marker the frontend most recently set as the
        // active waypoint.  Called from the WebSocket thread; thread-safe.
        void SetWaypointFormID(std::uint32_t formID) noexcept;

        // Records the FormID of the quest the frontend wants to set as the tracked quest.
        // The change is applied on the game thread on the next quest sample.
        // Called from the WebSocket thread; thread-safe.
        void SetActiveQuestFormID(std::uint32_t formID) noexcept;

    private:
        GameStatePoller()  = default;
        ~GameStatePoller() = default;

        GameStatePoller(const GameStatePoller&)            = delete;
        GameStatePoller& operator=(const GameStatePoller&) = delete;

        // The permanent task callback — executed every game frame on the game thread.
        void OnGameFrame();

        // Reads the player's current vitals and world position from the game engine.
        void SamplePlayerState();

        // Reads the player's inventory and detects changes since the last sample.
        void SampleInventory();

        // Scans all BGSLocation map markers in the current worldspace and broadcasts
        // the full list.  Only runs when the worldspace changes or a periodic rescan
        // is due (roughly every FRAMES_PER_MARKER_RESCAN frames).
        void SampleMapMarkers();

        // Scans the quest journal for running quests with displayed objectives and
        // broadcasts the result.  Runs every FRAMES_PER_QUEST_RESCAN frames or
        // immediately when a set_active_quest action is pending.
        void SampleQuests();

        std::atomic<bool>        _active{ false };
        std::atomic<bool>        _registered{ false };

        // FormID of the marker the frontend most recently designated as the waypoint.
        // Written from the uWS thread (SetWaypointFormID); read on the game thread
        // (SampleMapMarkers).  Zero means no waypoint is currently set.
        std::atomic<std::uint32_t> _waypointFormID{ 0 };

        // FormID of the quest the frontend wants to set as tracked.
        // Written from the uWS thread (SetActiveQuestFormID); consumed and zeroed on
        // the game thread (SampleQuests).  Zero means no pending change.
        std::atomic<std::uint32_t> _pendingActiveQuestFormID{ 0 };

        // Frame counter used to throttle sampling to roughly every 100 ms at 60 fps
        // (i.e. every 6 frames).
        static constexpr int     FRAMES_PER_SAMPLE = 6;
        int                      _frameCount{ 0 };

        // Inventory change detection: track the cached weight as a cheap proxy.
        // A weight change triggers a full inventory re-scan.
        float                    _lastInventoryWeight{ -1.0f };

        // Map marker rescan: fire a full scan on worldspace change and every
        // FRAMES_PER_MARKER_RESCAN frames to pick up newly-discovered locations.
        static constexpr int     FRAMES_PER_MARKER_RESCAN = 600;  // ~10 s at 60 fps
        int                      _markerRescanCounter{ 0 };
        bool                     _forceMapMarkerRescan{ false };
        RE::TESWorldSpace*       _lastWorldspace{ nullptr };

        // Quest rescan: broadcast every FRAMES_PER_QUEST_RESCAN frames so objective
        // completions and stage changes are reflected in the frontend promptly.
        static constexpr int     FRAMES_PER_QUEST_RESCAN = 120;  // ~2 s at 60 fps
        int                      _questRescanCounter{ 0 };
        bool                     _forceQuestRescan{ false };
    };
}
