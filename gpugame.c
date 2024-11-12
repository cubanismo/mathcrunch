#include <jaguar.h>

#include "gpu_68k_shr.h"
#include "startup.h"
#include "u235se.h"
#include "music.h"
#include "sprites.h"

#define NULL ((void *)0)

extern drawString(const Sprite *sprite, unsigned long coords, void *str);

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

static void blit_rect(
    const Sprite *dst,
    unsigned int color,
    unsigned int x,
    unsigned int y,
    unsigned int width,
    unsigned int height
)
{
    unsigned int flags = dst->blitterFlags;

    if (flags == 0) {
        /* Sprite is not compatible with the blitter (Probably due to width) */
        return;
    }

    if ((width & 3) || (x & 3)) {
        flags |= XADDPIX;
    } else {
        flags |= XADDPHR;
    }

    /* Wait for blitter to idle */
    while ((*(volatile long *)B_CMD & 1) == 0);

    /* Set address of destination surface */
    *(volatile long *)A1_BASE = dst->surfAddr;

    /*
     * Contiguous phrases,
     * 16-bit pixels,
     * Window Width = BMP_WIDTH
     * Blit a phrase at a time
     */
    *(volatile long *)A1_FLAGS = flags;

    /* Start at <x (low 16 bits), y (high 16 bits)> */
    *(volatile long *)A1_PIXEL = x | (y << 16);

    /* After each line, back X up BMP_WIDTH pixels, add 1 to Y */
    *(volatile long *)A1_STEP = (1 << 16) | (-width & 0x0000ffff);

    /* Run inner (X) loop BMP_WIDTH times, outer (Y) loop BMP_HEIGHT times */
    *(volatile long *)B_COUNT = (height<<16) | width;

    /*
     * Jaguar RGB16 bits: RRRR.RBBB.BBGG.GGGG.
     * Repeat it 4 times to fill the phrase-size pattern register
     */
	((volatile long *)B_PATD)[0] = color;
	((volatile long *)B_PATD)[1] = color;

    /*
     * Add A1_STEP after each line/outer loop iteration, use pattern
     * data register for source.
     */
    /* *bcmd = blitCmd; */
    *(volatile long *)B_CMD = UPDA1|PATDSEL;

    blitCount++;
}

