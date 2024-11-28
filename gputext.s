		.include "jaguar.inc"
		.include "g_gpugame_codesize.inc"

GPUCODE_OFFSET	.equ	(GPUGAME_CODESIZE + 7) & ~7	; Phrase-align this file's code

		.globl	_drawStringOff
		.globl	_gputext_start
		.globl	_gputext_end
		.globl	_gputext_size
		.globl	_gputext_dst
		.globl	_update_animations

		.extern	_animations

		.text
		.dphrase
_gputext_start:
		.gpu
		.org	(G_RAM + GPUCODE_OFFSET)
_gputext_dst:

; These are all hard-coded from the clr6x12.jft font for now.
CHR_WIDTH	.equ	6
CHR_HEIGHT	.equ	12
FNTWIDTH	.equ	WID96				; WID768 / 8 per comment in drawstring
FNTFIRSTCHR	.equ	$20
FNTLASTCHR	.equ	$7f
zeror		.equr	r6

_drawStringOff:	; Write a NUL-terminated string to a surface
		;  r0 = sprite:      The sprite surface to draw to
		;  r1 = coords:      The packed coordinates (y<<16)|x
		;  r2 = stringaddr:  Pointer to the NUL-terminated string
		;  r3 = frameoffset: Offset from sprite base address for current frame
		;  fontdata:   The 1bpp font surface
		moveq	#0, zeror			; 0 will be stored in various fields

		move	r0, r14

		movei	#A1_BASE, r11

		load	(r14+8), r0			; Get surface address in r0

		movei	#$ffffffff, r4

		add	r3, r0				; Add frame offset into r0

		movei	#B_PATD, r5
		movei	#A1_CLIP, r3

		store	r0, (r11)			; Store A1_BASE (destination addr)
		store	r4, (r5)			; Store white in B_PATD low dword
		addq	#4, r5				; Point r5 at 2nd dword of B_PATD
		store	zeror, (r3)			; Store 0 in A1_CLIP (No clipping)
		store	r4, (r5)			; Store white in B_PATD high dword

		movei	#XADDPIX|YADD0, r12
		load	(r14+9), r4			; Get sprite blitter flags in r4
		movei	#A1_FLAGS, r5

		movei	#A1_FPIXEL, r7
		or	r12, r4

		; Add (-CHR_WIDTH, 1) to dst x, y pointers after each inner loop iter
		movei	#(1<<16)|((-CHR_WIDTH)&$ffff), r8
		movei	#A1_STEP, r9
		movei	#A1_FSTEP, r10

		store	r4, (r5)			; Store A1_FLAGS
		store	zeror, (r7)			; Store 0 in A1_FPIXEL
		store	r8, (r9)			; Store (-CHR_WIDTH, 1) in A1_STEP
		store	zeror, (r10)			; Store 0 in A1_FSTEP

		movei	#fontdata, r4
		movei	#A2_BASE, r5
		; Use PIXEL8 even though we're reading 1bit pixels. This, combined
		; with the below SRCENX + !SRCEN command, FNTWIDTH set to actual
		; divided by 8, fixed A2_STEP of -1, 1, and blit width of at most 8
		; is a trick to get the blitter to:
		;  1) Read 8 pixels of data at a time (PIXEL8 instead of PIXEL1)
		;  2) Read source data only once per inner loop iteration (SRCENX)
		;  3) Do bit->pixel expansion even with <8bpp dst pixels
		; which allows using the bit comparator even for 1/2/4bpp dst
		; pixels. Note for fonts with a character width >8, this trick would
		; require multiple blits per character, since the inner loop must be
		; clamped to 8 bits/1 byte given source data is only read once. This
		; case is not implemented here.
		movei	#PITCH1|PIXEL8|FNTWIDTH|XADDPIX|YADD0, r0
		movei	#A2_FLAGS, r7
		movei	#(1<<16)|((-1)&$ffff), r8
		movei	#A2_STEP, r9

		store	r4, (r5)			; Store fontdata in A2_BASE
		store	r0, (r7)			; Store A2_FLAGS
		store	r8, (r9)			; Store (-1, 1) in A2_STEP

		movei	#A1_PIXEL, r12
		movei	#(CHR_HEIGHT<<16)|CHR_WIDTH, r3
		movei	#B_COUNT, r4

		; The blit loop always starts by adding CHR_WIDTH to the dst
		; position. Subtract CHR_WIDTH from the initial position to account
		; for this.
		subq	#CHR_WIDTH, r1

		; r5 = B_CMD value. Notes:
		;  -1bit pixels need DSTEN,
		;  -SRCENX means read a pixel from src on first inner loop iteration
		;  -!SRCEN means no pixels read on other inner loop iterations
		;  -BCOMPEN uses bitmask to decide which pixels to actually write
		movei	#SRCENX|DSTEN|UPDA1|UPDA2|PATDSEL|BCOMPEN, r5;
		movei	#B_CMD, r0
		movei	#FNTFIRSTCHR, r8	; Load first char idx of font in r8
		movei	#FNTLASTCHR, r9		; Load last char idx of font in r9
		movei	#A2_PIXEL, r10

		jr		.nextchr			; Jump into loop
		xor		r11, r11			; clear r11

