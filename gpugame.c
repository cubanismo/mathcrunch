#include <jaguar.h>

#include "gpu_68k_shr.h"
#include "startup.h"
#include "u235se.h"
#include "music.h"
#include "sprites.h"

#define NULL ((void *)0)

extern void drawStringOff(const Sprite *sprite,
                          unsigned long coords,
                          void *str,
                          unsigned long frame_offset);

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

static unsigned int calc_frame_offset(unsigned int frameSize, unsigned int frameNum)
{
    unsigned int result = 0;

    /*
     * JRISC only has a 16-bit multiply. Implement a really naive 32-bit multiply
     * that will be good enough for small values of frameNum.
     */
    while (frameNum--) result += frameSize;

    return result;
}

static inline void drawString(const Sprite *sprite,
                              unsigned int frameNum,
                              unsigned long coords,
                              void *str)
{
    drawStringOff(sprite, coords, str, calc_frame_offset(sprite->frameSize, frameNum));
}

static void blit_rect(
    const Sprite *dst,
    unsigned int frame_num,
    unsigned int color,
    unsigned int y_x,
    unsigned int width,
    unsigned int height
)
{
    unsigned int flags = dst->blitterFlags;

    if (flags == 0) {
        /* Sprite is not compatible with the blitter (Probably due to width) */
        return;
    }

    if ((width & 3) || (y_x & 3)) {
        flags |= XADDPIX;
    } else {
        flags |= XADDPHR;
    }

    /* Wait for blitter to idle */
    while ((*(volatile long *)B_CMD & 1) == 0);

    /* Set address of destination surface */
    *(volatile long *)A1_BASE = dst->surfAddr + calc_frame_offset(dst->frameSize, frame_num);

    /*
     * Contiguous phrases,
     * 16-bit pixels,
     * Window Width = BMP_WIDTH
     * Blit a phrase at a time
     */
    *(volatile long *)A1_FLAGS = flags;

    /* Start at <x (low 16 bits), y (high 16 bits)> */
    *(volatile long *)A1_PIXEL = y_x;

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

static void blit_color(const Sprite *dst, unsigned int frame_num, unsigned int color)
{
    blit_rect(dst, frame_num, color, 0, BMP_PHRASES * 4, BMP_HEIGHT);
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

/* Shift address into place for first phrase high word of bitmap object*/
#define ADDR_TO_OBJ_BITMAP(addr_) ((unsigned int)(addr_) << (11-3))

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

    /* This generates broken code. Use the if statements below instead
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
    } */
    if (width == 16) {
        blitterFlags |= WID16;
    } else if (width == 32) {
        blitterFlags |= WID32;
    } else if (width == 48) {
        blitterFlags |= WID48;
    } else if (width == 64) {
        blitterFlags |= WID64;
    } else if (width == 96) {
        blitterFlags |= WID96;
    } else if (width == 128) {
        blitterFlags |= WID128;
    } else if (width == 192) {
        blitterFlags |= WID192;
    } else if (width == 256) {
        blitterFlags |= WID256;
    } else if (width == 320) {
        blitterFlags |= WID320;
    } else if (width == 512) {
        blitterFlags |= WID512;
    } else {
        blitterFlags = 0;
    }

    sprite->blitterFlags = blitterFlags;
    sprite->surfAddr = (unsigned int)address;
    sprite->frameSize = (unsigned short)((width << depth) >> 3) * (unsigned short)height;
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
    sprite->firstPhraseHighTemplate = ADDR_TO_OBJ_BITMAP(address);

    /* Third word: x (0) | depth | pitch (1|2) | dwidth | iwidth */
    sprite->secondPhraseLowTemplate = (depth << 12) | ((((flags & 1) >> 1) + 1) << 15) | (width << 18) | ((width & 0xf) << 28);

    /* Fourth word: iwidth | trans */
    sprite->secondPhraseHighTemplate = (width >> 4) | ((flags & 1) << 15);

    SET_SPRITE_X(sprite, 32);
    SET_SPRITE_Y(sprite, 32);
}

static void set_sprite_frame(
    Sprite *sprite,
    unsigned int frame
)
{
    /* First phrase high dword: link (0) | address (data) */
    sprite->firstPhraseHighTemplate = ADDR_TO_OBJ_BITMAP(sprite->surfAddr + calc_frame_offset(sprite->frameSize, frame));
}

static void init_screen(Sprite *screen, unsigned int frame, unsigned int color)
{
    blit_color(screen, frame, color);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 40), 240, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 45), 239, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 50), 238, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 55), 237, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 60), 236, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 65), 235, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 70), 234, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 75), 233, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 80), 232, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 85), 231, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 90), 230, 5);

    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 100), 240, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 105), 236, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 110), 232, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 115), 228, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 120), 224, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 125), 220, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 130), 216, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 135), 212, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 140), 208, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 145), 204, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 150), 200, 5);

    blit_rect(screen, frame, 0xff77ff77, PACK_XY(40, 160), 240, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(41, 165), 238, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(42, 170), 236, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(43, 175), 234, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(44, 180), 232, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(45, 185), 230, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(46, 190), 228, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(47, 195), 226, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(48, 200), 224, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(49, 205), 222, 5);
    blit_rect(screen, frame, 0xff77ff77, PACK_XY(50, 210), 220, 5);
}