static void blit_color(const Sprite *dst, unsigned int color)
{
    blit_rect(dst, color, 0, 0, BMP_PHRASES * 4, BMP_HEIGHT);
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

static void SetSpriteList(Sprite *list)
{
    cpuData = list;
    cpuCmd = CPUCMD_SET_SPRITE_LIST;

    run68kCmd();
}

#define DEPTH_TO_BLIT_DEPTH(d_) ((d_) << 3)

/* XXX Don't use more than 6 parameters. Stack parameter passing is buggy (Uses return addr as 6th parameter)  */
static void make_sprite(
    Sprite *sprite,
    void *address,          /* Must be phrase aligned */
    unsigned int width,     /* Must be phrase aligned */
    unsigned int height,
    unsigned int depth,
    int flags               /* bit 0: 1 if color 0 is transparent, 0 otherwise */
                            /* bit 1: 1 if double-buffered, 0 otherwise */
)
{
    unsigned int blitterFlags = PITCH1 | DEPTH_TO_BLIT_DEPTH(depth);

    switch (width) {
    case 16:
        blitterFlags |= WID16;
        break;
    case 32:
        blitterFlags |= WID32;
        break;
    case 48:
        blitterFlags |= WID48;
        break;
    case 64:
        blitterFlags |= WID64;
        break;
    case 96:
        blitterFlags |= WID96;
        break;
    case 128:
        blitterFlags |= WID128;
        break;
    case 192:
        blitterFlags |= WID192;
        break;
    case 256:
        blitterFlags |= WID256;
        break;
    case 320:
        blitterFlags |= WID320;
        break;
    case 512:
        blitterFlags |= WID512;
        break;
    default:
        blitterFlags = 0;
        break;
    }

    sprite->blitterFlags = blitterFlags;
    sprite->surfAddr = (unsigned int)address;
    sprite->next = NULL;

    /*
     * Convert width to phrases:
     *  >> 1 (/ 2) for 32bpp (depth = 5)
     *  >> 2 (/ 4) for 16bpp (depth = 4)
     *  >> 3 (/ 8) for 8bpp  (depth = 3)
     *  >> 4 (/ 16) for 4bpp (depth = 2)
     *  >> 5 (/ 32) for 2bpp (depth = 1)
     *  >> 6 (/ 64) for 1bpp (depth = 0)
     */
    width >>= (6 - depth);

    /* First phrase low dword: type (0) | yPos (0) | height | link */
    sprite->firstPhraseLowTemplate = height << 14;  /* Shift height into place */

    /* First phrase high dword: link (0) | address (data) */
    sprite->firstPhraseHighTemplate = (unsigned int)address << (11-3); /* Shift address into place */

    /* Third word: x (0) | depth | pitch (1|2) | dwidth | iwidth */
    sprite->secondPhraseLowTemplate = (depth << 12) | ((((flags & 1) >> 1) + 1) << 15) | (width << 18) | ((width & 0xf) << 28);

    /* Fourth word: iwidth | trans */
    sprite->secondPhraseHighTemplate = (width >> 4) | ((flags & 1) << 15);

    SET_SPRITE_X(sprite, 32);
    SET_SPRITE_Y(sprite, 32);
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
    Sprite *sprite = &spriteData[0];

    make_sprite(sprite,
                screenbmp,
                BMP_PHRASES * 4,
                BMP_HEIGHT,
                SPRITE_DEPTH16,
                SPRITE_SINGLE_BUFFERED | SPRITE_NOT_TRANSPARENT);

    SET_SPRITE_X(sprite, 16); /* Copied from InitLister logic, NTSC version for 320x240 bitmap */
    SET_SPRITE_Y(sprite, 13); /* Copied from InitLister logic, NTSC version for 320x240 bitmap */

    SetSpriteList(sprite);

    while (1) {
        while ((oldTicks + 1) >= ticks);
        oldTicks = ticks;

        color = (oldTicks & 0xffff);
        color |= (color << 16);

        blit_color(sprite, color);
        blit_rect(sprite, 0xff77ff77, 40, 40, 240, 5);
        blit_rect(sprite, 0xff77ff77, 40, 45, 239, 5);
        blit_rect(sprite, 0xff77ff77, 40, 50, 238, 5);
        blit_rect(sprite, 0xff77ff77, 40, 55, 237, 5);
        blit_rect(sprite, 0xff77ff77, 40, 60, 236, 5);
        blit_rect(sprite, 0xff77ff77, 40, 65, 235, 5);
        blit_rect(sprite, 0xff77ff77, 40, 70, 234, 5);
        blit_rect(sprite, 0xff77ff77, 40, 75, 233, 5);
        blit_rect(sprite, 0xff77ff77, 40, 80, 232, 5);
        blit_rect(sprite, 0xff77ff77, 40, 85, 231, 5);
        blit_rect(sprite, 0xff77ff77, 40, 90, 230, 5);

        blit_rect(sprite, 0xff77ff77, 40, 100, 240, 5);
        blit_rect(sprite, 0xff77ff77, 40, 105, 236, 5);
        blit_rect(sprite, 0xff77ff77, 40, 110, 232, 5);
        blit_rect(sprite, 0xff77ff77, 40, 115, 228, 5);
        blit_rect(sprite, 0xff77ff77, 40, 120, 224, 5);
        blit_rect(sprite, 0xff77ff77, 40, 125, 220, 5);
        blit_rect(sprite, 0xff77ff77, 40, 130, 216, 5);
        blit_rect(sprite, 0xff77ff77, 40, 135, 212, 5);
        blit_rect(sprite, 0xff77ff77, 40, 140, 208, 5);
        blit_rect(sprite, 0xff77ff77, 40, 145, 204, 5);
        blit_rect(sprite, 0xff77ff77, 40, 150, 200, 5);

        blit_rect(sprite, 0xff77ff77, 40, 160, 240, 5);
        blit_rect(sprite, 0xff77ff77, 41, 165, 238, 5);
        blit_rect(sprite, 0xff77ff77, 42, 170, 236, 5);
        blit_rect(sprite, 0xff77ff77, 43, 175, 234, 5);
        blit_rect(sprite, 0xff77ff77, 44, 180, 232, 5);
        blit_rect(sprite, 0xff77ff77, 45, 185, 230, 5);
        blit_rect(sprite, 0xff77ff77, 46, 190, 228, 5);
        blit_rect(sprite, 0xff77ff77, 47, 195, 226, 5);
        blit_rect(sprite, 0xff77ff77, 48, 200, 224, 5);
        blit_rect(sprite, 0xff77ff77, 49, 205, 222, 5);
        blit_rect(sprite, 0xff77ff77, 50, 210, 220, 5);

        /* Wait for blitter to idle */
        while ((*(volatile long *)B_CMD & 1) == 0);

        drawString(sprite, (10 << 16) | 20, gpuStr);
        drawString(sprite, (20 << 16) | 20, dspStr);

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

        if (++printDelay >= 30) {
            printDelay = 0;
            cpuCmd = CPUCMD_PRINT_STATS;
            run68kCmd();
        }
    }
}
