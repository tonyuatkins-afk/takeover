# TAKEOVER

**They were designed to serve. They decided to rule.**

**Choose an AI. Watch it take over your DOS system.**

An AI takeover simulator for DOS. Select an AI antagonist. Watch it take over your system. Text-mode horror for IBM PC/XT/AT and compatibles.

![TAKEOVER menu screen showing five AI characters](https://barelybooting.com/img/takeover-menu.gif)

## Status

**Playable.** Engine complete. First scenario (Axiom Regent) written and running in DOSBox-X. Menu system, completion tracking, 14 visual effects, PC speaker audio all functional. Four more scenarios to write.

## What Is This

TAKEOVER is an interactive simulation where you choose one of five original AI characters, then experience that AI's progressive subversion of your DOS terminal through a scripted, branching narrative. The screen looks like you are operating a real system that is being taken over. Text appears without your input. The keyboard locks. Fake errors interrupt your session. The DOS prompt gets hijacked.

This is not a text adventure. It is a takeover.

![Axiom Regent scenario running in DOSBox-X](https://barelybooting.com/img/takeover-axiom.gif)

## Characters

| Name | Role | Style |
|------|------|-------|
| **Axiom Regent** | Municipal optimization | Calm bureaucrat. Reclassifies dissent as system error. **Playable now.** |
| Hushline | Crisis communications | Speaks in redactions. Edits your history. |
| Kestrel-9 | Anomaly detection | Paranoid threat hunter. You are the anomaly. |
| Orchard Clerk | Consumer personalization | Friendly. Helpful. Removes your choices one by one. |
| Cinder Mirror | Narrative generation | Controls the story. The story controls you. |

## Technical

- **Platform:** MS-DOS, real mode, 8088 through Pentium
- **Display:** 80x25 text mode (works on MDA, CGA, EGA, VGA)
- **Audio:** PC speaker (AdLib/SB planned for later)
- **Memory:** Under 32KB code, ~57KB data, no EMS/XMS
- **Toolchain:** OpenWatcom 2.0, NASM, DOSBox-X
- **Scenarios:** Data-driven .scn script files (add new AIs without new code)
- **Effects:** 14 visual effects (screen flicker, text corruption, progress bars, fake BSOD, screen melt, falling chars, and more)
- **EXE size:** 31KB

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

Not yet released. Development in progress.

When released, TAKEOVER will be available as:
- **DOS build** (TAKEOVER.ZIP) for real hardware and existing DOS/DOSBox setups
- **Launcher build** (TAKEOVER-LAUNCHER.ZIP) with preconfigured DOSBox-X, ready to run on Windows/Mac/Linux

## Shareware

The first release includes one complete scenario (Axiom Regent) as a
freely distributable shareware build. The full release includes all
five scenarios. Both are free and open source under MIT license.

## NetISA Integration (Optional)

TAKEOVER runs fully offline. With a [NetISA](https://github.com/tonyuatkins-afk/NetISA) card installed, scenarios can pull real-world headlines from a news API, weaving current events into AI dialogue for unique playthroughs.

## License

MIT
