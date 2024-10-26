#include <jaguar.h>

#include "gpu_68k_shr.h"
#include "startup.h"

static void gpu_main(void);

/* This has to be the first function, since gcc sets up the stack in the first function */
void gpu_start(void)
{
    gpu_main();
}

static void blit_rect(unsigned int color, unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
    int stepx = (BMP_PHRASES * 4);

    if (width > 0) {
        stepx = width;

    } else {
        width = BMP_PHRASES * 4;
    }

    if (height == 0) {
        height = BMP_HEIGHT;
    }

    /* Wait for blitter to idle */
    while ((*(volatile long *)B_CMD & 1) == 0);

    /* Set address of destination surface */
    *A1_BASE = (unsigned long)screenbmp;

    /*
     * Contiguous phrases,
     * 16-bit pixels,
     * Window Width = BMP_WIDTH
     * Blit a phrase at a time
     */
    *A1_FLAGS = PITCH1|PIXEL16|WID320|XADDPHR;
    /* *A1_FLAGS = PITCH1|PIXEL16|WID320|XADDPIX; */

    /* Start at <x (low 16 bits), y (high 16 bits)> */
    *A1_PIXEL = x | (y << 16);

    /* After each line, back X up BMP_WIDTH pixels, add 1 to Y */
    *A1_STEP = (1 << 16) | (-stepx & 0x0000ffff);

    /* Run inner (X) loop BMP_WIDTH times, outer (Y) loop BMP_HEIGHT times */
    *B_COUNT = (height<<16) | width;

    /*
     * Jaguar RGB16 bits: RRRR.RBBB.BBGG.GGGG.
     * Repeat it 4 times to fill the phrase-size pattern register
     */
	(B_PATD)[0] = color;
	(B_PATD)[1] = color;

    /*
     * Add A1_STEP after each line/outer loop iteration, use pattern
     * data register for source.
     */
    *B_CMD = UPDA1|PATDSEL;

    blitCount++;
}

static void blit_color(unsigned int color)
{
    blit_rect(color, 0, 0, 0, 0);
}

static void gpu_main(void)
{
    unsigned int i;
    unsigned int oldTicks = ticks;
    unsigned int color;

    while (1) {
        while (oldTicks == ticks);
        oldTicks = ticks;

        color = (oldTicks & 0xffff);
        color |= (color << 16);

        blit_color(color);
        blit_rect(0xf0f0f0f0, 40, 40, 240, 5);
        blit_rect(0xf0f0f0f0, 40, 45, 239, 5);
        blit_rect(0xf0f0f0f0, 40, 50, 238, 5);
        blit_rect(0xf0f0f0f0, 40, 55, 237, 5);
        blit_rect(0xf0f0f0f0, 40, 60, 236, 5);
        blit_rect(0xf0f0f0f0, 40, 65, 235, 5);
        blit_rect(0xf0f0f0f0, 40, 70, 234, 5);
        blit_rect(0xf0f0f0f0, 40, 75, 233, 5);
        blit_rect(0xf0f0f0f0, 40, 80, 232, 5);
        blit_rect(0xf0f0f0f0, 40, 85, 231, 5);
        blit_rect(0xf0f0f0f0, 40, 90, 230, 5);

        blit_rect(0xf0f0f0f0, 40, 100, 240, 5);
        blit_rect(0xf0f0f0f0, 40, 105, 236, 5);
        blit_rect(0xf0f0f0f0, 40, 110, 232, 5);
        blit_rect(0xf0f0f0f0, 40, 115, 228, 5);
        blit_rect(0xf0f0f0f0, 40, 120, 224, 5);
        blit_rect(0xf0f0f0f0, 40, 125, 220, 5);
        blit_rect(0xf0f0f0f0, 40, 130, 216, 5);
        blit_rect(0xf0f0f0f0, 40, 135, 212, 5);
        blit_rect(0xf0f0f0f0, 40, 140, 208, 5);
        blit_rect(0xf0f0f0f0, 40, 145, 204, 5);
        blit_rect(0xf0f0f0f0, 40, 150, 200, 5);
        spinCount += 1;
    }
}
