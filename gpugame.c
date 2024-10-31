#include <jaguar.h>

#include "gpu_68k_shr.h"
#include "startup.h"
#include "u235se.h"
#include "music.h"

volatile unsigned long cpuCmd;
void *volatile cpuData;

static void gpu_main(void);

/* This has to be the first function, since gcc sets up the stack in the first function */
void gpu_start(void)
{
    /* Stuff that the C startup code would normally do */
    cpuCmd = CPUCMD_IDLE;
    cpuData = 0;

    gpu_main();
}

static void blit_rect(unsigned int color, unsigned int x, unsigned int y, unsigned int width, unsigned int height)
{
    unsigned int flags = PITCH1 | PIXEL16 | WID320;
    int stepx = (BMP_PHRASES * 4);

    if (width > 0) {
        stepx = width;
    } else {
        width = BMP_PHRASES * 4;
    }

    if ((width & 3) || (x & 3)) {
        flags |= XADDPIX;
    } else {
        flags |= XADDPHR;
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
    *A1_FLAGS = flags;

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

static void run68kCmd(void)
{
    static volatile long *gctrl = G_CTRL;

    /* Interrupt the 68k */
    *gctrl = GPUGO | 0x2;

    while (cpuCmd != 0);
}

static void ChangeMusicGPU(void *music)
{
    cpuData = music;
    cpuCmd = CPUCMD_CHANGE_MUSIC;

    run68kCmd();
}

static void gpu_main(void)
{
    unsigned int i;
    unsigned int oldTicks = ticks;
    unsigned int color;
    unsigned int oldPad1 = 0;
    unsigned int newPad1;
    unsigned int printDelay = 0;
    int newMusic = 0;

    while (1) {
        while (oldTicks == ticks);
        oldTicks = ticks;

        color = (oldTicks & 0xffff);
        color |= (color << 16);

        blit_color(color);
        blit_rect(0xff77ff77, 40, 40, 240, 5);
        blit_rect(0xff77ff77, 40, 45, 239, 5);
        blit_rect(0xff77ff77, 40, 50, 238, 5);
        blit_rect(0xff77ff77, 40, 55, 237, 5);
        blit_rect(0xff77ff77, 40, 60, 236, 5);
        blit_rect(0xff77ff77, 40, 65, 235, 5);
        blit_rect(0xff77ff77, 40, 70, 234, 5);
        blit_rect(0xff77ff77, 40, 75, 233, 5);
        blit_rect(0xff77ff77, 40, 80, 232, 5);
        blit_rect(0xff77ff77, 40, 85, 231, 5);
        blit_rect(0xff77ff77, 40, 90, 230, 5);

        blit_rect(0xff77ff77, 40, 100, 240, 5);
        blit_rect(0xff77ff77, 40, 105, 236, 5);
        blit_rect(0xff77ff77, 40, 110, 232, 5);
        blit_rect(0xff77ff77, 40, 115, 228, 5);
        blit_rect(0xff77ff77, 40, 120, 224, 5);
        blit_rect(0xff77ff77, 40, 125, 220, 5);
        blit_rect(0xff77ff77, 40, 130, 216, 5);
        blit_rect(0xff77ff77, 40, 135, 212, 5);
        blit_rect(0xff77ff77, 40, 140, 208, 5);
        blit_rect(0xff77ff77, 40, 145, 204, 5);
        blit_rect(0xff77ff77, 40, 150, 200, 5);

        blit_rect(0xff77ff77, 40, 160, 240, 5);
        blit_rect(0xff77ff77, 41, 165, 238, 5);
        blit_rect(0xff77ff77, 42, 170, 236, 5);
        blit_rect(0xff77ff77, 43, 175, 234, 5);
        blit_rect(0xff77ff77, 44, 180, 232, 5);
        blit_rect(0xff77ff77, 45, 185, 230, 5);
        blit_rect(0xff77ff77, 46, 190, 228, 5);
        blit_rect(0xff77ff77, 47, 195, 226, 5);
        blit_rect(0xff77ff77, 48, 200, 224, 5);
        blit_rect(0xff77ff77, 49, 205, 222, 5);
        blit_rect(0xff77ff77, 50, 210, 220, 5);

        spinCount += 1;

        newPad1 = *u235se_pad1;

        if (((oldPad1 ^ newPad1) & newPad1) & U235SE_BUT_B) {
            if (!newMusic) {
                ChangeMusicGPU(mus_main);
                newMusic = 1;
            } else {
                ChangeMusicGPU(mus_title);
                newMusic = 0;
            }
        }

        oldPad1 = *u235se_pad1;

        if (++printDelay >= 60) {
            printDelay = 0;
            cpuCmd = CPUCMD_PRINT_STATS;
            run68kCmd();
        }
    }
}
