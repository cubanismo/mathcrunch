#ifndef STARTUP_H_
#define STARTUP_H_

extern volatile unsigned long ticks;
extern unsigned char screenbmp[];

/* XXX Should be shared with startup.s */
#define PPP 4
#define BMP_WIDTH 320
#define BMP_HEIGHT 240
#define BMP_PHRASES (BMP_WIDTH/PPP)
#define BMP_LINES (BMP_HEIGHT*2)

#endif /* STARTUP_H_ */
