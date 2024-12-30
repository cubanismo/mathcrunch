#ifndef GPUASM_H_
#define GPUASM_H_

extern void draw_string_off(const Sprite *sprite,
                            unsigned long coords,
                            void *str,
                            unsigned long frame_offset);
extern void update_animations(void);

#endif /* GPUASM_H_ */
