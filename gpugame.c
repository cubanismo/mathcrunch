#include <jaguar.h>

#include "gpu_68k_shr.h"
#include "startup.h"
#include "u235se.h"
#include "music.h"
#include "sprites.h"
#include "gpuasm.h"

#define NULL ((void *)0)
#define GRID_SIZE_X (PLAYER_WIDTH)
#define GRID_SIZE_Y (PLAYER_HEIGHT)

#define GRID_MAX_X (5)
#define GRID_MAX_Y (4)

volatile unsigned long cpuCmd;
void *volatile cpuData0;
void *volatile cpuData1;

static void gpu_main(void);

#define SHORT_MUL(a, b) ((unsigned long)((unsigned short)(a) * (unsigned short)(b)))

/*
 * Wait for the blitter to go idle.
 *
 * This inline assembly block is roughly the equivalent of this C code:
 *
 *   while ((*(volatile long *)B_CMD & 1) == 0);
 *
 * But the compiler sometimes does a horrendous job of code generation on that
 * snippet. Compared to the:
 *
 *   5 instructions/7 words/1 branch/1 registers (+ TMP)
 *
 * used here, it generates up to:
 *
 *   11 instructions/13 words/2 branches/3 registers
 *
 * to accomplish this plus some arbitrary operation it shoves in the delay slot
 * of both branches. Save a handful of bytes of precious local RAM and
 * demonstrate how to insert inline assembly by hand-coding this small snippet.
 *
 * Details:
 *
 * volatile = Tell compiler not to optimize code ins this inline assembly block
 *
 * .wloop%=: = A local label named '.wloop<something>', where <something> is a
 *             unique identifier for each use of "asm", meaning each time this
 *             macro is used it will generate a unique label name. This ensures
 *             the label is unique even if the macro is used multiple times
 *             within the same function/local assembly scope.
 *
 * The three ':' lines at the end are the parameter declaration portion of the
 * inline assembly statement and have the following format:
 *
 * : input parameters    - None
 * : input parameters    - None
 * : clobbered registers - r0 and the condition codes/flags internal register
 *
 * These allow the user to bind C labels/variables to arbitrary or specific
 * registers, and to let the compiler know which registers will be clobbered/
 * invalidated by this inline assembly code block.
 *
 * Special trick for the JRISC gcc compiler: We can use the "TMP" register,
 * which the JRISC gcc compiler defines to be r16 via a .REGEQU statement it
 * inserts at the top of each assembly listing it generates, without marking it
 * as a clobbered register, because it's specifically reserved for local usage
 * of this type by the compiler anyway. The compiler itself only emits usage of
 * TMP in self-contained assembly blocks that won't interfere with our own
 * self-contained, non-optimizable inline assembly block. That said, it probably
 * wouldn't hurt anything to include it in the clobber list anyway.
 *
 * Full syntax details here:
 *
 *   https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html
 */
                                                        /* #B_CMD */
#define BLITTER_WAIT() asm volatile("              movei   #$F02238, TMP\n"    \
                                    ".wloop%=:     load    (TMP), r0\n"        \
                                    "              btst    #0, r0\n"           \
                                    "              jr      EQ, .wloop%=\n"     \
                                    "              nop\n"                      \
                                    :                                          \
                                    :                                          \
                                    : "r0", "cc")

/* This has to be the first function, since gcc sets up the stack in the first function */
void gpu_start(void)
{
    /* Stuff that the C startup code would normally do */
    cpuCmd = CPUCMD_IDLE;
    cpuData0 = 0;

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

static inline void draw_string(const Sprite *sprite,
                               unsigned int frameNum,
                               unsigned long coords,
                               void *str)
{
    draw_string_off(sprite, coords, str, calc_frame_offset(sprite->frameSize, frameNum));
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
    BLITTER_WAIT();

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
    cpuData0 = music;
    cpuCmd = CPUCMD_CHANGE_MUSIC;

    run68kCmd();
}

static void SetSpriteList(Sprite *list)
{
    cpuData0 = list;
    cpuCmd = CPUCMD_SET_SPRITE_LIST;

    run68kCmd();
}

static void int_to_str_gpu(char *str, unsigned int val)
{
    cpuData0 = str;
    cpuData1 = (void *)val;
    cpuCmd = CPUCMD_INT_TO_STR;

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

static const unsigned int GRID_START_X = 40;
static const unsigned int GRID_START_Y = 40;

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
        blit_rect(screen, frame, GRID_CLR, PACK_XY(GRID_START_X, GRID_START_Y + SHORT_MUL(j, GRID_SIZE_Y)), SHORT_MUL(GRID_BOXES_X, GRID_SIZE_X), 1);
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
     * -As noted in comments above, its math is always off by one for such loops
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
                                GRID_START_Y + SHORT_MUL(i, GRID_SIZE_Y) + 12), tmp_str);
        }
    }
}

