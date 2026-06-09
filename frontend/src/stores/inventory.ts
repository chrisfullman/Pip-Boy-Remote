import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { InventoryItem, ItemCategoryValue } from '@/types'

// Fallout 4 encodes metadata in item name strings that the player never sees
// in the normal Pip-Boy.  Strip them from the display name but preserve as
// structured fields so they can be used later (icons, scrapping UI, etc.).
//
// Known encodings:
//   [Tag]         — leading bracket tag, e.g. "[ToolHammer] Ball-Peen Hammer"
//   {{{Mat,...}}} — trailing triple-brace scrap list, e.g. "Hammer{{{Wood,Steel}}}"
function parseItemName(raw: string): Pick<InventoryItem, 'name' | 'iconTag' | 'scrapMaterials'> {
  let name = raw

  let iconTag: string | undefined
  const tagMatch = name.match(/^\[([^\]]+)\]\s*/)
  if (tagMatch) {
    iconTag = tagMatch[1]
    name = name.slice(tagMatch[0].length)
  }

  let scrapMaterials: string[] | undefined
  const scrapMatch = name.match(/\{{{([^}]+)}}}\s*$/)
  if (scrapMatch) {
    scrapMaterials = scrapMatch[1].split(',').map((s) => s.trim()).filter(Boolean)
    name = name.slice(0, name.length - scrapMatch[0].length).trimEnd()
  }

  return { name, iconTag, scrapMaterials }
}

export const useInventoryStore = defineStore('inventory', () => {
  const items = ref<InventoryItem[]>([])

  function applyUpdate(incoming: InventoryItem[]): void {
    // The backend emits one entry per inventory stack, so the same formID can
    // appear multiple times (e.g. one equipped copy + one unequipped copy of
    // the same weapon).  Merge them into a single entry: sum counts, mark
    // isEquipped if any stack is equipped.
    const merged = new Map<number, InventoryItem>()
    for (const item of incoming) {
      const existing = merged.get(item.formID)
      if (existing) {
        existing.count     += item.count
        existing.isEquipped = existing.isEquipped || item.isEquipped
      } else {
        merged.set(item.formID, { ...item, ...parseItemName(item.name) })
      }
    }
    items.value = Array.from(merged.values())
  }

  function byCategory(category: ItemCategoryValue): InventoryItem[] {
    return items.value.filter((i) => i.category === category)
  }

  const equippedItems = computed(() => items.value.filter((i) => i.isEquipped))

  return { items, equippedItems, applyUpdate, byCategory }
})
