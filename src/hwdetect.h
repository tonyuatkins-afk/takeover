/*
 * hwdetect.h - Hardware capability detection
 *
 * Detects display adapter (VGA/EGA/CGA/MDA), AdLib/OPL2 sound card,
 * and math coprocessor (8087/287/387) at startup. All other modules
 * query g_hw to decide capabilities.
 */

#ifndef HWDETECT_H
#define HWDETECT_H

/* Display adapter types (mutually exclusive) */
#define HW_DISP_MDA     0
#define HW_DISP_CGA     1
#define HW_DISP_EGA     2
#define HW_DISP_VGA     3

typedef struct {
    unsigned char display;    /* HW_DISP_* constant */
    unsigned char adlib;      /* 1 if AdLib/OPL2 present */
    unsigned char fpu;        /* 1 if math coprocessor present */
    unsigned char pad;        /* padding to 4 bytes */
} hw_caps_t;

extern hw_caps_t g_hw;

/* Detect all hardware. Call once at startup before scr_init(). */
void hw_detect_all(void);

#endif /* HWDETECT_H */
