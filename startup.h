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
extern char press_b_str[];
extern unsigned long m2_vals[];
extern unsigned long m3_vals[];
extern unsigned long m4_vals[];
extern unsigned long m5_vals[];
extern unsigned long m6_vals[];
extern unsigned long m7_vals[];
extern unsigned long m8_vals[];
extern unsigned long m9_vals[];
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

extern long unsigned ChangeMusic(void *modFilePtr);
extern void printStats(void);
extern void stop68k(void);

#endif /* STARTUP_H_ */
