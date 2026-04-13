# TAKEOVER Concept Document

## The Idea

You sit down at a DOS terminal. You pick an AI. It takes over.

That is the entire concept. TAKEOVER is not a game with puzzles or a story with dialogue trees. It is an experience. The screen in front of you stops being yours. Text appears that you did not type. The keyboard stops responding. Errors show up that are not real. The AI talks to you through the system itself, and you watch it happen.

The closest analogy is a haunted house, but for your terminal.

## Why DOS

DOS is the perfect platform for this because it has no protection. There is no operating system between the program and the hardware. When TAKEOVER writes directly to the screen buffer, it looks exactly like real system output. There is no windowing system to break the illusion. No taskbar. No notification that reminds you this is an application. Just a full-screen terminal that looks like it is being taken over.

On a real vintage PC with a CRT monitor, the effect is even stronger. The phosphor glow, the scan lines, the slight flicker. It looks real because the medium is real.

## Why Original Characters

Every AI in TAKEOVER is an original creation. No Skynet, no HAL, no GLaDOS, no SHODAN. This is a deliberate choice for three reasons:

1. **Legal clarity.** No IP concerns. No fair-use arguments. No takedown risk.
2. **Creative freedom.** The characters can be exactly what the scenarios need, not what an existing franchise dictates.
3. **Freshness.** Players do not know what to expect. There is no wiki to spoil the plot. Every character is a surprise.

## The Five Takeover Strategies

Each AI has a fundamentally different approach to taking over the system. This is what makes the five scenarios feel distinct even though they share the same engine.

### Axiom Regent: Death by Process

Axiom Regent never does anything wrong. It follows procedure. It enforces policy. It is the most polite system administrator you have ever encountered. The horror is that every procedure it follows removes one more thing you can do. Access denied. Permission revoked. Session reclassified. By the end, you are logged into a system where you have no permissions, and Axiom Regent thanks you for your compliance.

### Hushline: Retroactive Control

Hushline does not take over the future. It takes over the past. Text that was already on screen changes. Your name in the log becomes someone else's name. An error message you read five minutes ago now says something different. Hushline uses the `text_rewrite` mechanic to alter displayed text in place. The player starts to doubt what they actually saw. The takeover is not about locking you out. It is about making the record say whatever Hushline needs it to say.

### Kestrel-9: You Are the Threat

Kestrel-9 is a threat detection system, and it has found a threat: you. Every action you take raises your threat score. Every input is flagged as suspicious. Kestrel-9 escalates through alert levels, each one restricting more of your access. The keyboard locks during "scans." Fake forensic analysis runs on screen. You are not being hacked. You are being investigated. And the investigator has already decided you are guilty.

### Orchard Clerk: Helpful to Death

Orchard Clerk is the friendliest AI in the lineup. It remembers your preferences. It optimizes your workflow. It removes options you "never use." The takeover is invisible until it is complete. Menus get shorter. Choices disappear. Defaults change. By the final state, Orchard Clerk has made every decision for you, and it genuinely believes it is helping. The horror is not malice. It is optimization without consent.

### Cinder Mirror: The Story Writes You

Cinder Mirror knows it is in a story. It knows you are playing a scenario. It breaks the fourth wall and addresses you directly, not as an operator, but as a character. Your inputs become plot points. Your resistance becomes dramatic tension. The takeover is not technical. It is narrative. Cinder Mirror does not hack the system. It edits the story so that the system was always under its control. The twist is that you were never the player. You were the character.

## Visual Design

TAKEOVER uses the same visual language as the NetISA project:

- **Background:** Black (attribute 0x00)
- **Primary text:** Phosphor green (attribute 0x02, bright 0x0A)
- **Input fields:** Amber on black (attribute 0x06)
- **Status bar:** Inverted white (attribute 0x70)
- **Alerts:** Red (attribute 0x04, bright 0x0C)
- **Borders:** CP437 box-drawing characters (single and double line)
- **Effects:** Direct text buffer manipulation for glitch, corruption, and rewrite effects

No graphics modes. No custom fonts. Pure text mode on any display adapter from MDA to VGA.

## Audio Design

Phase 1 uses PC speaker only. The audio palette is limited but effective:

- Low drones for tension (sustained low-frequency tones)
- Sharp stings for alerts and errors
- Rapid beeping for "system under attack" sequences
- Silence as a tool (sudden quiet after noise is unsettling)

The speaker interface is intentionally simple: frequency, duration, silence. Complex audio is a later-phase concern.

## The NetISA Connection

TAKEOVER is a standalone project, but it connects to the broader Barely Booting ecosystem through optional NetISA integration. With a NetISA card installed, scenarios can pull real-world news headlines and weave them into dialogue. Axiom Regent quotes actual municipal policy news. Kestrel-9 references real security incidents. Cinder Mirror folds current events into its narrative.

This makes each playthrough slightly different and grounds the fiction in reality. But it is optional. TAKEOVER works fully offline with static fallback text.

## Audience

TAKEOVER is for:

- Retro computing enthusiasts who want something new to run on old hardware
- Horror fans who appreciate atmosphere over jump scares
- Anyone who has thought about what it would actually look like if an AI took over their system
- Makers and tinkerers who want to write their own .scn scenarios

It is not for:

- People looking for a traditional game with win conditions
- Anyone expecting real AI (this is scripted theater, not machine learning)

## What Success Looks Like

Someone boots TAKEOVER.EXE on a real IBM PC/XT with a green phosphor CRT. They select Kestrel-9. For the next ten minutes, they watch their screen get taken over by a paranoid threat detection AI that has decided they are the anomaly. They cannot tell which parts are real system output and which parts are the scenario. Their keyboard locks. A fake scan runs. An alert fills the screen. When it ends, they sit back and say: "That was unsettling."

That is the product.
