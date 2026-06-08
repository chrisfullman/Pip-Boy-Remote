#include <catch2/catch_all.hpp>
#include <nlohmann/json.hpp>

#include "GameState.h"
#include "JsonMessages.h"

using namespace PipBoyRemote;

// ──────────────────────────────────────────────────────────────────────────────
// Heartbeat
// ──────────────────────────────────────────────────────────────────────────────

TEST_CASE("BuildHeartbeat produces valid JSON", "[json][heartbeat]")
{
    const auto raw = JsonMessages::BuildHeartbeat(true);
    REQUIRE_NOTHROW(nlohmann::json::parse(raw));

    const auto j = nlohmann::json::parse(raw);
    CHECK(j["type"]      == "heartbeat");
    CHECK(j["connected"] == true);
    CHECK(j.contains("timestamp"));
    CHECK(j["timestamp"].is_string());
}

TEST_CASE("BuildHeartbeat reflects connected=false", "[json][heartbeat]")
{
    const auto j = nlohmann::json::parse(JsonMessages::BuildHeartbeat(false));
    CHECK(j["connected"] == false);
}

// ──────────────────────────────────────────────────────────────────────────────
// State update
// ──────────────────────────────────────────────────────────────────────────────

TEST_CASE("BuildStateUpdate produces valid JSON with required fields", "[json][state]")
{
    PlayerState state;
    state.health          = 85.0f;
    state.maxHealth       = 100.0f;
    state.actionPoints    = 50.0f;
    state.maxActionPoints = 75.0f;
    state.currentWeight   = 120.5f;
    state.maxCarryWeight  = 200.0f;
    state.x               = 12345.6f;
    state.y               = -9876.5f;
    state.z               = 300.0f;

    const auto raw = JsonMessages::BuildStateUpdate(state);
    REQUIRE_NOTHROW(nlohmann::json::parse(raw));

    const auto j = nlohmann::json::parse(raw);

    REQUIRE(j.contains("type"));
    CHECK(j["type"] == "state_update");

    REQUIRE(j.contains("player"));
    const auto& p = j["player"];
    CHECK(p["health"]          == Catch::Approx(85.0));
    CHECK(p["maxHealth"]       == Catch::Approx(100.0));
    CHECK(p["actionPoints"]    == Catch::Approx(50.0));
    CHECK(p["maxActionPoints"] == Catch::Approx(75.0));
    CHECK(p["currentWeight"]   == Catch::Approx(120.5));
    CHECK(p["maxCarryWeight"]  == Catch::Approx(200.0));

    REQUIRE(p.contains("coords"));
    REQUIRE(p["coords"].is_array());
    CHECK(p["coords"].size() == 3);

    REQUIRE(p.contains("condition"));
    CHECK(p["condition"].contains("head"));
    CHECK(p["condition"].contains("torso"));
    CHECK(p["condition"].contains("leftArm"));
    CHECK(p["condition"].contains("rightArm"));
    CHECK(p["condition"].contains("leftLeg"));
    CHECK(p["condition"].contains("rightLeg"));
    CHECK(p["condition"].contains("powerArmor"));
}

// ──────────────────────────────────────────────────────────────────────────────
// Inventory update
// ──────────────────────────────────────────────────────────────────────────────

TEST_CASE("BuildInventoryUpdate produces valid JSON", "[json][inventory]")
{
    InventorySnapshot snap;

    InventoryItem weapon;
    weapon.formID     = 0x0001F278;
    weapon.name       = "10mm Pistol";
    weapon.category   = ItemCategory::Weapon;
    weapon.count      = 1;
    weapon.isEquipped = true;
    weapon.weight     = 4.0f;
    weapon.value      = 50;
    snap.items.push_back(std::move(weapon));

    InventoryItem stimpak;
    stimpak.formID     = 0x00023736;
    stimpak.name       = "Stimpak";
    stimpak.category   = ItemCategory::Aid;
    stimpak.count      = 10;
    stimpak.isEquipped = false;
    stimpak.weight     = 0.1f;
    stimpak.value      = 35;
    snap.items.push_back(std::move(stimpak));

    const auto raw = JsonMessages::BuildInventoryUpdate(snap);
    REQUIRE_NOTHROW(nlohmann::json::parse(raw));

    const auto j = nlohmann::json::parse(raw);
    CHECK(j["type"] == "inventory_update");
    REQUIRE(j.contains("items"));
    REQUIRE(j["items"].is_array());
    CHECK(j["items"].size() == 2);

    const auto& first = j["items"][0];
    CHECK(first["name"]       == "10mm Pistol");
    CHECK(first["isEquipped"] == true);
    CHECK(first["count"]      == 1);
}

TEST_CASE("BuildInventoryUpdate handles empty inventory", "[json][inventory]")
{
    InventorySnapshot empty;
    const auto j = nlohmann::json::parse(JsonMessages::BuildInventoryUpdate(empty));
    CHECK(j["type"]        == "inventory_update");
    CHECK(j["items"].size() == 0);
}

// ──────────────────────────────────────────────────────────────────────────────
// Action response
// ──────────────────────────────────────────────────────────────────────────────

TEST_CASE("BuildActionResponse success has no error field", "[json][action]")
{
    const auto j = nlohmann::json::parse(JsonMessages::BuildActionResponse("equip", true));
    CHECK(j["type"]    == "action_response");
    CHECK(j["action"]  == "equip");
    CHECK(j["success"] == true);
    CHECK_FALSE(j.contains("error"));
}

TEST_CASE("BuildActionResponse failure includes error message", "[json][action]")
{
    const auto j = nlohmann::json::parse(
        JsonMessages::BuildActionResponse("fast_travel", false, "Fast travel is disabled indoors")
    );
    CHECK(j["success"] == false);
    CHECK(j["error"]   == "Fast travel is disabled indoors");
}
