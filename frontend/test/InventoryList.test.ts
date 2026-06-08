import { describe, it, expect, beforeEach, vi } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia, setActivePinia } from 'pinia'
import InventoryList from '../src/components/InventoryList.vue'
import { useInventoryStore } from '../src/stores/inventory'
import { useConnectionStore } from '../src/stores/connection'
import { ItemCategory, type InventoryItem } from '../src/types'

const SAMPLE_ITEMS: InventoryItem[] = [
  { formID: 1, name: '10mm Pistol',  category: ItemCategory.Weapon, count: 1,  isEquipped: true,  weight: 4.0,  value: 75  },
  { formID: 2, name: 'Combat Rifle', category: ItemCategory.Weapon, count: 1,  isEquipped: false, weight: 13.0, value: 184 },
  { formID: 3, name: 'Leather Armor',category: ItemCategory.Armor,  count: 1,  isEquipped: true,  weight: 8.0,  value: 60  },
  { formID: 4, name: 'Stimpak',      category: ItemCategory.Aid,    count: 14, isEquipped: false, weight: 0.1,  value: 75  },
  { formID: 5, name: '10mm Round',   category: ItemCategory.Ammo,   count: 240,isEquipped: false, weight: 0.0,  value: 1   },
  { formID: 6, name: 'Desk Fan',     category: ItemCategory.Misc,   count: 2,  isEquipped: false, weight: 2.0,  value: 3   },
]

function mountList() {
  const pinia = createPinia()
  setActivePinia(pinia)
  return { wrapper: mount(InventoryList, { global: { plugins: [pinia] } }), pinia }
}

beforeEach(() => {
  setActivePinia(createPinia())
})

// ── Empty state ───────────────────────────────────────────────────────────────

describe('InventoryList empty state', () => {
  it('shows the empty-state message when there are no items', () => {
    const { wrapper } = mountList()
    expect(wrapper.text()).toContain('No inventory data received yet.')
    expect(wrapper.find('ul').exists()).toBe(false)
  })
})

// ── Item rendering ────────────────────────────────────────────────────────────

describe('InventoryList item rendering', () => {
  it('renders all items when the "All" tab is active', async () => {
    const { wrapper, pinia } = mountList()
    setActivePinia(pinia)
    useInventoryStore().applyUpdate(SAMPLE_ITEMS)
    await wrapper.vm.$nextTick()
    expect(wrapper.findAll('li.item')).toHaveLength(SAMPLE_ITEMS.length)
  })

  it('shows the "Equipped" badge on equipped items', async () => {
    const { wrapper, pinia } = mountList()
    setActivePinia(pinia)
    useInventoryStore().applyUpdate(SAMPLE_ITEMS)
    await wrapper.vm.$nextTick()
    const badges = wrapper.findAll('.badge')
    // 10mm Pistol and Leather Armor are equipped
    expect(badges).toHaveLength(2)
    expect(badges[0].text()).toBe('Equipped')
  })

  it('shows item count in the header label', async () => {
    const { wrapper, pinia } = mountList()
    setActivePinia(pinia)
    useInventoryStore().applyUpdate(SAMPLE_ITEMS)
    await wrapper.vm.$nextTick()
    expect(wrapper.text()).toContain(`${SAMPLE_ITEMS.length} items`)
  })

  it('shows "1 item" (singular) when only one item matches', async () => {
    const { wrapper, pinia } = mountList()
    setActivePinia(pinia)
    useInventoryStore().applyUpdate(SAMPLE_ITEMS)
    await wrapper.vm.$nextTick()
    // Click the Aid tab — only Stimpak matches
    const tabs = wrapper.findAll('.tab')
    const aidTab = tabs.find((t) => t.text() === 'Aid')!
    await aidTab.trigger('click')
    expect(wrapper.text()).toContain('1 item')
    expect(wrapper.text()).not.toContain('1 items')
  })
})

// ── Tab filtering ─────────────────────────────────────────────────────────────

describe('InventoryList tab filtering', () => {
  async function setupAndClickTab(tabLabel: string) {
    const { wrapper, pinia } = mountList()
    setActivePinia(pinia)
    useInventoryStore().applyUpdate(SAMPLE_ITEMS)
    await wrapper.vm.$nextTick()
    const tab = wrapper.findAll('.tab').find((t) => t.text() === tabLabel)!
    await tab.trigger('click')
    await wrapper.vm.$nextTick()
    return wrapper
  }

  it('filters to only weapons when the Weapons tab is clicked', async () => {
    const wrapper = await setupAndClickTab('Weapons')
    const items = wrapper.findAll('li.item')
    expect(items).toHaveLength(2)
    expect(wrapper.text()).toContain('10mm Pistol')
    expect(wrapper.text()).toContain('Combat Rifle')
    expect(wrapper.text()).not.toContain('Stimpak')
  })

  it('filters to only armor when the Armor tab is clicked', async () => {
    const wrapper = await setupAndClickTab('Armor')
    expect(wrapper.findAll('li.item')).toHaveLength(1)
    expect(wrapper.text()).toContain('Leather Armor')
  })

  it('filters to only aid items when the Aid tab is clicked', async () => {
    const wrapper = await setupAndClickTab('Aid')
    expect(wrapper.findAll('li.item')).toHaveLength(1)
    expect(wrapper.text()).toContain('Stimpak')
  })

  it('shows all items when All tab is clicked after another tab', async () => {
    const wrapper = await setupAndClickTab('Weapons')
    const allTab = wrapper.findAll('.tab').find((t) => t.text() === 'All')!
    await allTab.trigger('click')
    await wrapper.vm.$nextTick()
    expect(wrapper.findAll('li.item')).toHaveLength(SAMPLE_ITEMS.length)
  })
})

