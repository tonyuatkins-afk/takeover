# Testing TAKEOVER in DOSBox-X

## Quick Start

1. Build: `wmake` (requires OpenWatcom 2.0)
2. Copy TAKEOVER.EXE and the scenarios/ directory to a DOSBox-X mount
3. Run: `TAKEOVER.EXE`
4. Select Axiom Regent from the menu
5. Play through the scenario

## Command Line Testing

Run a specific scenario directly (bypasses menu):
```
TAKEOVER.EXE scenarios\test_engine.scn
```

## DOSBox-X Settings

Recommended dosbox-x.conf settings:
```
[cpu]
cycles=3000

[speaker]
pcspeaker=true

[dosbox]
machine=svga_s3
```

## What to Verify

- [ ] Menu renders correctly with all 5 characters listed
- [ ] Arrow key navigation highlights characters and shows descriptions
- [ ] Esc exits cleanly to DOS
- [ ] Axiom Regent scenario loads and runs
- [ ] Text appears with typing effect
- [ ] PC speaker tones are audible
- [ ] Choices work (arrow keys + Enter)
- [ ] Effects fire correctly (screen_flicker, progress_bar, etc.)
- [ ] Scenario reaches an ending
- [ ] Completion marker appears on menu after finishing
- [ ] TAKEOVER.DAT is created after first completion
- [ ] Re-running shows completion marker preserved
- [ ] Command-line override still works with test_engine.scn
