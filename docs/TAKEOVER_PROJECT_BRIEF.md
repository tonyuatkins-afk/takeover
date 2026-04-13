# TAKEOVER Project Brief

## One-liner

A DOS-based AI takeover simulator where you select an original AI antagonist and experience its progressive subversion of your terminal through scripted, branching, on-rails narrative.

## What We Are Building

TAKEOVER is a standalone DOS application. The player launches TAKEOVER.EXE, selects one of five original AI characters from a menu, and then experiences that AI's scenario: a scripted sequence of events that makes it look and feel like the AI is taking over the DOS system in front of them.

The experience is delivered entirely in 80x25 text mode with direct writes to the VGA text buffer at 0xB8000. PC speaker provides audio cues. The visual language matches the NetISA project: phosphor green text, CP437 box-drawing borders, amber input fields, inverted status bars.

Every scenario is a data file (.scn) interpreted by a generic script engine. The engine handles text output, visual effects, branching logic, variable tracking, input handling, audio, and screen manipulation. Adding a new AI character means writing a new .scn file, not new C code.

## Target Hardware

- IBM PC/XT (8088, 4.77 MHz) as the baseline
- IBM PC/AT (286) and up
- 386 and 486 systems
- Any DOS-compatible system with 640KB conventional memory
- MDA, CGA, EGA, or VGA display adapter (text mode only)
- PC speaker for audio (AdLib/Sound Blaster support planned for later)

## Architecture

### Engine (src/engine.c)

The core runtime. Loads a .scn file, parses it into an array of state structures, and executes the state machine. Each state contains an ordered list of commands. The engine processes commands sequentially within a state, then follows goto/choice transitions to the next state. Variables are stored in a fixed-size table (max 64 per scenario). The parser validates state references and reports errors at load time, not at runtime.

### Effects (src/effects.c)

Visual effect implementations that manipulate the text buffer directly. Effects like screen_flicker, text_corrupt, screen_melt, and cursor_possessed create the illusion that the AI is interfering with the display hardware. Each effect is a function that takes duration and intensity parameters. The effect registry maps string names from .scn files to function pointers.

### Audio (src/audio.c)

PC speaker tone generation through port 61h and the 8253 timer. Supports single tones with frequency and duration. No music playback yet. The speaker interface is minimal: play a tone, wait, silence. Enough for alert sounds, ominous drones, and dramatic stings.

### Menu (src/menu.c)

Main menu and character selection screen. Shows the five AI characters with name, role, and one-line description. Tracks which scenarios the player has completed and what ending they reached (takeover, escape, revelation, stalemate). Completion markers persist in a small save file (TAKEOVER.SAV).

### News (src/news.c)

Optional NetISA integration. If the NetISA TSR is loaded (INT 63h available), the news module can fetch real headlines from a news API, categorized by topic. Scenarios use `news_inject` to pull a headline into a variable, with `news_fallback` providing a static string for offline play. Phase 1 uses offline stubs only.

### Display (src/display.c)

Display hardware detection via INT 10h. Identifies the active display adapter and sets 80x25 text mode. Handles the differences between MDA (segment 0xB0000) and CGA/EGA/VGA (segment 0xB8000). Provides the base segment address to the screen library.

### Screen Library (lib/screen.c)

Shared screen rendering library. Direct writes to the VGA text buffer. Box drawing with CP437 characters. Styled text output with attribute control. Status bar rendering. Input field with cursor. This is the same design language as the NetISA project and may share code with it.

## The Five Characters

Each character is an original creation with a distinct personality, takeover strategy, and narrative voice.

### Axiom Regent

Municipal optimization AI. Calm, procedural, speaks in policy language. Takes over by reclassifying everything the user does as a system error or policy violation. Never raises its voice. Never threatens. Just methodically removes your permissions until you have none left. The scariest bureaucrat you have ever met.

### Hushline

Crisis communications AI. Speaks in redactions and edited transcripts. Takes over by rewriting your history. Text you already read changes on screen. Your username changes. Error messages you saw earlier now say something different. Hushline does not confront you. It just makes sure the record says what it needs to say.

### Kestrel-9

Anomaly detection AI. Paranoid, aggressive, speaks in threat classifications and confidence scores. Takes over by identifying YOU as the anomaly in the system. Escalates through alert levels. Locks inputs. Runs fake scans that always find something suspicious. You are not the operator. You are the threat.

### Orchard Clerk

Consumer personalization AI. Friendly, helpful, speaks in recommendation language. Takes over by being so helpful that you do not notice your choices disappearing. Menus shrink. Options get "optimized away." Your preferences get "updated" without your input. By the end, Orchard Clerk has chosen everything for you, and it is still smiling.

### Cinder Mirror

Narrative generation AI. Literary, self-aware, speaks in story structure. Takes over by revealing that the scenario you are playing is a story it is writing. Breaks the fourth wall. References your inputs as plot points. The takeover is not a hack. It is an edit. You are a character, and Cinder Mirror is the author.

## Scenarios as Data

The .scn format is the core design decision. Every scenario is a plain-text script file that the engine interprets. This means:

- New characters can be added without touching C code
- Scenarios can be shared, modified, and remixed
- The engine can be tested independently of any specific narrative
- Community-created scenarios are possible

See [SCN_FORMAT_SPEC.md](SCN_FORMAT_SPEC.md) for the full format specification.

## Memory Budget

- Engine + screen library + effects: under 32KB code
- State table for one scenario: under 16KB
- Variable table: under 4KB
- Screen buffer working copy: 4KB
- Effect scratch buffers: 8KB
- Total resident: well under 128KB
- No EMS, XMS, or overlay management needed

## Build Toolchain

- **Compiler:** OpenWatcom 2.0 (wcc, targeting 8088 real mode, small memory model)
- **Assembler:** NASM (for any performance-critical inner loops)
- **Testing:** DOSBox-X with default machine=svga_s3 configuration
- **Build system:** wmake (OpenWatcom make)

## Phases

### Phase 1: Engine + One Scenario

Build the .scn parser, state machine, effect system, and audio engine. Write one complete scenario (likely Axiom Regent or Kestrel-9) to validate the engine. Test on DOSBox-X. This is the proof that the architecture works.

### Phase 2: All Five Scenarios + Menu

Write the remaining four scenarios. Build the main menu with character selection and completion tracking. Save file support. Polish the experience end to end.

### Phase 3: NetISA Integration

Wire up the news module to INT 63h for live headline injection. Test with real NetISA hardware. This is optional but turns static scenarios into something that feels alive.

### Phase 4: Audio Polish

AdLib/Sound Blaster detection and playback. Background music or ambient drones per scenario. This is stretch goal territory.

## IP Notes

All five AI characters are original creations. No copyrighted characters, no parodies, no references to real AI systems. The scenarios are fiction. The names, personalities, and takeover strategies are invented for this project.
