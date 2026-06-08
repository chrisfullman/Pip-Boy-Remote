#include "JsonMessages.h"

#include <chrono>
#include <ctime>
#include <format>

#include <nlohmann/json.hpp>

namespace PipBoyRemote::JsonMessages
{
    // Returns the current UTC time as an ISO 8601 date-time string, e.g.
    // "2026-06-07T14:30:00.123Z".  Uses Windows-safe gmtime_s / POSIX gmtime_r
    // depending on the compiler.
    static std::string CurrentTimestamp()
    {
        const auto now  = std::chrono::system_clock::now();
        const auto tt   = std::chrono::system_clock::to_time_t(now);
        const auto ms   = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

        std::tm utc{};
#ifdef _WIN32
        ::gmtime_s(&utc, &tt);
#else
        ::gmtime_r(&tt, &utc);
#endif

        return std::format(
            "{:04d}-{:02d}-{:02d}T{:02d}:{:02d}:{:02d}.{:03d}Z",
            utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
            utc.tm_hour, utc.tm_min, utc.tm_sec,
            static_cast<int>(ms)
        );
    }

    std::string BuildHeartbeat(bool connected)
    {
        const nlohmann::json msg = {
            { "type",      "heartbeat"        },
            { "timestamp", CurrentTimestamp() },
            { "connected", connected          },
        };
        return msg.dump();
    }

    std::string BuildStateUpdate(const PlayerState& state)
    {
        const nlohmann::json msg = {
            { "type",      "state_update"     },
            { "timestamp", CurrentTimestamp() },
            { "player", {
                { "health",          state.health          },
                { "maxHealth",       state.maxHealth        },
                { "actionPoints",    state.actionPoints     },
                { "maxActionPoints", state.maxActionPoints  },
                { "currentWeight",   state.currentWeight    },
                { "maxCarryWeight",  state.maxCarryWeight   },
                { "coords",          { state.x, state.y, state.z } },
                { "level",           state.level            },
                { "experience",      state.experience       },
                { "nextLevelXP",     state.nextLevelXP      },
                { "condition", {
                    { "head",      state.condition.head      },
                    { "torso",     state.condition.torso     },
                    { "leftArm",   state.condition.leftArm   },
                    { "rightArm",  state.condition.rightArm  },
                    { "leftLeg",   state.condition.leftLeg   },
                    { "rightLeg",  state.condition.rightLeg  },
                    { "powerArmor", {
                        { "head",      state.condition.paHead     },
                        { "torso",     state.condition.paTorso    },
                        { "leftArm",   state.condition.paLeftArm  },
                        { "rightArm",  state.condition.paRightArm },
                        { "leftLeg",   state.condition.paLeftLeg  },
                        { "rightLeg",  state.condition.paRightLeg },
                    }},
                }},
            }},
        };
        return msg.dump();
    }

    std::string BuildInventoryUpdate(const InventorySnapshot& snapshot)
    {
        nlohmann::json items = nlohmann::json::array();
        for (const auto& item : snapshot.items) {
            items.push_back({
                { "formID",     item.formID                              },
                { "name",       item.name                                },
                { "category",   static_cast<std::uint8_t>(item.category) },
                { "count",      item.count                               },
                { "isEquipped", item.isEquipped                          },
                { "weight",     item.weight                              },
                { "value",      item.value                               },
            });
        }

        const nlohmann::json msg = {
            { "type",      "inventory_update" },
            { "timestamp", CurrentTimestamp() },
            { "items",     std::move(items)   },
        };
        return msg.dump();
    }

    std::string BuildActionResponse(std::string_view action, bool success, std::string_view errorMessage)
    {
        nlohmann::json msg = {
            { "type",      "action_response"  },
            { "timestamp", CurrentTimestamp() },
            { "action",    action             },
            { "success",   success            },
        };
        if (!errorMessage.empty()) {
            msg["error"] = errorMessage;
        }
        return msg.dump();
    }

    std::string BuildMapMarkersUpdate(const MapMarkersSnapshot& snapshot)
    {
        nlohmann::json markers = nlohmann::json::array();
        for (const auto& m : snapshot.markers) {
            nlohmann::json entry = {
                { "formID",          m.formID           },
                { "name",            m.name             },
                { "coords",          { m.x, m.y }       },
                { "isDiscovered",    m.isDiscovered      },
                { "isFastTravelable", m.isFastTravelable },
            };
            if (!m.markerType.empty()) {
                entry["markerType"] = m.markerType;
            }
            markers.push_back(std::move(entry));
        }

        nlohmann::json msg = {
            { "type",      "map_markers_update" },
            { "timestamp", CurrentTimestamp()   },
            { "markers",   std::move(markers)   },
        };
        if (!snapshot.worldspace.empty()) {
            msg["worldspace"] = snapshot.worldspace;
        }
        if (snapshot.activeWaypointID != 0) {
            msg["activeWaypointID"] = snapshot.activeWaypointID;
        }
        return msg.dump();
    }
}
