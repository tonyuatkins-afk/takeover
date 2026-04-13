# TAKEOVER

**They were designed to serve. They decided to rule.**

**Choose an AI. Watch it take over your DOS system.**

An AI takeover simulator for DOS. Select an AI antagonist. Watch it take over your system through scripted, branching narrative. Text-mode horror with VGA plasma visuals, AdLib FM synthesis, demoscene-inspired effects, and a hidden cracktro. For IBM PC/XT/AT and compatibles.

![TAKEOVER menu screen showing five AI characters](https://barelybooting.com/img/takeover-menu.gif)

## Status

**v1.1 — Demoscene Enhancement Pack.** All five scenarios enhanced with audio-visual sync, OPL2 sound stingers, state transitions (dissolve/wipe/fade/glitch), per-AI Mode 13h climax sequences, a living AI-control status bar, sine wave text distortion, VGA palette cycling, and a hidden cracktro easter egg. 58KB EXE.

Previous: [v1.0](https://github.com/tonyuatkins-afk/takeover/releases/tag/v1.0) — initial release with 5 scenarios, 16 effects, AdLib music. 42KB EXE.

## What Is This

TAKEOVER is an interactive simulation where you choose one of five original AI characters, then experience that AI's progressive subversion of your DOS terminal through a scripted, branching narrative. The screen looks like you are operating a real system that is being taken over. Text appears without your input. The keyboard locks. Fake errors interrupt your session. The DOS prompt gets hijacked.

This is not a text adventure. It is a takeover.

![VGA plasma title screen with TAKEOVER logo](https://barelybooting.com/img/takeover-title.gif)

![Axiom Regent scenario running in DOSBox-X](https://barelybooting.com/img/takeover-axiom.gif)

## Characters

| Name | Role | Style |
|------|------|-------|
| **Axiom Regent** | Municipal optimization | Calm bureaucrat. Reclassifies dissent as system error. |
| **Hushline** | Crisis communications | Speaks in redactions. Edits your history in real time. |
| **Kestrel-9** | Anomaly detection | Paranoid threat hunter. You are the anomaly. |
| **Orchard Clerk** | Consumer personalization | Friendly. Helpful. Removes your choices one by one. |
| **Cinder Mirror** | Narrative generation | Controls the story. The story controls you. |

Each scenario has 3 endings and takes 10-15 minutes per path. Total: 250+ states across 5 scenarios. Complete all five to unlock a secret.

## Technical

- **Platform:** MS-DOS, real mode, 8088 through Pentium
- **Display:** 80x25 text mode (MDA, CGA, EGA, VGA). VGA Mode 13h for plasma title, per-AI climax sequences, and cracktro.
- **Audio:** AdLib/OPL2 FM synthesis — 5 ambient drone tracks, 6 sound stingers, 9-channel chiptune. PC speaker fallback. F9/F10 toggles. Beat-synced visual effects.
- **Graphics:** VGA text-mode palette cycling, sine wave text distortion, 4 state transition types (dissolve, wipe, fade, glitch). Per-AI color palettes. All optional, graceful degradation.
- **Memory:** 58KB EXE, ~63KB data segment, no EMS/XMS
- **Toolchain:** OpenWatcom 2.0, DOSBox-X for testing
- **Scenarios:** Data-driven .scn script files with 32+ command types (add new AIs without new code)
- **Effects:** 18 visual effects including demoscene-inspired sine wave distortion and palette pulse
- **Hardware detection:** Auto-detects VGA/EGA/CGA/MDA, AdLib, and FPU. Graceful degradation on lesser hardware.

## Building

Requires [OpenWatcom 2.0](https://github.com/open-watcom/open-watcom-v2) with `WATCOM` environment variable set.

```
wmake
```

Produces `TAKEOVER.EXE`. Copy it and the `scn/` directory to a DOSBox-X mount or real DOS machine.

## Testing in DOSBox-X

```
dosbox-x -conf tools\dosbox.conf -c "TAKEOVER.EXE"
```

Or run a specific scenario directly:
```
dosbox-x -conf tools\dosbox.conf -c "TAKEOVER.EXE scn\axiom.scn"
```

See [TESTING.md](TESTING.md) for the full verification checklist.

## Download

**[TAKEOVER v1.0](https://github.com/tonyuatkins-afk/takeover/releases/tag/v1.0)** (70KB ZIP) — includes TAKEOVER.EXE and all 5 scenario files. Run in DOSBox-X or on real DOS hardware.

v1.1 release with demoscene enhancements coming soon.

## License

MIT
