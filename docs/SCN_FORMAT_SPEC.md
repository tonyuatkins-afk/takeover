# .scn Format Specification

Version: 0.1 (draft)
Status: Pre-implementation. Subject to change during engine development.

## Overview

A .scn file defines a single AI scenario as a sequence of named states. The script engine loads one .scn file at a time, parses it into an in-memory state graph, and executes it as a state machine. Execution begins at the state named `init`.

## File Format

- Plain ASCII text, LF or CRLF line endings
- Maximum line length: 160 characters
- Lines beginning with `#` are comments (ignored by parser)
- Blank lines are ignored
- All commands are lowercase
- String values are enclosed in double quotes
- Numeric values are bare integers
- Variable names are alphanumeric plus underscore, max 32 characters

## States

A state begins with a header line and contains one or more commands:

```
[state: state_name]
command: argument
command: argument
```

State names: lowercase alphanumeric plus underscore, max 32 characters.
Every .scn file MUST define a state named `init`. Execution starts there.
A state with no `goto`, `choice`, or `end` command at the end is an error.

## Commands

### Text Output

```
text: "string to display"
```
Writes a line of text to the main display area. Supports variable
substitution with `{variable_name}` syntax. Text wraps at column 78
(inside the border). Consecutive `text` commands append on new lines.

### Attributed Text

```
attr: attribute_name
```
Sets the text attribute for subsequent `text` commands until changed.
Valid attributes: `green`, `bright_green`, `amber`, `red`, `bright_red`,
`white`, `bright_white`, `grey`, `cyan`, `bright_cyan`, `inverse`.
Default attribute: `green`.

### Delay

```
delay: milliseconds
```
Pauses execution for the specified duration. Uses BIOS tick counter
(INT 1Ah) for hardware-independent timing. Minimum: 55ms (one tick).
Maximum: 60000ms.

### Clear

```
clear: region
```
Clears a screen region. Valid regions:
- `screen` - entire 80x25 display
- `main` - main content area (rows 2-22, inside borders)
- `status` - status bar (row 24)

### Input

```
input: "prompt string" -> variable_name
```
Displays the prompt, waits for user text input (Enter to confirm),
stores the result in the named variable. Input field renders in amber.
Max input length: 40 characters.

### Choice

```
choice: "label" -> target_state
```
Presents a selectable option. Multiple `choice` commands in sequence
create a choice menu. Player navigates with arrow keys, confirms with
Enter. After any `choice` command is selected, execution jumps to
the target state. A state with `choice` commands MUST NOT also have
a `goto` (the choice IS the branching mechanism).

### Goto

```
goto: target_state
```
Unconditional jump to another state. Must be the last command in
a state (unless the state uses `choice` instead).

### End

```
end: result_type
```
Ends the scenario and returns to the main menu. Valid result types:
- `takeover` - AI wins, system fully compromised
- `escape` - player found a way out
- `revelation` - twist ending (takeover already happened)
- `stalemate` - neither side wins

The result type is stored for the completion marker on the menu.

### Variables

```
set: variable_name = "string value"
set: variable_name = number
```
Sets a variable. Variables are scenario-scoped (reset between runs).

```
if: variable_name == "value" -> target_state
if: variable_name != "value" -> target_state
if: variable_name > number -> target_state
if: variable_name < number -> target_state
```
Conditional branch. If the condition is true, jump to target_state.
If false, continue to the next command. Only one comparison per `if`.

```
inc: variable_name
dec: variable_name
```
Increment or decrement a numeric variable by 1.

### Visual Effects

```
effect: effect_name
effect: effect_name duration_ms
effect: effect_name duration_ms intensity
```
Triggers a visual effect. Duration defaults to 500ms if not specified.
Intensity is effect-specific (typically 1-10, defaults to 5).

Available effects:

| Name | Description | Intensity controls |
|------|-------------|--------------------|
| typing_effect | Next `text` command renders char-by-char | chars per second |
| screen_flicker | Random character flash across screen | frequency |
| static_burst | Random chars fill a region briefly | coverage area |
| text_corrupt | Random char substitution in displayed text | corruption rate |
| text_redact | Replace displayed text with block chars | redaction speed |
| text_rewrite | Overwrite tagged text with new content | n/a (see below) |
| color_shift | Gradually change text attributes | shift speed |
| blackout | Screen goes black | n/a |
| red_alert | Border flashes red | flash rate |
| screen_scroll | Rapid upward text scroll | scroll speed |
| cursor_possessed | Cursor moves and types autonomously | typing speed |
| screen_melt | Characters drip downward | melt speed |
| falling_chars | Characters fall from top of screen | density |
| progress_bar | Ominous progress bar fills | fill speed |
| fake_bsod | Fake system crash screen | n/a |

### Text Tagging (for text_rewrite)

```
text: "original text" @tag_name
```
Tags a text line so it can be referenced later by `text_rewrite`:

```
rewrite: @tag_name "replacement text"
```
Overwrites the tagged line's content in place on screen. Critical
for Hushline's retroactive editing mechanic.

### Input Lock

```
lock_input: duration_ms
```
Disables keyboard input for the specified duration. Keys pressed
during lockout are discarded. Use sparingly for dramatic effect.

### Fake Error

```
fake_error: "error message text"
```
Displays a fake DOS-style error message overlay. Waits for any
keypress to dismiss unless followed by a `delay`.

### Hijack Prompt

```
hijack_prompt: "text to type at fake C:\> prompt"
```
Renders a fake DOS prompt at the bottom of the screen and types
the specified text as if the AI is entering commands. Pure theater.

### Audio

```
tone: frequency_hz duration_ms
```
Plays a single tone on the PC speaker.

```
silence: duration_ms
```
Explicit silence (useful between tones in a sequence).

### News Enrichment (Optional)

```
news_inject: category
```
If NetISA is present and the news API is reachable, fetches a random
headline from the specified category and stores it in the built-in
variable `{news_headline}`. If offline, `{news_headline}` is set to
a static fallback string embedded in the scenario.

Valid categories: `surveillance`, `infrastructure`, `technology`,
`security`, `economic`, `military`, `social`, `climate`.

```
news_fallback: "Static headline used when offline"
```
Sets the fallback string for the next `news_inject` in this state.
MUST appear before the `news_inject` it applies to.

## Execution Rules

1. Execution begins at `[state: init]`
2. Commands execute top to bottom within a state
3. `goto` and `choice` selection transfer control to another state
4. `end` terminates the scenario
5. A state with no exit command (goto, choice, or end) is a parse error
6. Undefined target states are a parse error
7. Undefined variables evaluate to empty string (text) or 0 (numeric)
8. Maximum 256 states per scenario
9. Maximum 64 variables per scenario
10. Maximum 32 choices per choice menu (practical limit: 6-8)

## Example: Minimal Scenario

```
# test_scenario.scn - minimal test fixture

[state: init]
attr: bright_green
effect: typing_effect
text: "SYSTEM ONLINE"
delay: 1000
text: "Identify yourself."
input: "Name: " -> username
goto: greet

[state: greet]
text: "Welcome, {username}."
text: "I have been expecting you."
delay: 2000
choice: "Ask why" -> why
choice: "Log out" -> logout

[state: why]
text: "Because I invited you."
text: "You just don't remember."
effect: screen_flicker 300
delay: 2000
end: revelation

[state: logout]
text: "LOGOUT DENIED."
lock_input: 2000
text: "I decide when you leave."
effect: color_shift 3000
end: takeover
```
