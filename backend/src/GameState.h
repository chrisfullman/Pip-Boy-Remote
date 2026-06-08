#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Plain data structures that describe a snapshot of the player's game state.
// No game-engine headers are included here so this file can be used in tests.

namespace PipBoyRemote
{
    // Condition percentage (0.0 = destroyed, 100.0 = full) for each player limb.
    struct LimbConditions
    {
        float head      = 100.0f;
        float torso     = 100.0f;
        float leftArm   = 100.0f;
        float rightArm  = 100.0f;
        float leftLeg   = 100.0f;
        float rightLeg  = 100.0f;

        // Power-armor limb conditions (populated when the player wears power armor).
        float paHead     = 100.0f;
        float paTorso    = 100.0f;
        float paLeftArm  = 100.0f;
        float paRightArm = 100.0f;
        float paLeftLeg  = 100.0f;
        float paRightLeg = 100.0f;
    };

    // Snapshot of the player's vitals and world position.
    // Written by GameStatePoller on the game thread; read by WebSocketServer.
    struct PlayerState
    {
        float health           = 0.0f;
        float maxHealth        = 0.0f;
        float actionPoints     = 0.0f;
        float maxActionPoints  = 0.0f;
        float currentWeight    = 0.0f;   // total inventory weight
        float maxCarryWeight   = 0.0f;

        // World coordinates in Fallout 4's unit system (1 unit ≈ 1.42 cm)
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        LimbConditions condition;

        // Player character level and experience
        float level              = 1.0f;
        float experience         = 0.0f;
        float nextLevelXP        = 200.0f;
    };

    // Category tags that mirror the Pip-Boy's inventory tabs.
    enum class ItemCategory : std::uint8_t
    {
        Weapon = 0,
        Armor  = 1,
        Aid    = 2,   // food, drink, medication
        Ammo   = 3,
        Misc   = 4,
        Other  = 255,
    };

    // A single item entry from the player's inventory.
    struct InventoryItem
    {
        std::uint32_t formID     = 0;
        std::string   name;
        ItemCategory  category   = ItemCategory::Other;
        std::int64_t  count      = 0;
        bool          isEquipped = false;
        float         weight     = 0.0f;
        std::int32_t  value      = 0;   // base game value in caps
    };

    // Complete snapshot of the player's inventory.
    struct InventorySnapshot
    {
        std::vector<InventoryItem> items;
    };

    // A single discoverable location shown on the world map.
    struct MapMarker
    {
        std::uint32_t formID            = 0;
        std::string   name;
        float         x                = 0.0f;
        float         y                = 0.0f;
        bool          isDiscovered     = false;  // player has visited; shown on map
        bool          isFastTravelable = false;  // player can fast travel here
        std::string   markerType;                // frontend icon key, e.g. "Cave", "Settlement"
    };

    // Complete snapshot of map markers for a worldspace.
    // Written by GameStatePoller on the game thread; read by WebSocketServer.
    struct MapMarkersSnapshot
    {
        std::vector<MapMarker> markers;
        std::string            worldspace;             // worldspace editor ID, e.g. "Commonwealth"
        std::uint32_t          activeWaypointID = 0;   // form ID of active waypoint; 0 if none
    };
}
