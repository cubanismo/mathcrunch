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

#ifndef SPRITES_H_
#define SPRITES_H_

typedef struct Sprite {
    struct Sprite *next;                        /* Off 0 */
    unsigned long int firstPhraseLowTemplate;   /* Off 4: height */
    unsigned long int firstPhraseHighTemplate;  /* Off 8: data/address */
    unsigned long int secondPhraseLowTemplate;  /* Off 12: depth, pitch, dwidth, iwidth */
    unsigned long int secondPhraseHighTemplate; /* Off 16: iwidth, trans */
    unsigned long int x;                        /* Off 20 */
    unsigned long int y;                        /* Off 24 */
    unsigned long int frameSize;                /* Off 28 */
    unsigned long int surfAddr;                 /* Off 32 */
    unsigned long int blitterFlags;             /* Off 36 */
} Sprite; /* Size = 40 */

typedef struct Animation {
    struct Animation *next;                     /* Off 0 */
    Sprite *sprite;                             /* Off 4 */

    unsigned int speedPerTick;                  /* Off 8 */

    unsigned int endX;                          /* Off 12 */
    unsigned int endY;                          /* Off 16 */
} Animation; /* Size = 20 */

typedef struct Timer {
    struct Timer *next;
    struct Animation *animation;
    unsigned int endTick;
} Timer;

extern Sprite spriteData[];
extern Animation animationData[];
extern Animation *animations;
extern Timer timerData[];
extern Timer *timers;

/* sprite graphics */
extern unsigned char jagcrunchbmp[];
extern unsigned char u235sebmp[];
extern unsigned char titlebmp[];
extern unsigned char *spritebmps[];

/* sprite depths for make_sprite. Based on O_DEPTH* in jaguar.inc */
#define SPRITE_DEPTH32 5
#define SPRITE_DEPTH16 4
#define SPRITE_DEPTH8  3
#define SPRITE_DEPTH4  2
#define SPRITE_DEPTH2  1
#define SPRITE_DEPTH1  0

/* sprite flags for make_sprite */
#define SPRITE_NOT_TRANSPARENT  (0)
#define SPRITE_TRANSPARENT      (1)
#define SPRITE_SINGLE_BUFFERED  (0 << 1)
#define SPRITE_DOUBLE_BUFFERED  (1 << 1)

#define SET_SPRITE_X(s_, x_) (s_)->x = (unsigned long int)(x_)
#define SET_SPRITE_Y(s_, y_) (s_)->y = ((unsigned long int)(y_) << 4)

#define GET_SPRITE_X(s_) ((s_)->x)
#define GET_SPRITE_Y(s_) ((s_)->y >> 4)

#define PACK_XY(x_, y_) (((y_) << 16) | (x_))

#endif /* SPRITES_H_ */