static const unsigned int SCREEN_OFF_X = 16; /* Copied from InitLister logic, NTSC version for 320 x 240 bitmap */
static const unsigned int SCREEN_OFF_Y = 13; /* Copied from InitLister logic, NTSC version for 320 x 240 bitmap */

static void myclamp(int *val, int min, int max) {
    if (*val < min) *val = min;
    if (*val > max) *val = max;
}

static void gpu_main(void)
{
    unsigned int i;
    unsigned int oldTicks;
    unsigned int bg_color = 0x0; /* Black */
    unsigned int oldPad1 = 0;
    unsigned int newPad1;
    unsigned int printDelay = 0;
    int newMusic = 0;
    Sprite *screen = &spriteData[0];
    Sprite *player = &spriteData[1];
    int player_x = 0;
    int player_y = 0;
    unsigned int sprite_frame = 1;
    unsigned int draw_debug = 0;
    unsigned int num_multiples_remaining;
    char *end_str = win_str;
    Animation *a, *aLocal;

    animations = NULL;

    make_sprite(screen,
                screenbmp,
                BMP_PHRASES * 4,
                BMP_HEIGHT,
                SPRITE_DEPTH16,
                SPRITE_SINGLE_BUFFERED | SPRITE_NOT_TRANSPARENT);

    SET_SPRITE_X(screen, SCREEN_OFF_X);
    SET_SPRITE_Y(screen, SCREEN_OFF_Y);

    make_sprite(player,
                jagcrunchbmp,
                PLAYER_WIDTH,
                PLAYER_HEIGHT,
                SPRITE_DEPTH16,
                SPRITE_SINGLE_BUFFERED | SPRITE_TRANSPARENT);

    SET_SPRITE_X(player, 16 + GRID_START_X);
    SET_SPRITE_Y(player, 13 + GRID_START_Y);

    screen->next = player;

    SetSpriteList(screen);

    num_multiples_remaining = pick_numbers(m2_vals, 2);

    /*
     * Should be:
     *   for (i = 0; i < 2; i++)
     * But that only iterates once because the compiler is buggy
     */
    init_screen(screen, 0, bg_color);
    init_screen(screen, 1, bg_color);

    /* Wait for blitter to idle */
    BLITTER_WAIT();

    while (num_multiples_remaining) {
        oldTicks = ticks;
        while (ticks == oldTicks);

        update_animations();

        sprite_frame = !sprite_frame;

        /* Clear text regions */
        if (draw_debug) {
            /* Debug text */
            blit_rect(screen, sprite_frame, bg_color, PACK_XY(100, 210), 200, 20);
        }

        /* Score */
        blit_rect(screen, sprite_frame, bg_color, PACK_XY(GRID_START_X + 42, GRID_START_Y + SHORT_MUL(5, PLAYER_HEIGHT) + 7), 56, 13);

        /* Wait for blitter to idle */
        BLITTER_WAIT();

        if (draw_debug) {
            draw_string(screen, sprite_frame, PACK_XY(100, 210), gpu_str);
            draw_string(screen, sprite_frame, PACK_XY(100, 220), dsp_str);
        }
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

        if (((oldPad1 ^ newPad1) & newPad1) & U235SE_BUT_0) {
            draw_debug ^= 1;
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
            a->sprite = player;
            a->endX = SCREEN_OFF_X + GRID_START_X + SHORT_MUL(player_x, GRID_SIZE_X);
            a->endY = SCREEN_OFF_Y + GRID_START_Y + SHORT_MUL(player_y, GRID_SIZE_Y);
            a->speedPerTick = 4;
            a->next = animations;
            animations = a;
        }

        oldPad1 = *u235se_pad1;

        if (++printDelay >= 30) {
            printDelay = 0;
            cpuCmd = CPUCMD_PRINT_STATS;
            run68kCmd();
        }
    }

    /* Display a super fancy victory or game over screen */
    blit_color(screen, sprite_frame, bg_color);
    BLITTER_WAIT();
    draw_string(screen, sprite_frame, PACK_XY(40, 110), end_str);
}
