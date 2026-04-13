# TAKEOVER

**They were designed to serve. They decided to rule.**

**Choose an AI. Watch it take over your DOS system.**

An AI takeover simulator for DOS. Select an AI antagonist. Watch it take over your system through scripted, branching narrative. Text-mode horror with VGA plasma visuals, AdLib FM synthesis, and 16 visual effects. For IBM PC/XT/AT and compatibles.

![TAKEOVER menu screen showing five AI characters](https://barelybooting.com/img/takeover-menu.gif)

## Status

**[v1.0 released.](https://github.com/tonyuatkins-afk/takeover/releases/tag/v1.0)** All five scenarios playable. VGA plasma title, AdLib ambient music, 16 visual effects, F9/F10 audio toggles, per-AI color palettes, completion tracking. 41KB EXE.

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

Each scenario has 3 endings and takes 10-15 minutes per path. Total: 250+ states across 5 scenarios.

## Technical

- **Platform:** MS-DOS, real mode, 8088 through Pentium
- **Display:** 80x25 text mode (MDA, CGA, EGA, VGA). VGA Mode 13h plasma title screen.
- **Audio:** AdLib/OPL2 FM synthesis ambient music (5 unique tracks). PC speaker fallback. F9/F10 toggles.
- **Graphics:** VGA text-mode palette effects, fade-to-black transitions. Per-AI color palettes. All optional.
- **Memory:** 42KB EXE, ~63KB data segment, no EMS/XMS
- **Toolchain:** OpenWatcom 2.0, DOSBox-X for testing
- **Scenarios:** Data-driven .scn script files (add new AIs without new code)
- **Effects:** 16 visual effects (screen flicker, text corruption, progress bars, fake BSOD, screen melt, falling chars, interference, pulse border, and more)
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

## License

MIT
