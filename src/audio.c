/*
 * audio.c - PC speaker tone generation via PIT
 *
 * Uses the 8253/8254 Programmable Interval Timer channel 2
 * and port 61h speaker gate for tone output.
 *
 * PIT base frequency: 1,193,180 Hz
 * Divisor = 1193180 / desired_frequency
 */

#include "audio.h"
#include "engine.h"     /* engine_delay */
#include <conio.h>      /* inp, outp */

/* PIT oscillator frequency */
#define PIT_FREQ    1193180UL

/* I/O ports */
#define PIT_CMD     0x43    /* PIT command register */
#define PIT_CH2     0x42    /* PIT channel 2 data */
#define PORT_61H    0x61    /* Speaker gate / PIT gate */

static int sfx_muted = 0;

void audio_set_mute(int m) { sfx_muted = m; }
int  audio_get_mute(void)  { return sfx_muted; }

void audio_tone(int freq_hz, int duration_ms)
{
    unsigned int divisor;
    unsigned char gate;

    if (freq_hz <= 0 || duration_ms <= 0)
        return;

    if (sfx_muted) {
        engine_delay(duration_ms);
        return;
    }

    /* Compute PIT divisor. Clamp to 16-bit range. */
    if (freq_hz < 19)
        freq_hz = 19;   /* minimum for 16-bit divisor */
    divisor = (unsigned int)(PIT_FREQ / (unsigned long)freq_hz);

    /* Program PIT channel 2: mode 3 (square wave), binary counting.
     * Command byte 0xB6 = channel 2, lobyte/hibyte, mode 3, binary. */
    outp(PIT_CMD, 0xB6);
    outp(PIT_CH2, divisor & 0xFF);          /* low byte */
    outp(PIT_CH2, (divisor >> 8) & 0xFF);   /* high byte */

    /* Enable speaker: set bits 0 (PIT gate) and 1 (speaker data) */
    gate = (unsigned char)inp(PORT_61H);
    outp(PORT_61H, gate | 0x03);

    /* Hold for duration */
    engine_delay(duration_ms);

    /* Disable speaker: clear bits 0 and 1 */
    gate = (unsigned char)inp(PORT_61H);
    outp(PORT_61H, gate & 0xFC);
}

void audio_silence(int duration_ms)
{
    if (duration_ms > 0)
        engine_delay(duration_ms);
}
