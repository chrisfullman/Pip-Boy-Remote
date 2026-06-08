<script setup lang="ts">
import { ref, computed } from 'vue'
import { useInventoryStore } from '@/stores/inventory'
import { useConnectionStore } from '@/stores/connection'
import { ItemCategory, CATEGORY_LABEL, type ItemCategoryValue } from '@/types'

const inv  = useInventoryStore()
const conn = useConnectionStore()

const activeCategory = ref<ItemCategoryValue | 'all'>('all')
const searchQuery    = ref('')

const TABS: { key: ItemCategoryValue | 'all'; label: string }[] = [
  { key: 'all',              label: 'All'     },
  { key: ItemCategory.Weapon, label: 'Weapons' },
  { key: ItemCategory.Armor,  label: 'Armor'   },
  { key: ItemCategory.Aid,    label: 'Aid'     },
  { key: ItemCategory.Ammo,   label: 'Ammo'    },
  { key: ItemCategory.Misc,   label: 'Misc'    },
]

const visibleItems = computed(() => {
  const q = searchQuery.value.trim().toLowerCase()
  return inv.items.filter((item) => {
    const categoryMatch = activeCategory.value === 'all' || item.category === activeCategory.value
    const nameMatch     = !q || item.name.toLowerCase().includes(q)
    return categoryMatch && nameMatch
  })
})

function equip(formID: number) {
  conn.sendAction({ action: 'equip', formID })
}

function unequip(formID: number) {
  conn.sendAction({ action: 'unequip', formID })
}

function consume(formID: number) {
  conn.sendAction({ action: 'consume', formID })
}
</script>

<template>
  <section class="surface inventory">
    <!-- Category tabs -->
    <div class="tabs">
      <button
        v-for="tab in TABS"
        :key="String(tab.key)"
        class="tab"
        :class="{ 'tab--active': activeCategory === tab.key }"
        @click="activeCategory = tab.key"
      >
        {{ tab.label }}
      </button>
    </div>

    <!-- Search -->
    <div class="search-row">
      <input
        v-model="searchQuery"
        class="input"
        style="flex: 1"
        placeholder="Search…"
        type="search"
      />
      <span class="label" style="white-space: nowrap">
        {{ visibleItems.length }} item{{ visibleItems.length !== 1 ? 's' : '' }}
      </span>
    </div>

    <!-- Item list -->
    <div v-if="inv.items.length === 0" class="empty">
      No inventory data received yet.
    </div>

    <div v-else-if="visibleItems.length === 0" class="empty">
      No items match "{{ searchQuery }}".
    </div>

    <ul v-else class="item-list">
      <li
        v-for="item in visibleItems"
        :key="item.formID"
        class="item"
        :class="{ 'item--equipped': item.isEquipped }"
      >
        <div class="item__name">
          {{ item.name }}
          <span v-if="item.isEquipped" class="badge">Equipped</span>
        </div>
        <div class="item__meta">
          <span class="label">{{ CATEGORY_LABEL[item.category] }}</span>
          <span class="value">×{{ item.count }}</span>
          <span class="label">{{ item.weight.toFixed(1) }} wt</span>
          <span class="label">{{ item.value }}c</span>
        </div>
        <div class="item__actions">
          <template v-if="item.category === ItemCategory.Aid">
            <button class="btn" :disabled="!conn.isConnected" @click="consume(item.formID)">
              Use
            </button>
          </template>
          <template v-else-if="item.category !== ItemCategory.Ammo && item.category !== ItemCategory.Misc">
            <button
              v-if="item.isEquipped"
              class="btn btn--danger"
              :disabled="!conn.isConnected"
              @click="unequip(item.formID)"
            >
              Unequip
            </button>
            <button
              v-else
              class="btn"
              :disabled="!conn.isConnected"
              @click="equip(item.formID)"
            >
              Equip
            </button>
          </template>
        </div>
      </li>
    </ul>
  </section>
</template>

<style scoped>
.inventory {
  display: flex;
  flex-direction: column;
  gap: 10px;
  padding: 0;
  overflow: hidden;
}

.tabs {
  padding: 0 4px;
  border-radius: var(--radius) var(--radius) 0 0;
}

.search-row {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 0 12px;
}

.empty {
  color: var(--c-muted);
  font-size: 13px;
  text-align: center;
  padding: 24px;
}

.item-list {
  list-style: none;
  overflow-y: auto;
  max-height: 400px;
  padding-bottom: 4px;
}

.item {
  display: flex;
  flex-direction: column;
  gap: 4px;
  padding: 8px 12px;
  border-top: 1px solid var(--c-border);
  transition: background 0.1s;
}

.item:hover {
  background: var(--c-surface2);
}

.item--equipped {
  border-left: 2px solid var(--c-accent);
}

.item__name {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 13px;
  font-weight: 500;
}

.item__meta {
  display: flex;
  gap: 10px;
  align-items: center;
}

.item__actions {
  display: flex;
  gap: 6px;
}

.badge {
  font-size: 10px;
  font-weight: 600;
  color: var(--c-accent);
  border: 1px solid var(--c-accent);
  border-radius: 3px;
  padding: 1px 5px;
  letter-spacing: 0.04em;
  text-transform: uppercase;
}
</style>