static void gpu_main(void)
{
    unsigned int i;
    unsigned int oldTicks = ticks;
    unsigned int color = 0x00ff00ff;
    unsigned int oldPad1 = 0;
    unsigned int newPad1;
    unsigned int printDelay = 0;
    int newMusic = 0;
    Sprite *screen = &spriteData[0];
    Sprite *player = &spriteData[1];
    unsigned int player_x = 32;
    unsigned int player_y = 32;
    unsigned int sprite_frame = 1;

    make_sprite(screen,
                screenbmp,
                BMP_PHRASES * 4,
                BMP_HEIGHT,
                SPRITE_DEPTH16,
                SPRITE_SINGLE_BUFFERED | SPRITE_NOT_TRANSPARENT);

    SET_SPRITE_X(screen, 16); /* Copied from InitLister logic, NTSC version for 320x240 bitmap */
    SET_SPRITE_Y(screen, 13); /* Copied from InitLister logic, NTSC version for 320x240 bitmap */

    make_sprite(player,
                playerbmp,
                PLAYER_WIDTH,
                PLAYER_HEIGHT,
                SPRITE_DEPTH16,
                SPRITE_SINGLE_BUFFERED | SPRITE_TRANSPARENT);

    blit_rect(player, 0, 0xff00ff00, 0, PLAYER_WIDTH, PLAYER_HEIGHT);

    screen->next = player;

    SetSpriteList(screen);

    /*
     * Should be:
     *   for (i = 0; i < 2; i++)
     * But that only iterates once because the compiler is buggy
     */
    init_screen(screen, 0, color);
    init_screen(screen, 1, color);

    /* Wait for blitter to idle */
    while ((*(volatile long *)B_CMD & 1) == 0);

    while (1) {
        oldTicks = ticks;
        while (ticks == oldTicks);

        sprite_frame = !sprite_frame;

        blit_rect(screen, sprite_frame, color, PACK_XY(20, 10), 240, 30);

        /* Wait for blitter to idle */
        while ((*(volatile long *)B_CMD & 1) == 0);

        drawString(screen, sprite_frame, PACK_XY(20, 10), gpuStr);
        drawString(screen, sprite_frame, PACK_XY(20, 20), dspStr);

        set_sprite_frame(screen, sprite_frame);

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

        if (((oldPad1 ^ newPad1) & newPad1) & U235SE_BUT_A) {
            if (screen->next) {
                screen->next = NULL;
            } else {
                screen->next = player;
            }
        }

        if (newPad1 & U235SE_BUT_UP) {
            player_y -= 1;
        } else if (newPad1 & U235SE_BUT_DOWN) {
            player_y += 1;
        }

        if (newPad1 & U235SE_BUT_LEFT) {
            player_x -= 1;
        } else if (newPad1 & U235SE_BUT_RIGHT) {
            player_x += 1;
        }
        SET_SPRITE_X(player, player_x);
        SET_SPRITE_Y(player, player_y);

        oldPad1 = *u235se_pad1;

        if (++printDelay >= 30) {
            printDelay = 0;
            cpuCmd = CPUCMD_PRINT_STATS;
            run68kCmd();
        }
    }
}
