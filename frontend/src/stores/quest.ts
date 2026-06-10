import { defineStore } from 'pinia'
import { computed, ref } from 'vue'
import type { QuestEntry } from '@/types'

export const useQuestStore = defineStore('quest', () => {
  /** All running (non-completed) quests reported by the backend. */
  const quests            = ref<QuestEntry[]>([])

  /** FormID of the quest currently tracked in the HUD / Pip-Boy; null if none. */
  const activeQuestFormID = ref<number | null>(null)

  /** The tracked quest entry, or null if no quest is tracked. */
  const trackedQuest = computed<QuestEntry | null>(() =>
    quests.value.find(q => q.formID === activeQuestFormID.value) ?? null
  )

  /** All quests that are not currently the tracked quest. */
  const untrackedQuests = computed<QuestEntry[]>(() =>
    quests.value.filter(q => q.formID !== activeQuestFormID.value)
  )

  function applyQuestUpdate(incoming: QuestEntry[], activeID?: number): void {
    quests.value            = incoming
    activeQuestFormID.value = activeID ?? null
  }

  return { quests, activeQuestFormID, trackedQuest, untrackedQuests, applyQuestUpdate }
})
