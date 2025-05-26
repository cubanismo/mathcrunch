#ifndef GPUASM_H_
#define GPUASM_H_

extern void draw_string_off(const Sprite *sprite,
                            unsigned long coords,
                            void *str,
                            unsigned long frame_offset);
extern void update_animations(void);
extern unsigned int get_rand_entry(const unsigned long *val_array);

#endif /* GPUASM_H_ */
