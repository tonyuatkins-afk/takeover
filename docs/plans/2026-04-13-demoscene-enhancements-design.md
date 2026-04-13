---
title: "Demoscene Enhancement Pack"
date: "2026-04-13"
source: "design"
---

# TAKEOVER Demoscene Enhancement Pack

## Summary

Eight demoscene-inspired enhancements to transform TAKEOVER from a DOS
text-mode game into an audiovisual experience that pays homage to four
decades of demoscene culture. All features respect the existing constraints:
8088 target, small memory model, C89, ~64KB DGROUP limit.

## Features Implemented

### 1. Audio-Visual Sync (adlib.c)
- `adlib_beat` counter increments on every OPL2 note-on event
- `adlib_last_ch` tracks which channel fired
- Effects can poll `adlib_beat` to sync visuals to music
- DGROUP cost: 2 bytes

### 2. OPL2 Sound Stingers (adlib.c)
- 6 fire-and-forget stinger patches on channels 3-8
- alarm, stamp, click, buzz, ominous chord, cash register
- Multi-note sequences (up to 3 notes) and chords
- New `stinger:` engine command
- DGROUP cost: ~38 bytes (playback state)

### 3. New Visual Effects (effects.c)
- `fx_sine_wave`: text characters ripple horizontally via sine offsets
- `fx_palette_pulse`: VGA DAC color cycling in text mode (VGA-only)
- Both registered in engine effect dispatch table

### 4. State Transitions (engine.c)
- 4 transition types: dissolve (LFSR), wipe, fade, glitch
- Syntax: `goto: state_name dissolve`
- `transition_default:` command sets per-scenario default
- 4KB far back-buffer for screen swap
- DGROUP cost: ~8 bytes

### 5. Living Status Bar (main.c, engine.c)
- Global `g_ai_control` (0-100) tracks AI control level
- Progress bar with block characters
- Color shifts green -> yellow -> red
- `control:` engine command to increment/decrement

### 6. Mode 13h Climax Sequences (climax.c)
- Shared half-res (160x100 doubled) framework
- 5 per-AI procedural effects:
  - Axiom: Grid Convergence (surveillance grid from noise)
  - Hushline: Redaction Sweep (document lines blacked out)
  - Kestrel-9: Threat Radar (rotating sweep + hotspots)
  - Orchard: Cozy Dissolution (warm plasma + closing iris)
  - Cinder: Reality Glitch (warped plasma + palette rotation)
- `climax:` engine command
- Runtime palette generation (768 bytes far BSS)

### 7. Cracktro Easter Egg (cracktro.c)
- Raster bars + TAKEOVER logo (top third)
- 40-star parallax starfield (middle)
- DYCP sine-bouncing scrolltext with 64-char bitmap font
- 9-channel OPL2 chiptune (C minor, 10-bar loop at 120 BPM)
- Triggered on completing all 5 scenarios (auto first time)
- Replayable via hidden 'C' key at menu

### 8. New Engine Commands
- `stinger: alarm|stamp|click|buzz|ominous|register`
- `climax: axiom|hushline|kestrel|orchard|cinder`
- `sync_beat: [timeout_ms]`
- `control: [+/-delta]`
- `transition_default: dissolve|wipe|fade|glitch|none`
- `goto: state_name [dissolve|wipe|fade|glitch]`

## Binary Impact

| Metric | v1.0 | v1.1 (this) | Delta |
|--------|------|-------------|-------|
| EXE size | 41.8 KB | 57.8 KB | +16 KB |
| New DGROUP | - | ~55 bytes | minimal |
| New files | 0 | 4 (climax.c/h, cracktro.c/h) | |
| Modified files | 0 | 7 | |

## New Scenario Commands Reference

```
stinger: alarm          # fire OPL2 stinger
climax: axiom           # Mode 13h climax sequence
sync_beat: 3000         # wait for next music beat (3s timeout)
control: +10            # increase AI control level by 10
transition_default: dissolve   # set default transition
goto: next_state fade   # goto with fade transition
effect: sine_wave 2000 3       # sine wave text distortion
effect: palette_pulse 3000 5   # VGA palette cycling
```
