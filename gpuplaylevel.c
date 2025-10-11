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

static void ChangeMusicGPU(void *music)
{
    cpuData0 = music;
    run68kCmd(CPUCMD_CHANGE_MUSIC);
}

#define GRID_START_X (40)
#define GRID_START_Y (40)

#define GRID_CLR                0x60e960e9
#define SCORE_BOX_CLR           0x1bff1bff
#define TITLE_BORDER_CLR        0xf7d6f7d6

static void myclamp(int *val, int min, int max) {
    if (*val < min) *val = min;
    if (*val > max) *val = max;
}

static void update_timers(void)
{
    Timer **t = &timers;

    while (*t) {
        if (ticks >= (*t)->endTick) {
            (*t)->animation = animations;
            animations = (*t)->animation;
            *t = (*t)->next;
        } else {
            t = &(*t)->next;
        }
    }
}

void playlevel(void)
{
    unsigned int oldTicks;
    unsigned int bg_color = 0x0; /* Black */
    unsigned int oldPad1 = 0;
    unsigned int newPad1;
    unsigned int printDelay = 0;
    Sprite *screen = &spriteData[0];
    int player_x = 0;
    int player_y = 0;
    unsigned int sprite_frame = 1;
    char *end_str = win_str;
    Animation *a, *aLocal;

    while (num_multiples_remaining) {
        oldTicks = ticks;
        while (ticks == oldTicks);

        update_timers();
        update_animations();

        sprite_frame = !sprite_frame;

        /* Clear text regions */

#if DEBUG_STATS != 0
        /* Debug text */
        blit_rect(screen, sprite_frame, bg_color, PACK_XY(100, 210), 200, 20);
#endif

        /* Score */
        blit_rect(screen, sprite_frame, bg_color, PACK_XY(GRID_START_X + 42, GRID_START_Y + SHORT_MUL(5, PLAYER_HEIGHT) + 7), 56, 13);

        /* Wait for blitter to idle */
        BLITTER_WAIT();

#if DEBUG_STATS != 0
        draw_string(screen, sprite_frame, PACK_XY(100, 210), gpu_str);
        draw_string(screen, sprite_frame, PACK_XY(100, 220), dsp_str);
#endif

        draw_string(screen, sprite_frame, PACK_XY(GRID_START_X + 45, GRID_START_Y + SHORT_MUL(5, GRID_SIZE_Y) + 8), scoreval_str);

        set_sprite_frame(screen, sprite_frame);

        spinCount += 1;

        newPad1 = *u235se_pad1;

        if (((oldPad1 ^ newPad1) & newPad1) & U235SE_BUT_B) {
            SquareData *s = &square_data[player_y][player_x];

            /* XXX Work around compiler bug. Compiler assumes load sets flags */
            unsigned int is_multiple = s->is_multiple;

            if (s->is_visible) {
                s->is_visible = 0;
                if (is_multiple == 0) {
                    end_str = lose_str;
                    ChangeMusicGPU(mus_main);
                    screen->next = NULL;
                    break;
                }

                run68kCmd(CPUCMD_PLAY_SOUND);
                score += 5;
                int_to_str_gpu(scoreval_str, score);
                num_multiples_remaining--;

                blit_rect(screen, 0, bg_color,
                          PACK_XY(GRID_START_X + 1 + SHORT_MUL(player_x, GRID_SIZE_X),
                                  GRID_START_Y + 1 + SHORT_MUL(player_y, GRID_SIZE_Y)),
                          GRID_SIZE_X - 1, GRID_SIZE_Y - 1);
                blit_rect(screen, 1, bg_color,
                          PACK_XY(GRID_START_X + 1 + SHORT_MUL(player_x, GRID_SIZE_X),
                                  GRID_START_Y + 1 + SHORT_MUL(player_y, GRID_SIZE_Y)),
                          GRID_SIZE_X - 1, GRID_SIZE_Y - 1);

                if (num_multiples_remaining == 0) {
                    screen->next = NULL;
                }
            }
        }

        a = NULL;

        /*
         * Work around compiler bug. It acts as if loading "animations" into a
         * register will set the flags, and seems to do a jump based on
         * it, but loads do not set flags on JRISC
         */
        aLocal = animations;

        if (newPad1 & U235SE_BUT_UP) {
            if (!aLocal) {
                player_y -= 1;
                a = &animationData[0];
            }
        } else if (newPad1 & U235SE_BUT_DOWN) {
            if (!aLocal) {
                player_y += 1;
                a = &animationData[0];
            }
        } else if (newPad1 & U235SE_BUT_LEFT) {
            if (!aLocal) {
                player_x -= 1;
                a = &animationData[0];
            }
        } else if (newPad1 & U235SE_BUT_RIGHT) {
            if (!aLocal) {
                player_x += 1;
                a = &animationData[0];
            }
        }

        myclamp(&player_x, 0, GRID_MAX_X);
        myclamp(&player_y, 0, GRID_MAX_Y);

        if (a) {
            a->sprite = &spriteData[1];
            a->endX = screen_off_x + GRID_START_X + SHORT_MUL(player_x, GRID_SIZE_X);
            a->endY = screen_off_y + GRID_START_Y + SHORT_MUL(player_y, GRID_SIZE_Y);
            a->speedPerTick = 4;
            a->next = animations;
            animations = a;
        }

        oldPad1 = newPad1;

        if (++printDelay >= 30) {
            printDelay = 0;
            run68kCmd(CPUCMD_PRINT_STATS);
        }
    }

    /* Display a super fancy victory or game over screen */
    blit_color(screen, sprite_frame, bg_color);
    BLITTER_WAIT();
    draw_string(screen, sprite_frame, PACK_XY(40, 110), end_str);
    if (level_num < 8) {
        draw_string(screen, sprite_frame, PACK_XY(40, 130), press_c_str);
    }

    oldPad1 = newPad1;
    do {
        newPad1 = *u235se_pad1;

        if (((oldPad1 ^ newPad1) & newPad1) & U235SE_BUT_C) break;

        oldPad1 = newPad1;
    } while (1);

    if (num_multiples_remaining > 0) {
        level_num = 0;
    }

    nextOverlay = NULL;
}
