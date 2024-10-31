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

volatile unsigned long spinCount;
volatile unsigned long blitCount;

unsigned long count;

#if defined(USE_GD)
static u8 GD_Bios[1024 * 4];
#endif

static void blitToGpu(void *dst, void *src, unsigned long size)
{
    while ((*B_CMD & 1) == 0);

    *A1_CLIP = 0; // Don't clip blitter writes
    *A1_BASE = (unsigned long)dst;
    *A2_BASE = (unsigned long)src;

    *A1_FLAGS = XADDPHR|PIXEL32|WID2048|PITCH1;
	*A2_FLAGS = XADDPHR|PIXEL32|WID2048|PITCH1;
    *A1_PIXEL = 0;
    *A2_PIXEL = 0;

    *B_COUNT = ((size + 3) >> 2) | (0x1 << 16);

    *B_CMD = SRCEN|UPDA1|UPDA2|LFU_REPLACE;

    while ((*B_CMD & 1) == 0);
}

void printStats(void)
{
    printf("spinCount = %u blitCount = %u B_CMD = 0x%08lx G_PC = 0x%08lx G_FLAGS = 0x%08lx G_CTRL = 0x%08lx D_PC = 0x%08lx D_FLAGS = 0x%08lx D_CTRL = 0x%08lx\n",
           spinCount, blitCount, *(volatile long *)B_CMD, *(volatile long *)G_PC, *(volatile long *)G_FLAGS, *(volatile long *)G_CTRL,
           *(volatile long *)D_PC, *(volatile long *)D_FLAGS, *(volatile long *)D_CTRL);
}

int start()
{
	volatile	long	*pc=(void *)G_PC;
	volatile	long	*ctrl=(void *)G_CTRL;
    long unsigned musicAddr;

    spinCount = blitCount = 0;

#if defined(USE_SKUNK)
	skunkRESET();
	skunkNOP();
	skunkNOP();
#elif defined(USE_GD)
    GD_Install(GD_Bios);
#endif

    printf("Running blit to GPU\n");

    blitToGpu(G_RAM, gpugame_start, (long)gpugame_size);
    printf("Done. spinCount = %u\n", spinCount);

    *pc = (unsigned long)&gpu_start;
    *ctrl = GPUGO;

    musicAddr = ChangeMusic(mus_title);
    printf("Starting music at 0x%08x\n", musicAddr);

    while (1) {
        stop68k();
    }
}
