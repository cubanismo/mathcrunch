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
