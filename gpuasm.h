#ifndef GPUASM_H_
#define GPUASM_H_

extern void drawStringOff(const Sprite *sprite,
                          unsigned long coords,
                          void *str,
                          unsigned long frame_offset);
extern void update_animations(void);

#endif /* GPUASM_H_ */
