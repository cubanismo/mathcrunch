/*
 * Copyright 2025 James Jones
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <jaguar.h>

#include "gpucommon.h"
#include "gpuasm.h"

#include "gpu_68k_shr.h"
#include "music.h"
#include "u235se.h"

static void SetSpriteList(Sprite *list)
{
    cpuData0 = list;
    run68kCmd(CPUCMD_SET_SPRITE_LIST);
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
    case 40:
        blitterFlags |= WID40;
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
    } else if (width == 40) {
        blitterFlags |= WID40;
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
}

#define GRID_START_X (40)
#define GRID_START_Y (40)

#define GRID_CLR                0x60e960e9
#define SCORE_BOX_CLR           0x1bff1bff
#define TITLE_BORDER_CLR        0xf7d6f7d6

static void init_screen(Sprite *screen, unsigned int frame, unsigned int color)
{
    int i;
    int j;
    unsigned int val;

    /* Clear to backgroud color */
    blit_color(screen, frame, color);

    /* Draw the game grid in bright purple */
    for (j = 0; j < (GRID_BOXES_Y + 1 /* XXX Compiler bug. */ + 1); j++) {
        blit_rect(screen, frame, GRID_CLR, PACK_XY(GRID_START_X, GRID_START_Y + SHORT_MUL(j, GRID_SIZE_Y)), SHORT_MUL(GRID_BOXES_X, GRID_SIZE_X) + 1, 1);
    }

    for (i = 0; i < (GRID_BOXES_X + 1 /* XXX Compiler bug. */ + 1); i++) {
        blit_rect(screen, frame, GRID_CLR, PACK_XY(GRID_START_X + SHORT_MUL(i, GRID_SIZE_X), GRID_START_Y), 1, SHORT_MUL(GRID_BOXES_Y, GRID_SIZE_Y));
    }

    /* Draw the score box in light blue */
    blit_rect(screen, frame, SCORE_BOX_CLR, PACK_XY(GRID_START_X + 40, GRID_START_Y + SHORT_MUL(5, GRID_SIZE_Y) + 5), 60, 2);
    blit_rect(screen, frame, SCORE_BOX_CLR, PACK_XY(GRID_START_X + 40, GRID_START_Y + SHORT_MUL(5, GRID_SIZE_Y) + 20), 60, 2);
    blit_rect(screen, frame, SCORE_BOX_CLR, PACK_XY(GRID_START_X + 40, GRID_START_Y + SHORT_MUL(5, GRID_SIZE_Y) + 7), 2, 13);
    blit_rect(screen, frame, SCORE_BOX_CLR, PACK_XY(GRID_START_X + 98, GRID_START_Y + SHORT_MUL(5, GRID_SIZE_Y) + 7), 2, 13);

    /* Draw the level title borders in orange */
    blit_rect(screen, frame, TITLE_BORDER_CLR, PACK_XY(GRID_START_X + 30, GRID_START_Y - 25), SHORT_MUL(6, GRID_SIZE_X) - 60, 2);
    blit_rect(screen, frame, TITLE_BORDER_CLR, PACK_XY(GRID_START_X + 30, GRID_START_Y - 5), SHORT_MUL(6, GRID_SIZE_X) - 60, 2);

    /* Wait for blitter to idle */
    BLITTER_WAIT();

    draw_string(screen, frame, PACK_XY(GRID_START_X - 15, GRID_START_Y - 25), level_str);
    draw_string(screen, frame, PACK_XY(GRID_START_X, GRID_START_Y - 15), levelnum_str);
    draw_string(screen, frame, PACK_XY(GRID_START_X - 15, GRID_START_Y + SHORT_MUL(5, GRID_SIZE_Y) + 8), score_str);
    draw_string(screen, frame, PACK_XY(GRID_START_X + 75, GRID_START_Y - 20), levelname_str);

    /*
     * XXX A general note on the JRISC GCC compiler's for loop handling:
     *
     * The compiler has two bugs that standard for (0..N) for loops tend to hit:
     *
     * -As noted in comments above, its math is always off by one for such lsoops
     * -When it uses cmpq for the loop condition, it interprets the order of the
     *  parameters backwards (cmpq's parameters are reversed compared to those
     *  of the similar cmp, sub, and subq instructions, but the compiler behaves
     *  as if its parameter ordering is the same as those instructions).
     *
     * Both these bugs can be avoided by counting down instead of up.
     */
    for (i = GRID_BOXES_Y - 1; i >= 0; i--) {
        for (j = GRID_BOXES_X - 1; j >= 0; j--) {
            val = square_data[i][j].val;
            int_to_str_gpu(tmp_str, val);
            draw_string(screen, frame,
                        PACK_XY(GRID_START_X + SHORT_MUL(j, GRID_SIZE_X) + 15,
                                GRID_START_Y + SHORT_MUL(i, GRID_SIZE_Y) + 13), tmp_str);
        }
    }
}

void levelinit(void)
{
    unsigned int i;
    unsigned int oldTicks;
    unsigned int bg_color = 0x0; /* Black */
    Sprite *screen = &spriteData[0];

    animations = NULL;
    timers = NULL;

    enemy_x[0] = 1;
    enemy_x[1] = 2;
    enemy_y[0] = enemy_y[1] = 0;

    make_sprite(screen,
                screenbmp,
                BMP_PHRASES * 4,
                BMP_HEIGHT,
                SPRITE_DEPTH16,
                SPRITE_SINGLE_BUFFERED | SPRITE_NOT_TRANSPARENT);

    SET_SPRITE_X(screen, screen_off_x);
    SET_SPRITE_Y(screen, screen_off_y);

    for (i = 1; i < (4 /* Work around compiler bug */ + 1); i++) {
        Sprite *tmpSprite = &spriteData[i];

        make_sprite(tmpSprite,
                    spritebmps[i-1],
                    PLAYER_WIDTH,
                    PLAYER_HEIGHT,
                    SPRITE_DEPTH16,
                    SPRITE_SINGLE_BUFFERED | SPRITE_TRANSPARENT);

        SET_SPRITE_X(tmpSprite, screen_off_x + GRID_START_X + SHORT_MUL(PLAYER_WIDTH, i - 1));
        SET_SPRITE_Y(tmpSprite, screen_off_y + GRID_START_Y);

        spriteData[i - 1].next = tmpSprite;
    }

    SetSpriteList(screen);

    do {
        num_multiples_remaining = pick_numbers(mult_vals, multiple_of);
    } while (num_multiples_remaining == 0);

    /*
     * Should be:
     *   for (i = 0; i < 2; i++)
     * But that only iterates once because the compiler is buggy
     */
    init_screen(screen, 1, bg_color);
    /* Need BLITTER_WAIT() here if init_screen() doesn't end in draw_string */
    set_sprite_frame(screen, 1);
    oldTicks = ticks;
    while (ticks == oldTicks);

    init_screen(screen, 0, bg_color);
    /* Need BLITTER_WAIT() here if init_screen() doesn't end in draw_string */
    set_sprite_frame(screen, 0);
    oldTicks = ticks;
    while (ticks == oldTicks);

    nextOverlay = &gpu_playlevel;
}
