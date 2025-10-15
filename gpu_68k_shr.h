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

#ifndef GPU_68K_SHR_H_
#define GPU_68K_SHR_H_

enum CpuCommands {
    CPUCMD_IDLE = 0,
    CPUCMD_PRINT_STATS,
    CPUCMD_CHANGE_MUSIC,
    CPUCMD_SET_SPRITE_LIST,
    CPUCMD_INT_TO_STR,
    CPUCMD_STOP_GPU,
    CPUCMD_PLAY_SOUND,

    CPUCMD_INVALID = 0xFFFFFFFF
};

/* If this struct layout is changed, update ASM in gpuasm.s */
typedef struct {
    unsigned long val;
    unsigned long is_multiple;
    unsigned long is_visible;
} SquareData;

typedef struct {
    void *codeSrc;
    void *codeDst;
    unsigned long codeSize;
} GpuCode;

typedef struct {
    GpuCode cCode;
    GpuCode asmCode;
    void (*startFunc)(void);
} GpuOverlay;

/* If these are changed, update ASM in gpuasm.s */
#define GRID_BOXES_X (6)
#define GRID_BOXES_Y (5)

/* main.c */
extern volatile unsigned long spinCount;
extern volatile unsigned long blitCount;
extern volatile unsigned long score;
extern volatile unsigned long level_num;
extern SquareData square_data[GRID_BOXES_Y][GRID_BOXES_X];
extern char tmp_str[];
extern unsigned long *mult_vals;
extern unsigned long multiple_of;
extern unsigned int num_multiples_remaining;
extern void (*gpu_main)(void);

/* gpugame.c */
extern volatile unsigned long cpuCmd;
extern void *volatile cpuData0;
extern void *volatile cpuData1;

extern unsigned long gpucommon_start[];
extern unsigned long gpucommon_end[];
extern unsigned long gpucommon_size[];
extern unsigned long gpucommon_loc[];
extern unsigned long gpulevelinit_start[];
extern unsigned long gpulevelinit_end[];
extern unsigned long gpulevelinit_size[];
extern unsigned long gpulevelinit_loc[];
extern unsigned long gpuplaylevel_start[];
extern unsigned long gpuplaylevel_end[];
extern unsigned long gpuplaylevel_size[];
extern unsigned long gpuplaylevel_loc[];
extern unsigned long gpuasm_start[];
extern unsigned long gpuasm_end[];
extern unsigned long gpuasm_size[];
extern unsigned long gpuasm_loc[];

extern void gpu_start(void);
extern void levelinit(void);
extern void playlevel(void);

#endif /* GPU_68K_SHR_H_ */
