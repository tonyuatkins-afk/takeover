/*
 * audio.h - PC speaker audio engine interface
 *
 * Tone generation via the 8253 PIT (Programmable Interval Timer)
 * and port 61h speaker gate.
 */

#ifndef AUDIO_H
#define AUDIO_H

/* Play a tone on the PC speaker.
 * freq_hz:     frequency in Hz (minimum ~19 Hz, practical range 37-8000)
 * duration_ms: duration in milliseconds */
void audio_tone(int freq_hz, int duration_ms);

/* Silence the speaker for a duration (explicit gap between tones) */
void audio_silence(int duration_ms);

#endif /* AUDIO_H */
