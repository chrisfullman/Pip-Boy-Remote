#pragma once

#include "GameState.h"
#include <string>

// Builders that serialize game state snapshots into JSON strings conforming to
// the schemas in schema/.  All functions are pure (no side effects) and safe to
// call from any thread.

namespace PipBoyRemote::JsonMessages
{
    // schema/heartbeat.schema.json
    // Sent periodically to confirm the server is alive.
    // connected = true when at least one WebSocket client is currently open.
    std::string BuildHeartbeat(bool connected);

    // schema/state-update.schema.json
    // Sent when player vitals, position, or limb conditions change.
    std::string BuildStateUpdate(const PlayerState& state);

    // schema/inventory-update.schema.json
    // Sent when the player's inventory changes.
    std::string BuildInventoryUpdate(const InventorySnapshot& snapshot);

    // schema/action-response.schema.json
    // Sent in response to an action request from the frontend.
    std::string BuildActionResponse(std::string_view action, bool success, std::string_view errorMessage = "");

    // schema/map-markers-update.schema.json
    // Sent on worldspace entry and periodically to reflect newly-discovered locations.
    std::string BuildMapMarkersUpdate(const MapMarkersSnapshot& snapshot);
}
