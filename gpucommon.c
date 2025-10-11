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

#include "gpu_68k_shr.h"
#include "sprites.h"

/* When defined to 1, debug statistics are printed on-screen */
#define DEBUG_STATS 0

volatile unsigned long cpuCmd;
void *volatile cpuData0;
void *volatile cpuData1;

/* This has to be the first function, since gcc sets up the stack in the first function */
void gpu_start(void)
{
    /* Stuff that the C startup code would normally do */
    cpuCmd = CPUCMD_IDLE;
    cpuData0 = 0;

    gpu_main();
}

unsigned int calc_frame_offset(unsigned int frameSize, unsigned int frameNum)
{
    unsigned int result = 0;

    /*
     * JRISC only has a 16-bit multiply. Implement a really naive 32-bit multiply
     * that will be good enough for small values of frameNum.
     */
    while (frameNum--) result += frameSize;

    return result;
}

void blit_rect(
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

void blit_color(const Sprite *dst, unsigned int frame_num, unsigned int color)
{
    blit_rect(dst, frame_num, color, 0, BMP_PHRASES * 4, BMP_HEIGHT);
}

void run68kCmd(unsigned int cmd)
{
    static volatile long *gctrl = G_CTRL;

    /* First convey the command */
    cpuCmd = cmd;

    /* Interrupt the 68k */
    *gctrl = GPUGO | 0x2;

    if (cmd == CPUCMD_STOP_GPU) {
        *gctrl = 0;
        asm volatile ("     nop\n"
                      "     nop\n");
    } else {
        while (cpuCmd != 0);
    }
}
