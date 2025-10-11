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

#ifndef GPUCOMMON_H_
#define GPUCOMMON_H_

#include "startup.h"
#include "sprites.h"

#define NULL ((void *)0)
#define GRID_SIZE_X (PLAYER_WIDTH)
#define GRID_SIZE_Y (PLAYER_HEIGHT)

#define GRID_MAX_X (5)
#define GRID_MAX_Y (4)

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

extern unsigned int calc_frame_offset(unsigned int frameSize, unsigned int frameNum);

extern void blit_rect(
    const Sprite *dst,
    unsigned int frame_num,
    unsigned int color,
    unsigned int y_x,
    unsigned int width,
    unsigned int height
);

extern void blit_color(const Sprite *dst, unsigned int frame_num, unsigned int color);

extern void run68kCmd(unsigned int cmd);

extern  void gpu_main(void);

#endif /* GPUCOMMON_H_ */
