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

extern Sprite spriteData[];
extern Animation animationData[];
extern Animation *animations;

/* sprite graphics */
extern unsigned char jagcrunchbmp[];

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