// ── Search filtering ──────────────────────────────────────────────────────────

describe('InventoryList search filtering', () => {
  it('filters items by name (case-insensitive)', async () => {
    const { wrapper, pinia } = mountList()
    setActivePinia(pinia)
    useInventoryStore().applyUpdate(SAMPLE_ITEMS)
    await wrapper.vm.$nextTick()
    await wrapper.find('input[type="search"]').setValue('stim')
    await wrapper.vm.$nextTick()
    expect(wrapper.findAll('li.item')).toHaveLength(1)
    expect(wrapper.text()).toContain('Stimpak')
  })

  it('shows the no-match message when search yields no results', async () => {
    const { wrapper, pinia } = mountList()
    setActivePinia(pinia)
    useInventoryStore().applyUpdate(SAMPLE_ITEMS)
    await wrapper.vm.$nextTick()
    await wrapper.find('input[type="search"]').setValue('zzznotfound')
    await wrapper.vm.$nextTick()
    expect(wrapper.find('ul').exists()).toBe(false)
    expect(wrapper.text()).toContain('No items match')
  })
})

// ── Action buttons ────────────────────────────────────────────────────────────

describe('InventoryList action buttons', () => {
  async function mountWithItems(connected = true) {
    const { wrapper, pinia } = mountList()
    setActivePinia(pinia)
    useInventoryStore().applyUpdate(SAMPLE_ITEMS)
    if (connected) {
      useConnectionStore().$patch({ status: 'connected' })
    }
    await wrapper.vm.$nextTick()
    return { wrapper, pinia }
  }

  it('shows Unequip for an equipped weapon', async () => {
    const { wrapper } = await mountWithItems()
    const items = wrapper.findAll('li.item')
    // First item is 10mm Pistol (equipped weapon)
    expect(items[0].text()).toContain('Unequip')
  })

  it('shows Equip for an unequipped weapon', async () => {
    const { wrapper } = await mountWithItems()
    const items = wrapper.findAll('li.item')
    // Second item is Combat Rifle (unequipped weapon)
    expect(items[1].text()).toContain('Equip')
  })

  it('shows Use for an Aid item', async () => {
    const { wrapper } = await mountWithItems()
    const items = wrapper.findAll('li.item')
    // Fourth item is Stimpak (Aid)
    expect(items[3].text()).toContain('Use')
  })

  it('shows no action button for Ammo items', async () => {
    const { wrapper } = await mountWithItems()
    const items = wrapper.findAll('li.item')
    // Fifth item is 10mm Round (Ammo)
    expect(items[4].find('button').exists()).toBe(false)
  })

  it('shows no action button for Misc items', async () => {
    const { wrapper } = await mountWithItems()
    const items = wrapper.findAll('li.item')
    // Sixth item is Desk Fan (Misc)
    expect(items[5].find('button').exists()).toBe(false)
  })

  it('disables action buttons when not connected', async () => {
    const { wrapper } = await mountWithItems(false)
    const buttons = wrapper.findAll('li.item button')
    for (const btn of buttons) {
      expect(btn.attributes('disabled')).toBeDefined()
    }
  })

  it('calls sendAction equip with the correct formID', async () => {
    const { wrapper, pinia } = await mountWithItems()
    setActivePinia(pinia)
    const conn = useConnectionStore()
    const spy = vi.spyOn(conn, 'sendAction').mockImplementation(() => {})

    const items = wrapper.findAll('li.item')
    // Second item is Combat Rifle (unequipped) — click its Equip button
    await items[1].find('button').trigger('click')
    expect(spy).toHaveBeenCalledWith({ action: 'equip', formID: 2 })
  })

  it('calls sendAction unequip with the correct formID', async () => {
    const { wrapper, pinia } = await mountWithItems()
    setActivePinia(pinia)
    const conn = useConnectionStore()
    const spy = vi.spyOn(conn, 'sendAction').mockImplementation(() => {})

    const items = wrapper.findAll('li.item')
    // First item is 10mm Pistol (equipped) — click its Unequip button
    await items[0].find('button').trigger('click')
    expect(spy).toHaveBeenCalledWith({ action: 'unequip', formID: 1 })
  })

  it('calls sendAction consume with the correct formID', async () => {
    const { wrapper, pinia } = await mountWithItems()
    setActivePinia(pinia)
    const conn = useConnectionStore()
    const spy = vi.spyOn(conn, 'sendAction').mockImplementation(() => {})

    const items = wrapper.findAll('li.item')
    // Fourth item is Stimpak — click its Use button
    await items[3].find('button').trigger('click')
    expect(spy).toHaveBeenCalledWith({ action: 'consume', formID: 4 })
  })
})
