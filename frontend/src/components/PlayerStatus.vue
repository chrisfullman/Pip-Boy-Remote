<script setup lang="ts">
import { computed } from 'vue'
import { usePlayerStore } from '@/stores/player'

const player = usePlayerStore()

const s = computed(() => player.state)

function pct(current: number, max: number): number {
  if (max <= 0) return 0
  return Math.min(100, Math.max(0, (current / max) * 100))
}

function hpClass(pctVal: number): string {
  if (pctVal <= 20) return 'meter__fill--danger'
  if (pctVal <= 40) return 'meter__fill--warn'
  return ''
}

function condColor(val: number): string {
  if (val <= 20) return 'var(--c-danger)'
  if (val <= 50) return 'var(--c-warn)'
  return 'var(--c-ok)'
}

const LIMBS = ['head', 'torso', 'leftArm', 'rightArm', 'leftLeg', 'rightLeg'] as const
const LIMB_LABEL: Record<typeof LIMBS[number], string> = {
  head: 'Head', torso: 'Torso', leftArm: 'L.Arm', rightArm: 'R.Arm', leftLeg: 'L.Leg', rightLeg: 'R.Leg',
}
</script>

<template>
  <section class="surface player-status">
    <div v-if="!s" class="empty">Waiting for game data…</div>

    <template v-else>
      <!-- Vitals -->
      <div class="row">
        <div class="stat">
          <div class="label">HP</div>
          <div class="value">{{ Math.round(s.health) }} / {{ Math.round(s.maxHealth) }}</div>
          <div class="meter" style="margin-top: 4px">
            <div
              class="meter__fill"
              :class="hpClass(pct(s.health, s.maxHealth))"
              :style="{ width: pct(s.health, s.maxHealth) + '%' }"
            />
          </div>
        </div>

        <div class="stat">
          <div class="label">AP</div>
          <div class="value">{{ Math.round(s.actionPoints) }} / {{ Math.round(s.maxActionPoints) }}</div>
          <div class="meter" style="margin-top: 4px">
            <div
              class="meter__fill"
              :style="{ width: pct(s.actionPoints, s.maxActionPoints) + '%' }"
            />
          </div>
        </div>

        <div class="stat">
          <div class="label">Weight</div>
          <div class="value">
            {{ s.currentWeight.toFixed(1) }} / {{ Math.round(s.maxCarryWeight) }}
          </div>
          <div class="meter" style="margin-top: 4px">
            <div
              class="meter__fill"
              :class="pct(s.currentWeight, s.maxCarryWeight) >= 100 ? 'meter__fill--danger' : ''"
              :style="{ width: pct(s.currentWeight, s.maxCarryWeight) + '%' }"
            />
          </div>
        </div>
      </div>

      <!-- Level / XP -->
      <div class="row row--xp" v-if="s.level !== undefined">
        <div class="stat">
          <span class="label">Level </span>
          <span class="value accent">{{ Math.round(s.level) }}</span>
        </div>
        <div class="stat stat--grow" v-if="s.experience !== undefined && s.nextLevelXP">
          <div class="label">
            XP {{ Math.round(s.experience - (s.levelStartXP ?? 0)) }} /
            {{ Math.round(s.nextLevelXP - (s.levelStartXP ?? 0)) }}
          </div>
          <div class="meter" style="margin-top: 4px">
            <div
              class="meter__fill"
              :style="{ width: pct(s.experience - (s.levelStartXP ?? 0), s.nextLevelXP - (s.levelStartXP ?? 0)) + '%' }"
            />
          </div>
        </div>
      </div>

      <!-- Limb conditions -->
      <div class="limbs">
        <div class="label" style="margin-bottom: 6px">Condition</div>
        <div class="limb-grid">
          <div
            v-for="limb in LIMBS"
            :key="limb"
            class="limb"
          >
            <div class="label">{{ LIMB_LABEL[limb] }}</div>
            <div
              class="value"
              :style="{ color: condColor(s.condition[limb]) }"
            >{{ Math.round(s.condition[limb]) }}</div>
          </div>
        </div>
      </div>

      <!-- Coordinates -->
      <div class="row">
        <div class="stat">
          <span class="label">Coords </span>
          <span class="value">
            {{ s.coords[0].toFixed(0) }}, {{ s.coords[1].toFixed(0) }}, {{ s.coords[2].toFixed(0) }}
          </span>
        </div>
      </div>
    </template>
  </section>
</template>

<style scoped>
.player-status {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.empty {
  color: var(--c-muted);
  font-size: 13px;
  text-align: center;
  padding: 24px 0;
}

.row {
  display: flex;
  flex-wrap: wrap;
  gap: 20px;
}

.row--xp {
  align-items: center;
}

.stat {
  min-width: 140px;
}

.stat--grow {
  flex: 1;
}

.accent {
  color: var(--c-accent);
  font-family: var(--font-mono);
  font-size: 18px;
  font-weight: 700;
}

.limb-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 8px 16px;
}

.limb {
  display: flex;
  align-items: center;
  gap: 8px;
}
</style>
