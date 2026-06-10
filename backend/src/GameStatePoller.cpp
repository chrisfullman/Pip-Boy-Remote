#include "PCH.h"
#include "GameStatePoller.h"
#include "GameState.h"
#include "WebSocketServer.h"

#include <REX/LOG.h>

// <wingdi.h> (pulled in by windows.h) defines ERROR as 0, which causes
// REX::ERROR to preprocess to REX::0 — an illegal token.  Undefine it
// after all Windows headers have been included via PCH.h.
#ifdef ERROR
#    undef ERROR
#endif

namespace PipBoyRemote
{
    // Maps a Fallout 4 form type to our ItemCategory enum.
    static ItemCategory FormTypeToCategory(RE::ENUM_FORM_ID formType)
    {
        switch (formType) {
            case RE::ENUM_FORM_ID::kWEAP: return ItemCategory::Weapon;
            case RE::ENUM_FORM_ID::kARMO: return ItemCategory::Armor;
            case RE::ENUM_FORM_ID::kALCH: return ItemCategory::Aid;
            case RE::ENUM_FORM_ID::kAMMO: return ItemCategory::Ammo;
            case RE::ENUM_FORM_ID::kMISC: return ItemCategory::Misc;
            default:                       return ItemCategory::Other;
        }
    }

    // Maps a MARKER_TYPE value to the string key used by the frontend to load
    // the corresponding SVG icon (e.g. "Cave" → "/maps/markers/CaveMarker.svg").
    // Returns an empty string for types without a dedicated icon; the frontend
    // will fall back to a plain coloured disc.
    static std::string MarkerTypeToString(RE::MARKER_TYPE type)
    {
        switch (type) {
            case RE::MARKER_TYPE::kCave:             return "Cave";
            case RE::MARKER_TYPE::kCity:             return "City";
            case RE::MARKER_TYPE::kDiamondCity:      return "DiamondCity";
            case RE::MARKER_TYPE::kEncampment:       return "Encampment";
            case RE::MARKER_TYPE::kIndustrial:       return "Industrial";
            case RE::MARKER_TYPE::kGovtBuilding:     return "GovtBuilding";
            case RE::MARKER_TYPE::kMetro:            return "Metro";
            case RE::MARKER_TYPE::kMilitaryBase:     return "MilitaryBase";
            case RE::MARKER_TYPE::kLandmark:         return "Landmark";
            case RE::MARKER_TYPE::kOffice:           return "Office";
            case RE::MARKER_TYPE::kRuinsTown:        return "RuinsTown";
            case RE::MARKER_TYPE::kRuinsUrban:       return "RuinsUrban";
            case RE::MARKER_TYPE::kSanctuary:        return "Sanctuary";
            case RE::MARKER_TYPE::kSettlement:       return "Settlement";
            case RE::MARKER_TYPE::kSewer:            return "Sewer";
            case RE::MARKER_TYPE::kVault:            return "Vault";
            case RE::MARKER_TYPE::kAirfield:         return "Airfield";
            case RE::MARKER_TYPE::kBunkerHill:       return "BunkerHill";
            case RE::MARKER_TYPE::kChurch:           return "Church";
            case RE::MARKER_TYPE::kFarm:             return "Farm";
            case RE::MARKER_TYPE::kFillingStation:   return "FillingStation";
            case RE::MARKER_TYPE::kForest:           return "Forest";
            case RE::MARKER_TYPE::kGoodNeighbor:     return "GoodNeighbor";
            case RE::MARKER_TYPE::kGraveyard:        return "Graveyard";
            case RE::MARKER_TYPE::kHospital:         return "Hospital";
            case RE::MARKER_TYPE::kInstitute:        return "Institute";
            case RE::MARKER_TYPE::kJunkyard:         return "Junkyard";
            case RE::MARKER_TYPE::kObservatory:      return "Observatory";
            case RE::MARKER_TYPE::kPier:             return "Pier";
            case RE::MARKER_TYPE::kQuarry:           return "Quarry";
            case RE::MARKER_TYPE::kRadioTower:       return "RadioTower";
            case RE::MARKER_TYPE::kSalem:            return "Salem";
            case RE::MARKER_TYPE::kSchool:           return "School";
            case RE::MARKER_TYPE::kShipwreck:        return "Shipwreck";
            case RE::MARKER_TYPE::kSubmarine:        return "Submarine";
            case RE::MARKER_TYPE::kTown:             return "Town";
            case RE::MARKER_TYPE::kBOS:              return "BOS";
            case RE::MARKER_TYPE::kBunker:           return "Bunker";
            case RE::MARKER_TYPE::kCastle:           return "Castle";
            case RE::MARKER_TYPE::kMinutemen:        return "Minutemen";
            case RE::MARKER_TYPE::kPoliceStation:    return "PoliceStation";
            case RE::MARKER_TYPE::kPrydwen:          return "Prydwen";
            case RE::MARKER_TYPE::kRailroadFaction:  return "RailroadFaction";
            case RE::MARKER_TYPE::kRailroad:         return "Railroad";
            case RE::MARKER_TYPE::kUSSConstitution:  return "USSConstitution";
            default:                                  return "";
        }
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Singleton
    // ──────────────────────────────────────────────────────────────────────────

    GameStatePoller& GameStatePoller::GetSingleton()
    {
        static GameStatePoller instance;
        return instance;
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Lifecycle
    // ──────────────────────────────────────────────────────────────────────────

    void GameStatePoller::Register()
    {
        if (_registered.exchange(true)) { return; }

        const auto* ti = F4SE::GetTaskInterface();
        if (!ti) {
            REX::ERROR("[PipBoyRemote] Task interface unavailable; game state polling disabled");
            return;
        }

        // AddTaskPermanent queues a recurring task on the game thread.
        // We capture `this` safely because the singleton outlives the game session.
        ti->AddTaskPermanent([this]() { OnGameFrame(); });
        REX::INFO("[PipBoyRemote] Game state poller registered");
    }

    void GameStatePoller::Start()
    {
        _frameCount           = 0;
        _lastInventoryWeight  = -1.0f;  // force an inventory scan on the first frame
        _markerRescanCounter  = 0;
        _forceMapMarkerRescan.store(true, std::memory_order_relaxed);  // force a marker scan on the first active frame
        _lastWorldspace       = nullptr;
        _questRescanCounter   = 0;
        _forceQuestRescan     = true;   // force a quest scan on the first active frame
        _active.store(true, std::memory_order_release);
    }

    void GameStatePoller::Pause()
    {
        _active.store(false, std::memory_order_release);
    }

    void GameStatePoller::Resume()
    {
        _active.store(true, std::memory_order_release);
    }

    void GameStatePoller::Stop()
    {
        _active.store(false, std::memory_order_release);
    }

    void GameStatePoller::SetWaypointFormID(std::uint32_t formID) noexcept
    {
        _waypointFormID.store(formID, std::memory_order_relaxed);
        // Trigger an immediate marker rescan so the star appears on the next game
        // frame rather than waiting up to FRAMES_PER_MARKER_RESCAN frames (~10 s).
        _forceMapMarkerRescan.store(true, std::memory_order_relaxed);
    }

    void GameStatePoller::ForceInventoryRescan() noexcept
    {
        _forceInventoryRescan.store(true, std::memory_order_relaxed);
    }

    void GameStatePoller::SetActiveQuestFormID(std::uint32_t formID) noexcept
    {
        _pendingActiveQuestFormID.store(formID, std::memory_order_relaxed);
    }

    // ──────────────────────────────────────────────────────────────────────────
    // Per-frame sampling (game main thread)
    // ──────────────────────────────────────────────────────────────────────────

    void GameStatePoller::OnGameFrame()
    {
        if (!_active.load(std::memory_order_acquire)) { return; }
        if (!WebSocketServer::GetSingleton().HasClients()) { return; }

        ++_frameCount;
        if (_frameCount < FRAMES_PER_SAMPLE) { return; }
        _frameCount = 0;

        SamplePlayerState();
        SampleInventory();

        // Increment the marker rescan counter; flag a forced rescan when it expires.
        ++_markerRescanCounter;
        if (_markerRescanCounter >= FRAMES_PER_MARKER_RESCAN) {
            _markerRescanCounter = 0;
            _forceMapMarkerRescan.store(true, std::memory_order_relaxed);
        }
        SampleMapMarkers();

        // Quest rescan: run on the periodic interval or immediately when a
        // set_active_quest action is pending.
        ++_questRescanCounter;
        if (_questRescanCounter >= FRAMES_PER_QUEST_RESCAN) {
            _questRescanCounter = 0;
            _forceQuestRescan   = true;
        }
        const bool hasPendingQuestAction =
            _pendingActiveQuestFormID.load(std::memory_order_relaxed) != 0;
        if (_forceQuestRescan || hasPendingQuestAction) {
            SampleQuests();
        }
    }

    void GameStatePoller::SamplePlayerState()
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) { return; }

        const auto* av = RE::ActorValue::GetSingleton();
        if (!av) { return; }

        PlayerState state;

        // Core vitals
        state.health          = player->GetActorValue(*av->health);
        state.maxHealth       = player->GetBaseActorValue(*av->health);
        state.actionPoints    = player->GetActorValue(*av->actionPoints);
        state.maxActionPoints = player->GetBaseActorValue(*av->actionPoints);

        // Carry weight: base stat is max carry capacity.
        // Current weight is the cached total from the inventory list.
        state.maxCarryWeight = player->GetActorValue(*av->carryWeight);
        state.currentWeight  = player->inventoryList ? player->inventoryList->cachedWeight : 0.0f;

        // World position
        state.x = player->GetPositionX();
        state.y = player->GetPositionY();
        state.z = player->GetPositionZ();

        // Limb conditions (base conditions, not power-armor)
        state.condition.head     = player->GetActorValue(*av->brainCondition);
        state.condition.torso    = player->GetActorValue(*av->enduranceCondition);
        state.condition.leftArm  = player->GetActorValue(*av->leftAttackCondition);
        state.condition.rightArm = player->GetActorValue(*av->rightAttackCondition);
        state.condition.leftLeg  = player->GetActorValue(*av->leftMobiltyCondition);   // typo in RE header intentional
        state.condition.rightLeg = player->GetActorValue(*av->rightMobilityCondition);

        // Power-armor limb conditions
        state.condition.paHead     = player->GetActorValue(*av->powerArmorHeadCondition);
        state.condition.paTorso    = player->GetActorValue(*av->powerArmorTorsoCondition);
        state.condition.paLeftArm  = player->GetActorValue(*av->powerArmorLeftArmCondition);
        state.condition.paRightArm = player->GetActorValue(*av->powerArmorRightArmCondition);
        state.condition.paLeftLeg  = player->GetActorValue(*av->powerArmorLeftLegCondition);
        state.condition.paRightLeg = player->GetActorValue(*av->powerArmorRightLegCondition);

        // Level: Actor::GetLevel() is the authoritative API (returns int16_t).
        state.level = static_cast<float>(player->GetLevel());

        // Current accumulated XP via the actor value system.
        state.experience = player->GetActorValue(*av->experience);

        // Cumulative XP threshold to reach the next level.
        // Each level L costs (iXPBase + (L-1)*iXPBumpBase) XP to clear, so the
        // running total to reach level (L+1) from level 1 is:
        //   L * iXPBase + iXPBumpBase * L*(L-1)/2
        // Verified against live session data:
        //   Level 1 → 2: 1*200 + 0          = 200  ✓
        //   Level 2 → 3: 2*200 + 50*1       = 450  (player had 205 XP at L2, not yet 450)
        //   Level 3 → 4: 3*200 + 50*3       = 750  ✓ (matches published FO4 tables)
        // The frontend hides the XP bar when this field is 0 (GMST unavailable).
        state.nextLevelXP = 0.0f;
        {
            auto* gsc = RE::GameSettingCollection::GetSingleton();
            if (gsc) {
                auto* xpBase     = gsc->GetSetting("iXPBase");
                auto* xpBumpBase = gsc->GetSetting("iXPBumpBase");
                if (xpBase && xpBumpBase) {
                    const auto L    = static_cast<std::int32_t>(state.level);
                    const auto base = xpBase->GetInt();
                    const auto bump = xpBumpBase->GetInt();
                    state.nextLevelXP  = static_cast<float>(L * base + bump * L * (L - 1) / 2);
                    // Same formula at (L-1) gives the cumulative XP at the start of this level.
                    const auto prevL   = L - 1;
                    state.levelStartXP = static_cast<float>(prevL * base + bump * prevL * (prevL - 1) / 2);
                }
            }
        }

        WebSocketServer::GetSingleton().BroadcastStateUpdate(state);
    }