.blitloop:	store	r7, (r10)			; Store src pixel loc in A2_PIXEL
		store	r1, (r12)			; Store dst pixel loc in A1_PIXEL
		store	r3, (r4)			; Write loop dimensions to B_COUNT
		store	r5, (r0)			; Write op to B_CMD

.nextchr:	addq	#CHR_WIDTH, r1		; Add CHR_WIDTH to dst pixel location
		loadb	(r2), r7			; Load next character
		cmpq	#0, r7				; At NUL terminator?
		jr		EQ, .waitlast		; if yes, wait for the last blit
		cmp		r9, r7				; Compare to last char
		addqt	#1, r2				; Always advance string pointer
		jr		HI, .nextchr		; If chr out of range, leave blank space
		sub		r8, r7				; Subtract font first chr from character
		jr		MI, .nextchr		; If chr out of range, leave blank space
		nop
.waitblit:	btst	#0, r11				; See if bit 0 is set
		jr		NE, .blitloop		; If done, next iteration
		xor		r11, r11			; Clear r11 for next iteration
		jr		.waitblit			; Else, keep waiting

.waitlast:	; Done. Wait for the last blit...
		load	(r0), r11			; In both loops: Read back blit status
		btst	#0, r11				; See if bit 0 is set
		jr		EQ, .waitlast
		nop					; Don't signal completion while spinning

.done:		; Return to the caller.
		load	(r31),r16
		jump	(r16)
		addqt	#4,r31	;rts

; This function is a manually-commented version of the Linux gcc263 compiler
; output given the following C code:
;
; static void update_animations(void)
; {
;     int done;
;     int tmpX, tmpY;
;     Animation *a, *prevA;
;
;     for (a = animations; a; a = a->next) {
;         tmpX = GET_SPRITE_X(a->sprite);
;         tmpY = GET_SPRITE_Y(a->sprite);
;         done = 1;
;
;         if (tmpX < a->endX) {
;             tmpX += a->speedPerTick;
;             done = 0;
;         } else if (tmpX > a->endX) {
;             tmpX -= a->speedPerTick;
;             done = 0;
;         }
;
;         if (tmpY < a->endY) {
;             tmpY += a->speedPerTick;
;             done = 0;
;         } else if (tmpY > a->endY) {
;             tmpY -= a->speedPerTick;
;             done = 0;
;         }
;
;         if (done) {
;             if (prevA) {
;                 prevA->next = a->next;
;             } else {
;                 animations = a->next;
;             }
;             a->next = NULL;
;             a->sprite = NULL;
;         } else {
;             SET_SPRITE_X(a->sprite, tmpX);
;             SET_SPRITE_Y(a->sprite, tmpY);
;         }
;     }
; }
;
; With the following changes/fixes:
; -Fix conditions for if (tmp[X,Y] < a->end[X,Y]) ... else if (tmp[X,Y] > a->end[X,Y])
; -Initialize prevA (r6)
; -Set prevA (r6) at end of each loop iteration
; -Set next properly when deleting list entry
; -XXX TODO: Clamp to end[X,Y]

_update_animations:
	movei	#_animations,r0		; r0 = &animations
	moveq	#0,r6			; prevA = 0
	load	(r0),r2			; r2 = animations
	cmpq	#0,r2			; if (animations == NULL) goto done;
	movei	#done,r16
	jump	EQ,(r16)
	move	r0,r7			; r7 = &animations

	move	r2,r14			; r14 = a = animations

