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
    unsigned long int frontIdx;                 /* Off 28 */
    unsigned long int surfAddr;                 /* Off 32 */
    unsigned long int blitterFlags;             /* Off 36 */
} Sprite; /* Size = 40 */

extern Sprite spriteData[];

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

#define SET_SPRITE_X(s_, x_) (s_)->x = (x_)
#define SET_SPRITE_Y(s_, y_) (s_)->y = ((y_) << 4)

#endif /* SPRITES_H_ */
