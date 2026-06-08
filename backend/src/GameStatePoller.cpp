#include "PCH.h"
#include "GameStatePoller.h"
#include "GameState.h"
#include "WebSocketServer.h"

#include <REX/LOG.h>

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

        // nextLevelXP requires reading the iXPBase / fXPLevelUpMult GMSTs and
        // applying the Fallout 4 XP curve formula.  These GMST names must be
        // confirmed against a live game session before use, so this field
        // is left as 0.0f for now.  The frontend treats 0 as "unavailable".
        state.nextLevelXP = 0.0f;

        WebSocketServer::GetSingleton().BroadcastStateUpdate(state);
    }

    void GameStatePoller::SampleInventory()
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player || !player->inventoryList) { return; }

        // Use the cached inventory weight as a cheap change detector.
        // A full rescan is only performed when the weight changes.
        const float currentWeight = player->inventoryList->cachedWeight;
        if (currentWeight == _lastInventoryWeight) { return; }
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
}
