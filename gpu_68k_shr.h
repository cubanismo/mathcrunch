#ifndef GPU_68K_SHR_H_
#define GPU_68K_SHR_H_

enum CpuCommands {
    CPUCMD_IDLE = 0,
    CPUCMD_PRINT_STATS,
    CPUCMD_CHANGE_MUSIC,
    CPUCMD_SET_SPRITE_LIST,
    CPUCMD_INT_TO_STR,

    CPUCMD_INVALID = 0xFFFFFFFF
};

/* If this struct layout is changed, update ASM in gpuasm.s */
typedef struct {
    unsigned long val;
    unsigned long is_multiple;
} SquareData;

/* If these are changed, update ASM in gpuasm.s */
#define GRID_BOXES_X (6)
#define GRID_BOXES_Y (5)

/* main.c */
extern volatile unsigned long spinCount;
extern volatile unsigned long blitCount;
extern volatile unsigned long score;
extern SquareData square_data[GRID_BOXES_Y][GRID_BOXES_X];
extern char tmp_str[];

/* gpugame.c */
extern volatile unsigned long cpuCmd;
extern void *volatile cpuData0;
extern void *volatile cpuData1;

extern unsigned long gpugame_start[];
extern unsigned long gpugame_end[];
extern unsigned long gpugame_size[];
extern unsigned long gpuasm_start[];
extern unsigned long gpuasm_end[];
extern unsigned long gpuasm_size[];
extern unsigned long gpuasm_dst[];

extern void gpu_start(void);

#endif /* GPU_68K_SHR_H_ */
