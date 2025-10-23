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

#ifndef STARTUP_H_
#define STARTUP_H_

extern volatile unsigned long ticks;
extern unsigned char screenbmp[];
extern char gpu_str[];
extern char dsp_str[];
extern char level_str[];
extern char levelnum_str[];
extern char levelname_str[];
extern char score_str[];
extern char scoreval_str[];
extern char win_str[];
extern char lose_str[];
extern char eaten_str[];
extern char press_c_str[];
extern unsigned long m2_vals[];
extern unsigned long m3_vals[];
extern unsigned long m4_vals[];
extern unsigned long m5_vals[];
extern unsigned long m6_vals[];
extern unsigned long m7_vals[];
extern unsigned long m8_vals[];
extern unsigned long m9_vals[];
extern unsigned long rnd;
extern unsigned long rnd_seed;
extern short screen_off_x;
extern short screen_off_y;
extern volatile unsigned long gpu_running;

/* XXX Should be shared with startup.s */
#define PPP 4
#define BMP_WIDTH 320
#define BMP_HEIGHT 240
#define BMP_PHRASES (BMP_WIDTH/PPP)
#define BMP_LINES (BMP_HEIGHT*2)

#define PLAYER_WIDTH 40
#define PLAYER_HEIGHT 32

extern unsigned short int GetU235SERand(void);
extern long unsigned ChangeMusic(void *modFilePtr);
extern void printStats(void);
extern void stop68k(void);

#endif /* STARTUP_H_ */
