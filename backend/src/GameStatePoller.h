#pragma once

#include <atomic>

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

        std::atomic<bool>        _active{ false };
        std::atomic<bool>        _registered{ false };

        // Frame counter used to throttle sampling to roughly every 100 ms at 60 fps
        // (i.e. every 6 frames).
        static constexpr int     FRAMES_PER_SAMPLE = 6;
        int                      _frameCount{ 0 };

        // Inventory change detection: track the cached weight as a cheap proxy.
        // A weight change triggers a full inventory re-scan.
        float                    _lastInventoryWeight{ -1.0f };
    };
}
