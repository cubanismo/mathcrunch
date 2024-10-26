#ifndef GPU_68K_SHR_H_
#define GPU_68K_SHR_H_

extern volatile unsigned long spinCount;
extern volatile unsigned long blitCount;
extern unsigned long gpugame_start[];
extern unsigned long gpugame_end[];
extern unsigned long gpugame_size[];

extern void gpu_start(void);

#endif /* GPU_68K_SHR_H_ */
