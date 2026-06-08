import { describe, it, expect, beforeEach, vi } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia, setActivePinia } from 'pinia'
import ConnectionBanner from '../src/components/ConnectionBanner.vue'
import { useConnectionStore } from '../src/stores/connection'
import { usePreferencesStore } from '../src/stores/preferences'

// One pinia instance per test; shared between mount and store access so they
// operate on the same store instances.
let pinia: ReturnType<typeof createPinia>

beforeEach(() => {
  pinia = createPinia()
  setActivePinia(pinia)
})

function mountBanner() {
  return mount(ConnectionBanner, { global: { plugins: [pinia] } })
}

// ── Status display ────────────────────────────────────────────────────────────

describe('ConnectionBanner status display', () => {
  it('shows Disconnected label by default', () => {
    const wrapper = mountBanner()
    expect(wrapper.text()).toContain('Disconnected')
  })

  it('shows Connecting… label when status is connecting', async () => {
    const wrapper = mountBanner()
    useConnectionStore().$patch({ status: 'connecting' })
    await wrapper.vm.$nextTick()
    expect(wrapper.text()).toContain('Connecting…')
  })

  it('shows Connected label when status is connected', async () => {
    const wrapper = mountBanner()
    useConnectionStore().$patch({ status: 'connected' })
    await wrapper.vm.$nextTick()
    expect(wrapper.text()).toContain('Connected')
  })

  it('displays the last error message when present', async () => {
    const wrapper = mountBanner()
    useConnectionStore().$patch({ lastError: 'Cannot reach 127.0.0.1:11104' })
    await wrapper.vm.$nextTick()
    expect(wrapper.text()).toContain('Cannot reach 127.0.0.1:11104')
  })

  it('hides the error element when lastError is null', () => {
    const wrapper = mountBanner()
    expect(wrapper.find('.banner__error').exists()).toBe(false)
  })
})

// ── Input state ───────────────────────────────────────────────────────────────

describe('ConnectionBanner input state', () => {
  it('enables host and port inputs when disconnected', () => {
    const wrapper = mountBanner()
    expect(wrapper.find('#ws-host').attributes('disabled')).toBeUndefined()
    expect(wrapper.find('#ws-port').attributes('disabled')).toBeUndefined()
  })

  it('disables inputs when connecting', async () => {
    const wrapper = mountBanner()
    useConnectionStore().$patch({ status: 'connecting' })
    await wrapper.vm.$nextTick()
    expect(wrapper.find('#ws-host').attributes('disabled')).toBeDefined()
    expect(wrapper.find('#ws-port').attributes('disabled')).toBeDefined()
  })

  it('disables inputs when connected', async () => {
    const wrapper = mountBanner()
    useConnectionStore().$patch({ status: 'connected' })
    await wrapper.vm.$nextTick()
    expect(wrapper.find('#ws-host').attributes('disabled')).toBeDefined()
    expect(wrapper.find('#ws-port').attributes('disabled')).toBeDefined()
  })

  it('pre-fills host and port from preferences store', () => {
    // Set preferences BEFORE mounting so the component reads the updated values.
    const prefs = usePreferencesStore()
    prefs.host = '192.168.1.50'
    prefs.port = 9999
    const wrapper = mountBanner()
    expect((wrapper.find('#ws-host').element as HTMLInputElement).value).toBe('192.168.1.50')
    expect((wrapper.find('#ws-port').element as HTMLInputElement).value).toBe('9999')
  })
})

// ── Button rendering ──────────────────────────────────────────────────────────

describe('ConnectionBanner button rendering', () => {
  it('shows the Connect button when disconnected', () => {
    const wrapper = mountBanner()
    expect(wrapper.find('button[type="submit"]').text()).toBe('Connect')
    expect(wrapper.find('button[type="button"]').exists()).toBe(false)
  })

  it('shows the Disconnect button (and no Connect button) when connected', async () => {
    const wrapper = mountBanner()
    useConnectionStore().$patch({ status: 'connected' })
    await wrapper.vm.$nextTick()
    expect(wrapper.find('button[type="button"]').text()).toBe('Disconnect')
    expect(wrapper.find('button[type="submit"]').exists()).toBe(false)
  })

  it('shows the Disconnect button when connecting', async () => {
    const wrapper = mountBanner()
    useConnectionStore().$patch({ status: 'connecting' })
    await wrapper.vm.$nextTick()
    expect(wrapper.find('button[type="button"]').text()).toBe('Disconnect')
  })
})

// ── User interactions ─────────────────────────────────────────────────────────

describe('ConnectionBanner user interactions', () => {
  it('calls conn.connect() with entered host and port on form submit', async () => {
    const wrapper = mountBanner()
    const conn = useConnectionStore()
    const connectSpy = vi.spyOn(conn, 'connect').mockImplementation(() => {})

    await wrapper.find('#ws-host').setValue('10.0.0.1')
    await wrapper.find('#ws-port').setValue(12345)
    await wrapper.find('form').trigger('submit')

    expect(connectSpy).toHaveBeenCalledOnce()
    expect(connectSpy).toHaveBeenCalledWith('10.0.0.1', 12345)
  })

  it('calls conn.disconnect() when the Disconnect button is clicked', async () => {
    const wrapper = mountBanner()
    const conn = useConnectionStore()
    conn.$patch({ status: 'connected' })
    await wrapper.vm.$nextTick()

    const disconnectSpy = vi.spyOn(conn, 'disconnect').mockImplementation(() => {})
    await wrapper.find('button[type="button"]').trigger('click')
    expect(disconnectSpy).toHaveBeenCalledOnce()
  })
})
