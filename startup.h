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
extern unsigned long m2_vals[];
extern short screen_off_x;
extern short screen_off_y;

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
