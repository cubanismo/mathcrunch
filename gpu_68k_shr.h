#ifndef GPU_68K_SHR_H_
#define GPU_68K_SHR_H_

enum CpuCommands {
    CPUCMD_IDLE = 0,
    CPUCMD_PRINT_STATS,
    CPUCMD_CHANGE_MUSIC,
    CPUCMD_SET_SPRITE_LIST,

    CPUCMD_INVALID = 0xFFFFFFFF
};

extern volatile unsigned long cpuCmd;
extern void *volatile cpuData;

extern volatile unsigned long spinCount;
extern volatile unsigned long blitCount;
extern unsigned long gpugame_start[];
extern unsigned long gpugame_end[];
extern unsigned long gpugame_size[];
extern unsigned long gputext_start[];
extern unsigned long gputext_end[];
extern unsigned long gputext_size[];

extern void gpu_start(void);

#endif /* GPU_68K_SHR_H_ */