foreach_animation:
	load	(r14+1),r0		; r0 = a->sprite
	move	r0,r14			; r14 = a->sprite
	load	(r14+5),r1		; r1 = tmpX = a->sprite->x
	load	(r14+6),r0		; r0 = a->sprite->y
	move	r2,r14			; r14 = a
	move	r0,r3			; r3 = tmpY = a->sprite->y
	load	(r14+3),r4		; r4 = a->endX
	moveq	#1,r5			; r5 = done = 1
	cmp	r4,r1			; if (tmpX >= a->endX) goto check_ltx;
	jr	CC,check_ltx
	shrq	#4,r3			; r3 = tmpY = a->sprite->y >> 4;

					; else { /* tmpX < a->endX */
	load	(r14+2),r0		;   r0 = a->speedPerTick
	moveq	#0,r5			;   r5 = done = 0
	jr	T, done_adj_x		;   tmpX += a->speedPerTick
	add	r0,r1			; }

check_ltx:
	cmp	r1,r4			; Originally: if (a->endX > tmpX)  goto done_adj_x_set
	jr	EQ,done_adj_x_set	; Changed to: if (a->endX == tmpX) goto done_adj_x_set
	move	r2,r14			; r14 = a

					; else { /* tmpX > a->endX */
	load	(r14+2),r0		;   r0 = a->speedPerTick
	moveq	#0,r5			;   r5 = done = 0
	sub	r0,r1			;   tmpX -= a->speedPerTick
					; }

done_adj_x_set:
	move	r2,r14			; r14 = a

done_adj_x:
	load	(r14+4),r0		; r0 = a->endY
	cmp	r0,r3			; if (tmpY >= a->endY) goto check_lty;
	jr	CC,check_lty
	cmp	r3,r0

					; else { /* tmpY < a->endY */
	load	(r14+2),r0		;   r0 = a->speedPerTick
	movei	#not_done,r16		;   tmpY += a->speedPerTick
	jump	T,(r16)			;   goto not_done
	add	r0,r3			; }

check_lty:
	jr	EQ,done_adj_y		; Originally: if (a->endY > tmpY)  goto done_adj_y
	nop				; Changed to: if (a->endY == tmpY) goto done_adj_y

					; else { /* tmpY > a->endY */
	move	r2,r14			;   r14 = a
	load	(r14+2),r0		;   r0 = a->speedPerTick
	moveq	#0,r5			;   r5 = done = 0
	sub	r0,r3			;   tmpY -= a->speedPerTick
					; }

done_adj_y:
	cmpq	#0,r5	;tstsi	r5	; if (done == 0) goto not_done;
	movei	#not_done,r16
	jump	EQ,(r16)
	cmpq	#0,r6			; if (prevA == NULL) goto no_prev
	jr	EQ,no_prev
	nop
					; else {
	load	(r2),r8			;   r8 = a->next
	jr	T,clear_next_sprite	;   prevA->next = a->next
	store	r8,(r6)			;   goto clear_next_sprite;
					; }

no_prev:
	load	(r2),r8			; r8 = a->next
	store	r8,(r7)			; animations = a->next
clear_next_sprite:
	move	r2,r14			; r14 = a
	move	r8,r2			; a = a->next
	moveq	#0,r8			; r8 = 0
	store	r8,(r14)		; a->next = NULL
	movei	#try_next,r16		; a->sprite = NULL
	jump	T,(r16)			; goto try_next;
	store	r8,(r14+1)

not_done:
	move	r2,r14			; r14 = a
	load	(r14+1),r0		; r0 = a->sprite
	move	r0,r14			; r14 = a->sprite
	store	r1,(r14+5)		; a->sprite->x = tmpX
	move	r2,r14			; r14 = a
	load	(r14+1),r1		; r1 = a->sprite
	move	r3,r0			; r0 = tmpY
	move	r1,r14			; r14 = a->sprite
	shlq	#4,r0
	store	r0,(r14+6)		; a->sprite->y = tmpY >> 4
	move	r2, r6			; prevA = a
	load	(r2),r2

try_next:
	cmpq	#0,r2			; if (a = a->next) goto foreach_animation;
	movei	#foreach_animation,r16
	jump	NE,(r16)
	move	r2,r14			; r14 = a

done:
	load	(r31),r16		; RTS
	jump	T,(r16)
	addqt	#4,r31

		.long

		.68000
_gputext_end:
_gputext_size	.equ	*-_gputext_start
		.print	"gputext Code size is ",/l/x _gputext_size,""

		.data
		.phrase

clr6x12fnt:	.incbin "clr6x12.jft"
fontdata	.equ	(clr6x12fnt+8)		; Font data is after 1 phrase header
