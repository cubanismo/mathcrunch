		.include "jaguar.inc"
		.include "g_gpugame_codesize.inc"

GPUCODE_OFFSET	.equ	(GPUGAME_CODESIZE + 7) & ~7	; Phrase-align this file's code

BMP_WIDTH	.equ	320 ; XXX Hack
		.globl	_drawString
		.globl	_gputext_start
		.globl	_gputext_end
		.globl	_gputext_size
		.globl	_gputext_dst

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

_drawString:	; Write a NUL-terminated string to the game list
		;  r0 = surfaddr:   The  surface to draw to
		;  r1 = coords:     The packed coordinates (y<<16)|x
		;  r2 = stringaddr: Pointer to the NUL-terminated string
		;  fontdata:   The 1bpp font surface
		moveq	#0, zeror			; 0 will be stored in various fields

		movei	#A1_BASE, r11

		movei	#A1_CLIP, r3
		movei	#$ffffffff, r4
		movei	#B_PATD, r5

		store	r0, (r11)			; Store A1_BASE (destination addr)
		store	r4, (r5)			; Store white in B_PATD low dword
		addq	#4, r5				; Point r5 at 2nd dword of B_PATD
		store	zeror, (r3)			; Store 0 in A1_CLIP (No clipping)
		store	r4, (r5)			; Store white in B_PATD high dword

		.assert BMP_WIDTH = 320
		movei	#PITCH1|PIXEL16|WID320|XADDPIX|YADD0, r4
		movei	#A1_FLAGS, r5

		movei	#A1_FPIXEL, r7

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
		.long

		.68000
_gputext_end:
_gputext_size	.equ	*-_gputext_start
		.print	"gputext Code size is ",/l/x _gputext_size,""

		.data
		.phrase

clr6x12fnt:	.incbin "clr6x12.jft"
fontdata	.equ	(clr6x12fnt+8)		; Font data is after 1 phrase header
