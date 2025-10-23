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

#ifndef GPUASM_H_
#define GPUASM_H_

#include "sprites.h"

extern void draw_string_off(const Sprite *sprite,
                            unsigned long coords,
                            void *str,
                            unsigned long frame_offset);
extern void update_animations(void);
extern unsigned int pick_numbers(const unsigned long *val_array,
                                 unsigned int multiple_of);

static inline void draw_string(const Sprite *sprite,
                               unsigned int frameNum,
                               unsigned long coords,
                               void *str)
{
    draw_string_off(sprite, coords, str, calc_frame_offset(sprite->frameSize, frameNum));
}

#endif /* GPUASM_H_ */
