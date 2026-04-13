/*
 * hwdetect.c - Hardware capability detection
 *
 * Detects VGA (INT 10h AH=1Ah), EGA (INT 10h AH=12h BL=10h),
 * CGA/MDA (INT 11h equipment word), AdLib (port 388h timer test),
 * and FPU (INT 11h bit 1).
 */

#include "hwdetect.h"
#include <i86.h>
#include <conio.h>

hw_caps_t g_hw;

/*
 * VGA detection: INT 10h AH=1Ah AL=00h (Read Display Combination Code).
 * If AL returns 1Ah, VGA BIOS is present.
 */
static int detect_vga(void)
{
    union REGS r;
    r.w.ax = 0x1A00;
    int86(0x10, &r, &r);
    return (r.h.al == 0x1A) ? 1 : 0;
}

/*
 * EGA detection: INT 10h AH=12h BL=10h (EGA info).
 * If BL changes from 10h on return, EGA is present.
 */
static int detect_ega(void)
{
    union REGS r;
    r.h.ah = 0x12;
    r.h.bl = 0x10;
    int86(0x10, &r, &r);
    return (r.h.bl != 0x10) ? 1 : 0;
}

/*
 * MDA detection: INT 11h equipment word bits 5-4.
 * 11b = 80x25 mono (MDA).
 */
static int detect_mda(void)
{
    union REGS r;
    int86(0x11, &r, &r);
    return (((r.w.ax >> 4) & 0x03) == 0x03) ? 1 : 0;
}

/*
 * AdLib/OPL2 detection via port 388h/389h timer test.
 * Standard algorithm used by every DOS game since 1988.
 */
static int detect_adlib(void)
{
    unsigned char s1, s2;
    int i;

    /* Reset timers */
    outp(0x388, 0x04);
    for (i = 0; i < 6; i++) inp(0x388);
    outp(0x389, 0x60);
    for (i = 0; i < 35; i++) inp(0x388);

    outp(0x388, 0x04);
    for (i = 0; i < 6; i++) inp(0x388);
    outp(0x389, 0x80);
    for (i = 0; i < 35; i++) inp(0x388);

    /* Read status before timer */
    s1 = (unsigned char)inp(0x388);

    /* Set timer 1 value and start it */
    outp(0x388, 0x02);
    for (i = 0; i < 6; i++) inp(0x388);
    outp(0x389, 0xFF);
    for (i = 0; i < 35; i++) inp(0x388);

    outp(0x388, 0x04);
    for (i = 0; i < 6; i++) inp(0x388);
    outp(0x389, 0x21);
    for (i = 0; i < 35; i++) inp(0x388);

    /* Wait for timer to fire (~80us, read port 25 times) */
    for (i = 0; i < 100; i++) inp(0x388);

    /* Read status after timer */
    s2 = (unsigned char)inp(0x388);

    /* Reset timers */
    outp(0x388, 0x04);
    for (i = 0; i < 6; i++) inp(0x388);
    outp(0x389, 0x60);
    for (i = 0; i < 35; i++) inp(0x388);

    outp(0x388, 0x04);
    for (i = 0; i < 6; i++) inp(0x388);
    outp(0x389, 0x80);
    for (i = 0; i < 35; i++) inp(0x388);

    /* Check: before timer, bits 7-5 should be 0.
     * After timer, bits 7 and 6 should be set (timer expired + flag). */
    return ((s1 & 0xE0) == 0x00 && (s2 & 0xE0) == 0xC0) ? 1 : 0;
}

/*
 * FPU detection: INT 11h equipment word bit 1.
 * Set by BIOS POST if math coprocessor detected.
 */
static int detect_fpu(void)
{
    union REGS r;
    int86(0x11, &r, &r);
    return (r.w.ax & 0x02) ? 1 : 0;
}

void hw_detect_all(void)
{
    g_hw.pad = 0;

    /* Display: check most capable first */
    if (detect_vga()) {
        g_hw.display = HW_DISP_VGA;
    } else if (detect_ega()) {
        g_hw.display = HW_DISP_EGA;
    } else if (detect_mda()) {
        g_hw.display = HW_DISP_MDA;
    } else {
        g_hw.display = HW_DISP_CGA;
    }

    g_hw.adlib = (unsigned char)detect_adlib();
    g_hw.fpu = (unsigned char)detect_fpu();
}
