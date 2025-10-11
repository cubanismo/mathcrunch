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
#if defined(USE_SKUNK)
#include <skunk.h>
#elif defined(USE_GD)
#include "gdbios.h"
#endif

#include "startup.h"
#include "gpu_68k_shr.h"
#include "music.h"
#include "u235se.h"
#include "sprintf.h"
#include "sprites.h"

volatile unsigned long spinCount;
volatile unsigned long blitCount;

volatile unsigned long score;

volatile unsigned long level_num;

Animation animationData[4];
Animation *animations;

unsigned long count;

unsigned long *mult_vals;
unsigned long multiple_of;

#if defined(USE_GD)
static u8 GD_Bios[1024 * 4];
#endif

SquareData square_data[GRID_BOXES_Y][GRID_BOXES_X];
char tmp_str[4] = "24";

unsigned long *m_vals[] = {
    &m2_vals[0],
    &m3_vals[0],
    &m4_vals[0],
    &m5_vals[0],
    &m6_vals[0],
    &m7_vals[0],
    &m8_vals[0],
    &m9_vals[0],
};

static void blitToGpu(void *dst, void *src, unsigned long size)
{
    printf("Blitting GPU code from 0x%08x size 0x%08x to 0x%08x\n", (long)src, size, (long)dst);
    while ((*(volatile long *)B_CMD & 1) == 0);

    /* Use 32-bit version of GPU memory */
    dst = (unsigned char *)dst + 0x8000;

    *A1_CLIP = 0; // Don't clip blitter writes
    *A1_BASE = (unsigned long)dst;
    *A2_BASE = (unsigned long)src;

    *A1_FLAGS = XADDPHR|PIXEL32|WID2048|PITCH1;
	*A2_FLAGS = XADDPHR|PIXEL32|WID2048|PITCH1;
    *A1_PIXEL = 0;
    *A2_PIXEL = 0;

    *B_COUNT = ((size + 3) >> 2) | (0x1 << 16);

    *B_CMD = SRCEN|UPDA1|UPDA2|LFU_REPLACE;

    while ((*(volatile long *)B_CMD & 1) == 0);
}

void printStats(void)
{
    sprintf(gpu_str, "GPU $%08lx $%08lx $%08lx",
            *(volatile long *)G_PC, *(volatile long *)G_FLAGS, *(volatile long *)G_CTRL);
    sprintf(dsp_str, "DSP $%08lx $%08lx $%08lx",
            *(volatile long *)D_PC, *(volatile long *)D_FLAGS, *(volatile long *)D_CTRL);
    printf("%s\n%s\n", gpu_str, dsp_str);
}

void intToStr(char *str, unsigned long int val)
{
    sprintf(str, "%u", val);
}

void doSplash(const unsigned char *splashbmp) {
    unsigned long oldTicks;

    /* Idle the blitter */
    while ((*(volatile long *)B_CMD & 1) == 0);

    /* Wait until vblank */
    oldTicks = ticks;
    while (ticks == oldTicks);

    *(volatile long *)A1_BASE = (unsigned long)screenbmp;
    /* NOTE: Assumes BMP_WIDTH is phrase-aligned */
    *(volatile long *)A1_FLAGS = XADDPHR|WID320|PITCH1|PIXEL16;
    *(volatile long *)A1_PIXEL = 0;
    *(volatile long *)A1_STEP = (1 << 16) | (-BMP_WIDTH & 0xffff);
    *(volatile long *)A2_BASE = (unsigned long)splashbmp;
    *(volatile long *)A2_FLAGS = XADDPHR|WID320|PITCH1|PIXEL16;
    *(volatile long *)A2_PIXEL = 0;
    *(volatile long *)A2_STEP = (1 << 16) | (-BMP_WIDTH & 0xffff);

    *(volatile long *)B_COUNT = (BMP_HEIGHT << 16) | BMP_WIDTH;
    *(volatile long *)B_CMD = SRCEN|UPDA1|UPDA2|LFU_REPLACE;

    /* Set oldTicks to 5 seconds from when this function was entered */
    oldTicks += 300;

    /* Wait 5 seconds */
    while (ticks < oldTicks);
}

int start()
{
	volatile	long	*pc=(void *)G_PC;
	volatile	long	*ctrl=(void *)G_CTRL;
    long unsigned musicAddr;

    level_num = 0;

    spinCount = blitCount = 0;
    gpu_str[0] = 'G';
    gpu_str[1] = 'P';
    gpu_str[2] = 'U';
    gpu_str[3] = '\0';

    dsp_str[0] = 'D';
    dsp_str[1] = 'S';
    dsp_str[2] = 'P';
    dsp_str[3] = '\0';

#if defined(USE_SKUNK)
	skunkRESET();
	skunkNOP();
	skunkNOP();
#elif defined(USE_GD)
    GD_Install(GD_Bios);
#endif

    doSplash(u235sebmp);
    doSplash(titlebmp);

    blitToGpu(G_RAM, gpugame_start, (long)gpugame_size);
    blitToGpu(gpuasm_dst, gpuasm_start, (long)gpuasm_size);
    printf("Done blitting GPU code\n");

    while (++level_num < 9) {
        sprintf(levelnum_str, "%u", level_num);
        sprintf(levelname_str, "Multiples of %u", level_num + 1);
        if (level_num == 1) {
            score = 0;
            musicAddr = ChangeMusic(mus_title);
            printf("Starting music at 0x%08x\n", musicAddr);
        }
        intToStr(scoreval_str, score);

        mult_vals = m_vals[level_num - 1];
        multiple_of = level_num + 1;
        gpu_running = 1;

        *pc = (unsigned long)&gpu_start;
        *ctrl = GPUGO;

        while (gpu_running) {
            stop68k();
        }
    }

    /* The user won */
    while (1);
}
