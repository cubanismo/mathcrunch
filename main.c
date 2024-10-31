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

    while ((*B_CMD & 1) == 0);
}

void printStats(void)
{
    sprintf(gpuStr, "GPU $%08lx $%08lx $%08lx",
            *(volatile long *)G_PC, *(volatile long *)G_FLAGS, *(volatile long *)G_CTRL);
    sprintf(dspStr, "DSP $%08lx $%08lx $%08lx",
            *(volatile long *)D_PC, *(volatile long *)D_FLAGS, *(volatile long *)D_CTRL);
    printf("%s\n%s\n", gpuStr, dspStr);
}

int start()
{
	volatile	long	*pc=(void *)G_PC;
	volatile	long	*ctrl=(void *)G_CTRL;
    long unsigned musicAddr;

    spinCount = blitCount = 0;
    gpuStr[0] = 'G';
    gpuStr[1] = 'P';
    gpuStr[2] = 'U';
    gpuStr[3] = '\0';

    dspStr[0] = 'D';
    dspStr[1] = 'S';
    dspStr[2] = 'P';
    dspStr[3] = '\0';

#if defined(USE_SKUNK)
	skunkRESET();
	skunkNOP();
	skunkNOP();
#elif defined(USE_GD)
    GD_Install(GD_Bios);
#endif

    printf("Running blit to GPU\n");

    blitToGpu(G_RAM, gpugame_start, (long)gpugame_size);
    blitToGpu((unsigned char *)G_RAM + 0x800, gputext_start, (long)gputext_size);
    printf("Done. spinCount = %u\n", spinCount);

    *pc = (unsigned long)&gpu_start;
    *ctrl = GPUGO;

    musicAddr = ChangeMusic(mus_title);
    printf("Starting music at 0x%08x\n", musicAddr);

    while (1) {
        stop68k();
    }
}
