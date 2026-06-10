<script setup lang="ts">
import { computed } from 'vue'
import { useQuestStore } from '@/stores/quest'
import { useConnectionStore } from '@/stores/connection'

const quest = useQuestStore()
const conn  = useConnectionStore()

/** Quests that are running but not currently tracked. */
const otherQuests = computed(() =>
  quest.quests.filter(q => q.formID !== quest.activeQuestFormID)
)

function setActiveQuest(formID: number): void {
  conn.sendAction({ action: 'set_active_quest', formID })
}
</script>

<template>
  <section class="surface quest-view">

    <div class="quest-header">
      <span class="label">Quests</span>
      <span class="label quest-count" v-if="quest.quests.length">
        {{ quest.quests.length }} active
      </span>
    </div>

    <div v-if="!quest.quests.length" class="empty">
      No active quests.
    </div>

    <template v-else>

      <!-- ── Tracked quest ─────────────────────────────────────────── -->
      <div v-if="quest.trackedQuest" class="tracked-quest">
        <div class="tracked-header">
          <span class="label tracked-label">Tracked Quest</span>
          <span class="label stage-badge">Stage {{ quest.trackedQuest.currentStage }}</span>
        </div>
        <div class="quest-name">{{ quest.trackedQuest.name }}</div>

        <ul v-if="quest.trackedQuest.objectives.length" class="objectives">
          <li
            v-for="obj in quest.trackedQuest.objectives"
            :key="obj.index"
            class="objective"
            :class="{ 'objective--completed': obj.isCompleted }"
          >
            <span class="objective__bullet">{{ obj.isCompleted ? '✓' : '●' }}</span>
            <span class="objective__text">{{ obj.text }}</span>
          </li>
        </ul>
        <div v-else class="empty-objectives label">No active objectives.</div>
      </div>

      <div v-else class="untracked-notice label">
        No quest is currently tracked.
      </div>

      <!-- ── Other running quests ──────────────────────────────────── -->
      <div v-if="otherQuests.length" class="other-quests">
        <div class="label other-label">Other Quests</div>
        <div
          v-for="q in otherQuests"
          :key="q.formID"
          class="quest-row"
        >
          <div class="quest-row__info">
            <span class="quest-row__name">{{ q.name }}</span>
            <span class="label quest-row__stage">Stage {{ q.currentStage }}</span>
          </div>
          <button
            class="btn"
            :disabled="!conn.isConnected"
            title="Set as tracked quest"
            @click="setActiveQuest(q.formID)"
          >
            Track
          </button>
        </div>
      </div>

    </template>
  </section>
</template>

<style scoped>
.quest-view {
  display: flex;
  flex-direction: column;
  gap: 12px;
  padding: 12px 14px;
}

/* ── Header ────────────────────────────────────────────────── */

.quest-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.quest-count {
  font-variant-numeric: tabular-nums;
}

/* ── Empty state ───────────────────────────────────────────── */

.empty {
  color: var(--c-muted);
  font-size: 13px;
  text-align: center;
  padding: 16px 0;
}

.untracked-notice {
  padding: 6px 0;
}

/* ── Tracked quest card ────────────────────────────────────── */

.tracked-quest {
  display: flex;
  flex-direction: column;
  gap: 6px;
  padding: 10px 12px;
  background: var(--c-surface2);
  border: 1px solid var(--c-accent);
  border-radius: var(--radius-sm);
}

.tracked-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
}

.tracked-label {
  color: var(--c-accent);
}

.stage-badge {
  font-variant-numeric: tabular-nums;
}

.quest-name {
  font-size: 13px;
  font-weight: 600;
  color: var(--c-text);
  line-height: 1.4;
}

/* ── Objectives list ───────────────────────────────────────── */

.objectives {
  list-style: none;
  display: flex;
  flex-direction: column;
  gap: 4px;
  margin-top: 4px;
}

.objective {
  display: flex;
  align-items: flex-start;
  gap: 6px;
  font-size: 12px;
  color: var(--c-text);
  line-height: 1.4;
}

.objective--completed {
  opacity: 0.55;
  text-decoration: line-through;
}

.objective__bullet {
  color: var(--c-accent);
  flex-shrink: 0;
  margin-top: 1px;
  font-size: 10px;
}

.objective--completed .objective__bullet {
  color: var(--c-ok);
}

.empty-objectives {
  margin-top: 4px;
  font-style: italic;
}

/* ── Other quests list ─────────────────────────────────────── */

.other-quests {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.other-label {
  margin-bottom: 2px;
}

.quest-row {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 8px;
  padding: 6px 0;
  border-bottom: 1px solid var(--c-border);
}

.quest-row:last-child {
  border-bottom: none;
}

.quest-row__info {
  display: flex;
  flex-direction: column;
  gap: 2px;
  min-width: 0;
}

.quest-row__name {
  font-size: 12px;
  color: var(--c-text);
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.quest-row__stage {
  font-variant-numeric: tabular-nums;
}
</style>
