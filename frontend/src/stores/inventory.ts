import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { InventoryItem, ItemCategoryValue } from '@/types'

export const useInventoryStore = defineStore('inventory', () => {
  const items = ref<InventoryItem[]>([])

  function applyUpdate(incoming: InventoryItem[]): void {
    items.value = incoming
  }

  function byCategory(category: ItemCategoryValue): InventoryItem[] {
    return items.value.filter((i) => i.category === category)
  }

  const equippedItems = computed(() => items.value.filter((i) => i.isEquipped))

  return { items, equippedItems, applyUpdate, byCategory }
})
