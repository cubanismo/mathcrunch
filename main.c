#include "jaguar.h"
#include "gpu_68k_shr.h"
#if defined(USE_SKUNK)
#include "skunk.h"
#endif
#include "sprintf.h"

extern volatile unsigned long ticks;
volatile unsigned long spinCount;

unsigned long count;

void blitToGpu(void *dst, void *src, unsigned long size)
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

int start()
{
	volatile	long	*pc=(void *)G_PC;
	volatile	long	*ctrl=(void *)G_CTRL;

    spinCount = 0;

#if defined(USE_SKUNK)
	skunkRESET();
	skunkNOP();
	skunkNOP();
#endif

    printf("Running blit to GPU\n");

    blitToGpu(G_RAM, gpugame_start, (long)gpugame_size);
    printf("Done. spinCount = %u\n", spinCount);

    *pc = (unsigned long)&gpu_main;
    *ctrl = GPUGO;

    while (1) {
        unsigned long oldTicks = ticks;

        while ((ticks - oldTicks) < 60);

        printf("spinCount = %u G_PC = 0x%08lx\n", spinCount, (unsigned long)*pc);
    }
}
