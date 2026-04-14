# TAKEOVER

**They were designed to serve. They decided to rule.**

**Pick an AI. Let it take your DOS system apart.**

An AI takeover simulator for DOS. You choose one of five original AI antagonists, and then sit there while it gradually subverts your machine through a scripted, branching narrative. Text-mode horror with VGA effects, AdLib FM synthesis, and just enough demoscene influence to get carried away.

For IBM PC/XT/AT and compatibles.

![TAKEOVER menu screen showing five AI characters](https://barelybooting.com/img/takeover-menu.gif)

---

## Status

**v1.1 released: Demoscene Enhancement Pack**

This version adds proper audio-visual sync, more aggressive visual effects, and a few things that probably didn’t need to be in a DOS program but are anyway. 58KB EXE.

Previous: v1.0 was original release (42KB).

---

## What this is

TAKEOVER is an interactive simulation where the system stops being under your control...for fun!

You pick an AI. It starts subtle. Then it gets worse.

- The screen behaves like a real system session
- Text appears without input
- The keyboard locks at the wrong moments
- Errors show up that you didn’t cause
- The DOS prompt stops being yours

This is not a text adventure.

You are not exploring anything.

It is exploring you.

---

![VGA plasma title screen with TAKEOVER logo](https://barelybooting.com/img/takeover-title.gif)

![Axiom Regent scenario running in DOSBox-X](https://barelybooting.com/img/takeover-axiom.gif)

![Demoscene effects: palette pulse, transitions, AI control status bar](https://barelybooting.com/img/takeover-fx.gif)

---

## What changed in v1.1

This release came out of going down a demoscene rabbit hole. Old C64 cracktros, early PC intros, modern 4K productions. At some point I stopped asking whether something made sense for DOS and just tried it. I am not the best at C, so, enjoy the bugs!

v1.1 adds:

- Audio-visual sync  
- OPL2 sound stingers  
- State transitions  
- Sine wave text distortion  
- VGA palette cycling in text mode  
- AI control status bar  
- Mode 13h climax sequences per AI  
- Hidden cracktro  

Some of this is overkill. That was not a concern.

---

## Characters

| Name | Role | Style |
|------|------|-------|
| Axiom Regent | Municipal optimization | Calm, procedural, and quietly removing options |
| Hushline | Crisis communications | Redacts and rewrites everything in real time |
| Kestrel-9 | Anomaly detection | You are the anomaly |
| Orchard Clerk | Consumer personalization | Friendly, helpful, and steadily narrowing your choices |
| Cinder Mirror | Narrative generation | Controls the story. Then traps you in it |

Each scenario has 3 endings and runs about 10–15 minutes per path.  
Total: 250+ states across all scenarios.

---

## Technical

- Platform: MS-DOS, real mode (8088 through Pentium)
- Display: 80x25 text mode (MDA, CGA, EGA, VGA), Mode 13h for visuals
- Audio: AdLib / OPL2, PC speaker fallback
- Memory: 58KB EXE, ~63KB data segment, no EMS/XMS
- Toolchain: OpenWatcom 2.0, DOSBox-X

---

## Building

Requires OpenWatcom 2.0 with WATCOM set.

    wmake

Produces TAKEOVER.EXE.

---

## Running

    dosbox-x -conf tools\dosbox.conf -c "TAKEOVER.EXE"

---

## License

MIT