    void GameStatePoller::SampleInventory()
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player || !player->inventoryList) { return; }

        // Use the cached inventory weight as a cheap change detector.
        // A full rescan is also forced when ForceInventoryRescan() has been called
        // (e.g. after equip/unequip, which doesn't change weight for carried items).
        const float currentWeight  = player->inventoryList->cachedWeight;
        const bool  forcedRescan   = _forceInventoryRescan.exchange(false, std::memory_order_relaxed);
        if (currentWeight == _lastInventoryWeight && !forcedRescan) { return; }
        _lastInventoryWeight = currentWeight;

        InventorySnapshot snapshot;

        // Lock the inventory for reading with a simple RAII guard.
        struct ReadGuard {
            RE::BSReadWriteLock& lock;
            explicit ReadGuard(RE::BSReadWriteLock& l) noexcept : lock(l) { lock.lock_read(); }
            ~ReadGuard() noexcept { lock.unlock_read(); }
        } guard(player->inventoryList->rwLock);

        player->inventoryList->ForEachStack(
            // Filter: always descend into every item's stack list.
            [](RE::BGSInventoryItem& /*item*/) { return true; },

            // Per-stack callback: accumulate stacks for the same form into one entry.
            [&snapshot](RE::BGSInventoryItem& item, RE::BGSInventoryItem::Stack& stack) {
                if (!item.object) { return true; }

                InventoryItem entry;
                entry.formID     = item.object->GetFormID();
                entry.category   = FormTypeToCategory(item.object->GetFormType());
                entry.count      = static_cast<std::int64_t>(stack.GetCount());
                entry.isEquipped = stack.IsEquipped();
                entry.weight     = item.GetTotalWeight();
                entry.value      = item.GetInventoryValue(0, false);

                // GetFullName returns the form's display name (localized).
                const auto nameView = RE::TESFullName::GetFullName(*item.object);
                entry.name = std::string{ nameView };

                snapshot.items.push_back(std::move(entry));
                return true;  // continue iteration
            }
        );

        WebSocketServer::GetSingleton().BroadcastInventoryUpdate(snapshot);
    }

    void GameStatePoller::SampleMapMarkers()
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) { return; }

        // Determine the player's current worldspace.  Interior cells have no worldspace.
        RE::TESWorldSpace* currentWorldspace = nullptr;
        const auto* playerCell = player->GetParentCell();
        if (playerCell && playerCell->IsExterior()) {
            currentWorldspace = playerCell->worldSpace;
        }

        // Only rescan when the worldspace changes or a periodic/forced rescan is due.
        // exchange(false) atomically reads and clears the flag in one operation,
        // preventing a write-from-WebSocket-thread data race.
        const bool forceRescan = _forceMapMarkerRescan.exchange(false, std::memory_order_relaxed);
        if (currentWorldspace == _lastWorldspace && !forceRescan) { return; }
        _lastWorldspace = currentWorldspace;

        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) { return; }

        MapMarkersSnapshot snapshot;

        if (currentWorldspace) {
            snapshot.worldspace = currentWorldspace->editorID.c_str();
        }

        // MapMarkerData::flags bit definitions (RE'd from game binary):
        //   0x01 = kVisible — marker is shown on the player's map (= discovered)
        //   0x02 = kCanTravelTo — player can fast travel to this location
        static constexpr std::uint8_t FLAG_VISIBLE     = 0x01;
        static constexpr std::uint8_t FLAG_CAN_TRAVEL  = 0x02;

        // Iterate all BGSLocation forms.  Each named location optionally carries a
        // worldLocMarker handle pointing to the TESObjectREFR that has the visible
        // map marker pin and its MapMarkerData extra data.
        for (auto* loc : dataHandler->GetFormArray<RE::BGSLocation>()) {
            if (!loc) { continue; }

            // Resolve the handle to a live reference; skip invalid/unloaded handles.
            const RE::NiPointer<RE::TESObjectREFR> markerRef = loc->worldLocMarker.get();
            if (!markerRef) { continue; }

            // Skip markers that are not in the current worldspace.
            const auto* cell = markerRef->GetParentCell();
            if (!cell || !cell->IsExterior() || cell->worldSpace != currentWorldspace) { continue; }

            // Get the map marker extra data attached to the reference.
            if (!markerRef->extraList) { continue; }
            const auto* extra = markerRef->extraList->GetByType<RE::ExtraMapMarker>();
            if (!extra || !extra->mapMarkerData) { continue; }

            const auto* mapData = extra->mapMarkerData;

            // MapMarkerData::flags and ::type are private (no public accessors).
            // Read them directly at their documented byte offsets from the RE'd header:
            //   0x10 = flags (uint8_t), 0x12 = type (MARKER_TYPE / uint32_t).
            const auto* mapDataBytes    = reinterpret_cast<const std::byte*>(mapData);
            const auto  rawFlags        = *reinterpret_cast<const std::uint8_t*>(mapDataBytes + 0x10);
            RE::MARKER_TYPE rawType{};
            std::memcpy(&rawType, mapDataBytes + 0x12, sizeof(rawType));

            MapMarker marker;
            marker.formID           = markerRef->GetFormID();
            marker.name             = mapData->GetFullName();
            marker.x                = markerRef->GetPositionX();
            marker.y                = markerRef->GetPositionY();
            marker.isDiscovered     = (rawFlags & FLAG_VISIBLE)    != 0;
            marker.isFastTravelable = (rawFlags & FLAG_CAN_TRAVEL) != 0;
            marker.markerType       = MarkerTypeToString(rawType);

            snapshot.markers.push_back(std::move(marker));
        }

        // Report the last waypoint the frontend explicitly set.  In-game quest-marker
        // waypoints (set through the Pip-Boy objectives screen) are not yet tracked;
        // that requires walking the quest journal for the active objective's reference.
        snapshot.activeWaypointID = _waypointFormID.load(std::memory_order_relaxed);

        WebSocketServer::GetSingleton().BroadcastMapMarkersUpdate(snapshot);
    }

    void GameStatePoller::SampleQuests()
    {
        _forceQuestRescan = false;

        auto* dataHandler = RE::TESDataHandler::GetSingleton();
        if (!dataHandler) { return; }

        // Apply any pending "set active quest" request from the frontend.
        // Setting kDisplayedInHUD on the desired quest and clearing it on all others
        // tells the game to track that quest in the HUD.
        const auto pendingFormID =
            _pendingActiveQuestFormID.exchange(0, std::memory_order_relaxed);
        if (pendingFormID != 0) {
            for (auto* quest : dataHandler->GetFormArray<RE::TESQuest>()) {
                if (!quest) { continue; }
                const auto desiredBit = static_cast<std::uint16_t>(RE::QuestFlag::kDisplayedInHUD);
                if (quest->GetFormID() == pendingFormID) {
                    quest->data.flags |= desiredBit;
                } else {
                    quest->data.flags &= static_cast<std::uint16_t>(~desiredBit);
                }
            }
        }

        QuestSnapshot snapshot;

        for (auto* quest : dataHandler->GetFormArray<RE::TESQuest>()) {
            if (!quest) { continue; }

            const auto flags = quest->data.flags;

            // Skip quests that are not actively running or have already ended.
            const bool isActive    = (flags & static_cast<std::uint16_t>(RE::QuestFlag::kActive))    != 0;
            const bool isCompleted = (flags & static_cast<std::uint16_t>(RE::QuestFlag::kCompleted)) != 0;
            const bool isFailed    = (flags & static_cast<std::uint16_t>(RE::QuestFlag::kFailed))    != 0;
            if (!isActive || isCompleted || isFailed) { continue; }

            QuestEntry entry;
            entry.formID       = quest->GetFormID();
            entry.currentStage = quest->currentStage;
            entry.isTracked    = (flags & static_cast<std::uint16_t>(RE::QuestFlag::kDisplayedInHUD)) != 0;

            const auto nameView = RE::TESFullName::GetFullName(*quest);
            entry.name = std::string{ nameView };

            // Skip internal/system quests with no display name.
            if (entry.name.empty()) { continue; }

            if (entry.isTracked) {
                snapshot.activeQuestFormID = entry.formID;
            }

            // Collect objectives that are currently displayed to the player.
            // kDisplayed  = active objective; kCompletedDisplayed = just completed.
            for (std::uint32_t i = 0; i < quest->objectives.size(); ++i) {
                auto* obj = quest->objectives[i];
                if (!obj) { continue; }

                const auto state = static_cast<RE::QUEST_OBJECTIVE_STATE>(obj->state);
                if (state != RE::QUEST_OBJECTIVE_STATE::kDisplayed &&
                    state != RE::QUEST_OBJECTIVE_STATE::kCompletedDisplayed) {
                    continue;
                }

                QuestObjective objective;
                objective.index       = obj->index;
                objective.text        = obj->displayText.c_str();
                objective.isCompleted = (state == RE::QUEST_OBJECTIVE_STATE::kCompletedDisplayed);
                entry.objectives.push_back(std::move(objective));
            }

            snapshot.quests.push_back(std::move(entry));
        }

        WebSocketServer::GetSingleton().BroadcastQuestUpdate(snapshot);
    }
}
