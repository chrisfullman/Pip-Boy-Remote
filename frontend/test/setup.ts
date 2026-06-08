import { vi } from 'vitest'

// Node.js 22 exposes its own experimental localStorage that overrides jsdom's
// and is undefined unless --localstorage-file is provided.  Replace it with a
// simple in-memory mock so stores that use localStorage work in all tests.
const makeLocalStorageMock = () => {
  let store: Record<string, string> = {}
  return {
    getItem:    vi.fn((key: string): string | null => store[key] ?? null),
    setItem:    vi.fn((key: string, value: string): void => { store[key] = value }),
    removeItem: vi.fn((key: string): void => { delete store[key] }),
    clear:      vi.fn((): void => { store = {} }),
    get length() { return Object.keys(store).length },
    key:        vi.fn((index: number): string | null => Object.keys(store)[index] ?? null),
  }
}

vi.stubGlobal('localStorage', makeLocalStorageMock())
