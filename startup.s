;-----------------------------------------------------------------------------
; Warning!!! Warning!!! Warning!!! Warning!!! Warning!!! Warning!!! Warning!!!
; Warning!!! Warning!!! Warning!!! Warning!!! Warning!!! Warning!!! Warning!!!
;-----------------------------------------------------------------------------
; Do not change any of the code in this file except where explicitly noted.
; Making other changes can cause your program's startup code to be incorrect.
;-----------------------------------------------------------------------------


;----------------------------------------------------------------------------
; Jaguar Development System Source Code
; Copyright (c)1995 Atari Corp.
; ALL RIGHTS RESERVED
;
; Modifications by James Jones are copyright 2025 James Jones, and licensed
; as follows:
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the “Software”), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.
;
; Module: startup.s - Hardware initialization/License screen display
;
; Revision History:
;  1/12/95 - SDS: Modified from MOU.COF sources.
;  2/28/95 - SDS: Optimized some code from MOU.COF.
;  3/14/95 - SDS: Old code preserved old value from INT1 and OR'ed the
;                 video interrupt enable bit. Trouble is that could cause
;                 pending interrupts to persist. Now it just stuffs the value.
;  4/17/95 - MF:  Moved definitions relating to startup picture's size and
;                 filename to top of file, separate from everything else (so
;                 it's easier to swap in different pictures).
;----------------------------------------------------------------------------
; Program Description:
; Jaguar Startup Code
;
; Steps are as follows:
; 1. Set GPU/DSP to Big-Endian mode
; 2. Set VI to $FFFF to disable video-refresh.
; 3. Initialize a stack pointer to high ram.
; 4. Initialize video registers.
; 5. Create an object list as follows:
;            BRANCH Object (Branches to stop object if past display area)
;            BRANCH Object (Branches to stop object if prior to display area)
;            BITMAP Object (Jaguar License Acknowledgement - see below)
;            STOP Object
; 6. Install interrupt handler, configure VI, enable video interrupts,
;    lower 68k IPL to allow interrupts.
; 7. Use GPU routine to stuff OLP with pointer to object list.
; 8. Turn on video.
; 9. Jump to _start.
;
; Notes:
; All video variables are exposed for program use. 'ticks' is exposed to allow
; a flicker-free transition from license screen to next. gSetOLP and olp2set
; are exposed so they don't need to be included by exterior code again.
;-----------------------------------------------------------------------------

	.include    "jaguar.inc"
	.include    "u235se/u235se.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Begin STARTUP PICTURE CONFIGURATION -- Edit this to change startup picture
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

PPP     	.equ    4      			; Pixels per Phrase (1-bit)
BMP_WIDTH   	.equ    320     		; Width in Pixels
BMP_HEIGHT  	.equ    240    			; Height in Pixels
PLAYER_WIDTH	.equ	64
PLAYER_HEIGHT	.equ	64

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; End of STARTUP PICTURE CONFIGURATION
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Globals
		.globl	gSetOLP
		.globl	olp2set
		.globl	_ticks
		.globl	_gpu_running
		.globl	_screenbmp
		.globl	_enemy_bitmap
		.globl	_m2_vals
		.globl	_m3_vals
		.globl	_m4_vals
		.globl	_m5_vals
		.globl	_m6_vals
		.globl	_m7_vals
		.globl	_m8_vals
		.globl	_m9_vals

		.globl  a_vdb
		.globl  a_vde
		.globl  a_hdb
		.globl  a_hde
		.globl  width
		.globl  height
		.globl	_screen_off_x
		.globl	_screen_off_y
		.globl	_ChangeMusic
		.globl	_stop68k
		.globl	_gpu_str
		.globl	_dsp_str
		.globl	_level_str
		.globl	_levelnum_str
		.globl	_levelname_str
		.globl	_score_str
		.globl	_scoreval_str
		.globl	_win_str
		.globl	_lose_str
		.globl	_press_c_str
		.globl	_spriteData
; Externals
		.extern	_start
		.extern _printStats
		.extern _intToStr
		.extern _cpuCmd;
		.extern _cpuData0;
		.extern _cpuData1;
		.extern samplebank;

MAX_SPRITES	.equ	4			; 3 characters + the screen
BMP_PHRASES 	.equ    (BMP_WIDTH/PPP) 	; Width in Phrases
BMP_LINES   	.equ    (BMP_HEIGHT*2)  	; Height in Half Scanlines
BITMAP_OFF  	.equ    (2*8)       		; Two Phrases
LISTSIZE    	.equ    3 + (MAX_SPRITES * 2)	; List length (in phrases):
						; 2 branch objs + 1 stop obj
						; + MAX_SPRITES bitmap objs

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Program Entry Point Follows...

		.text
		move.l  #$70007,G_END		; big-endian mode
		move.l  #$70007,D_END
		move.w  #$FFFF,VI       	; disable video interrupts

		move.l  #INITSTACK,a7   	; Setup a stack
			
		jsr 	InitVideo      		; Setup our video registers.
		jsr 	InitLister     		; Initialize Object Display List
		jsr 	InitInt      		; Initialize our VBLANK routine
		jsr	InitU235se		; Initialize the sound engine

;;; Sneaky trick to cause display to popup at first VB

		move.l	#$0,listbuf+BITMAP_OFF
		move.l	#$C,listbuf+BITMAP_OFF+4

		move.l  d0,olp2set      	; D0 is swapped OLP from InitLister
		move.l  #gSetOLP,G_PC   	; Set GPU PC
		move.l  #RISCGO,G_CTRL  	; Go!
waitforset:
		move.l  G_CTRL,d0   		; Wait for write.
		andi.l  #$1,d0
		bne 	waitforset

		move.w  #$6C1,VMODE     	; Configure Video

	     	jmp 	_start			; Jump to main code

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Procedure: gSetOLP
;            Use the GPU to set the OLP and quit.
;
;    Inputs: olp2set - Variable contains pre-swapped value to stuff OLP with.
;
; NOTE!!!: This code can run in DRAM only because it contains no JUMP's or
;          JR's. It will generate a warning with current versions of MADMAC
;          because it doesn't '.ORG'.
;
		.long
		.gpu
gSetOLP:
		movei   #olp2set,r0   		; Read value to write
		load    (r0),r1

		movei   #OLP,r0       		; Store it
		store   r1,(r0)

		moveq   #0,r0         		; Stop GPU
		movei   #G_CTRL,r1
		store   r0,(r1)
		nop             		; Two "feet" on the brake pedal
		nop

		.68000
		.bss
		.long

olp2set:    	.ds.l   1           		; GPU Code Parameter

		.text

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Procedure: InitInt
; Install our vertical blank and GPU interrupt handlers and enable interrupts
;

InitInt:
		move.l  d0,-(sp)

		move.l  #HandleInt,LEVEL0	; Install 68K LEVEL0 handler

		move.w  a_vde,d0        	; Must be ODD
		ori.w   #1,d0
		move.w  d0,VI

		move.w  #C_VIDENA|C_GPUENA,INT1	; Enable video and GPU interrupts

		move.w  sr,d0
		and.w   #$F8FF,d0       	; Lower 68k IPL to allow
		move.w  d0,sr           	; interrupts

		move.l  (sp)+,d0
		rts
		
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Procedure: InitVideo (same as in vidinit.s)
;            Build values for hdb, hde, vdb, and vde and store them.
;
						
InitVideo:
		movem.l d0-d6,-(sp)
			
		move.w  CONFIG,d0      		 ; Also is joystick register
		andi.w  #VIDTYPE,d0    		 ; 0 = PAL, 1 = NTSC
		beq 	palvals

		move.w  #NTSC_HMID,d2
		move.w  #NTSC_WIDTH,d0

		move.w  #NTSC_VMID,d6
		move.w  #NTSC_HEIGHT,d4

		bra 	calc_vals
palvals:
		move.w  #PAL_HMID,d2
		move.w  #PAL_WIDTH,d0

		move.w  #PAL_VMID,d6
		move.w  #PAL_HEIGHT,d4

calc_vals:
		move.w  d0,width
		move.w  d4,height

		move.w  d0,d1
		asr 	#1,d1         	 	; Width/2

		sub.w   d1,d2         	  	; Mid - Width/2
		add.w   #4,d2         	  	; (Mid - Width/2)+4

		sub.w   #1,d1         	  	; Width/2 - 1
		ori.w   #$400,d1      	  	; (Width/2 - 1)|$400
		
		move.w  d1,a_hde
		move.w  d1,HDE

		move.w  d2,a_hdb
		move.w  d2,HDB1
		move.w  d2,HDB2

		move.w  d6,d5
		sub.w   d4,d5
		move.w  d5,a_vdb

		add.w   d4,d6
		move.w  d6,a_vde

		move.w  a_vdb,VDB
		move.w  #$FFFF,VDE
			
		move.l  #0,BORD1        	; Black border
		move.w  #0,BG           	; Init line buffer to black
			
		movem.l (sp)+,d0-d6
		rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; InitU235: Initialize the U235 sound engine
;
; Registers:	d0 and a0 are clobbered
;
InitU235se:
		movem.l	d0/a0-a1,-(sp)
		move.w	CONFIG, d0
		andi.w	#VIDTYPE, d0
		beq	.palsound

		jsr	U235SE_initNTSC
		bra	.soundidone

.palsound:
		jsr	U235SE_initPAL

.soundidone:

		move.l	#U235SE_24KHZ, U235SE_playback_rate
		move.l	#U235SE_24KHZ_PERIOD, U235SE_playback_period
		; Mix music & SFX in mono
		move.l	#0,U235SE_playbackmode

		move.l	#D_RAM, D_PC
		move.l	#RISCGO, D_CTRL
		move.w	#$100, JOYSTICK

		movem.l	(sp)+,d0/a0-a1
		rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ChangeMusic(void *modFilePtr)
;
; C-callable function to change the mod file u235se is playing
;
_ChangeMusic:
		movem.l	d1-d7/a0-a6,-(sp)

		move.l	#U235SE_NOMOD, U235SE_playmod
		move.l	#stopmuscmds, U235SE_sfxplaylist_ptr
.waitsilence:
		tst.l	U235SE_sfxplaylist_ptr
		bne	.waitsilence

		move.l	60(sp), a0
		cmp.l	#0, a0
		beq	.donechg

		jsr	U235SE_modinit
		move.l	#32, U235SE_music_vol	; Set music volume to 32/63
		move.l	#63, U235SE_sfx_vol	; Set sound fx volume to 63/63
		move.l	#U235SE_PLAYMONO, U235SE_playmod

.donechg:
		move.l	a0, d0
		movem.l	(sp)+,d1-d7/a0-a6
		rts

SetSpriteList:
		move.l	d1, spriteList
		rts

WaitStopGPU:
		move.l  G_CTRL,d0   		; Wait for write.
		andi.w  #$1,d0
		bne 	WaitStopGPU
		move.l	#0,_gpu_running
		rts

PlaySound:
		lea	playsoundcmds,a0
		lea	U235SE_sfxplaylist_ptr, a1

		move.l	#samplebank,d0
		lsl.l	#4, d0
		or.l	#$b, d0
		move.l	d0, (a0)+

		move.l	#$42,(a0)+

		move.l	#0,(a0)

		move.l	#playsoundcmds,d0
		move.l	d0,(a1)

		move.l	#playsoundcmds, (a1)
		rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; InitLister: Initialize Object List Processor List
;
;    Returns: Pre-word-swapped address of current object list in d0.l
;
;  Registers: d0.l/d1.l - Phrase being built
;             d2.l/d3.l - Link address overlays
;             d4.l      - Work register
;             a0.l      - Roving object list pointer
		
InitLister:
		movem.l d1-d4/a0,-(sp)		; Save registers

		move.l	#0, spriteList		; Turn off the sprite list
			
		lea     listbuf,a0
		move.l  a0,d2           	; Copy

		add.l   #(LISTSIZE-1)*8,d2  	; Address of STOP object
		move.l	d2,d3			; Copy for low half

		lsr.l	#8,d2			; Shift high half into place
		lsr.l	#3,d2
		
		swap	d3			; Place low half correctly
		clr.w	d3
		lsl.l	#5,d3

; Write first BRANCH object (branch if YPOS > a_vde )

		clr.l   d0
		move.l  #(BRANCHOBJ|O_BRLT),d1  ; $4000 = VC < YPOS
		or.l	d2,d0			; Do LINK overlay
		or.l	d3,d1
								
		move.w  a_vde,d4                ; for YPOS
		lsl.w   #3,d4                   ; Make it bits 13-3
		or.w    d4,d1

		move.l	d0,(a0)+
		move.l	d1,(a0)+

; Write second branch object (branch if YPOS < a_vdb)
; Note: LINK address is the same so preserve it

		andi.l  #$FF000007,d1           ; Mask off CC and YPOS
		ori.l   #O_BRGT,d1      	; $8000 = VC > YPOS
		move.w  a_vdb,d4                ; for YPOS
		lsl.w   #3,d4                   ; Make it bits 13-3
		or.w    d4,d1

		move.l	d0,(a0)+
		move.l	d1,(a0)+

; Write a standard BITMAP object
		move.l	d2,d0
		move.l	d3,d1

		ori.l  #BMP_HEIGHT<<14,d1       ; Height of image

		move.w  height,d4           	; Center bitmap vertically
		sub.w   #BMP_HEIGHT,d4
		add.w   a_vdb,d4
		lsr.w	#1,d4			; Must be even
		move.w	d4,_screen_off_y	; Save for use in sprite engine
		lsl.w   #4,d4
		or.w    d4,d1                   ; Stuff YPOS in low phrase

		move.l	#_screenbmp,d4
		lsl.l	#8,d4
		or.l	d4,d0

		move.l	d0,(a0)+
		move.l	d1,(a0)+
		movem.l	d0-d1,bmpupdate

; Second Phrase of Bitmap
		move.l	#BMP_PHRASES>>4,d0	; Only part of top LONG is IWIDTH
		move.l  #O_DEPTH16|O_NOGAP,d1   ; Bit Depth = 16-bit, Contiguous data

		move.w  width,d4            	; Get width in clocks
		lsr.w   #2,d4               	; /4 Pixel Divisor
		sub.w   #BMP_WIDTH,d4
		lsr.w   #1,d4
		move.w	d4,_screen_off_x	; Save for use in sprite engine
		or.w    d4,d1

		ori.l	#(BMP_PHRASES<<18)|(BMP_PHRASES<<28),d1	; DWIDTH|IWIDTH

		move.l	d0,(a0)+
		move.l	d1,(a0)+

; Write a STOP object at end of list
		lea     listbuf+((LISTSIZE-1)*8),a0
		clr.l   (a0)+
		move.l  #(STOPOBJ|O_STOPINTS),(a0)+

; Now return swapped list pointer in D0

		move.l  #listbuf,d0
		swap    d0

		movem.l (sp)+,d1-d4/a0
		rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Procedure: HandleInt
;        Handle LEVEL0 Interrupt by dispatching to sub-handlers.
HandleInt:
		move.l	d0, -(sp)
		move.w	INT1, d0

		btst.l	#0, d0
		beq	.novid
		jsr	UpdateList

.novid:
		; Signal we're done processing all received interrupts
		lsl.w	#8, d0		; Only clear interrupts received
		or.w	#(C_VIDENA|C_GPUENA), d0
		move.w	d0, INT1
		move.w  #$0, INT2 ; Restore normal bus priorities

		move.l	(sp)+, d0
		rte

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Macro: HANDLE_CPUCMD
;
;       Macro to help dispatch the 68k commands from the GPU
;
; Params:
;   cmdNum:   The number of the command. Should be one of the valid
;             CPUCMD_* enum values
;   func:     The function to call for this command
;   regParam: If the function takes a stack parameter, set this to d1
;
.macro HANDLE_CPUCMD cmdNum, func, regPar0, regPar1
		cmp.l	#\cmdNum, d0
		bne.s	.no\~
.if \# > 3
		move.l	\regPar1, -(sp)
.endif
.if \# > 2
		move.l	\regPar0, -(sp)
.endif
		jsr	\func
.if \# > 2
		addq.l	#4, sp
.endif
.if \# > 3
		addq.l	#4, sp
.endif
		moveq.l	#0, d0
		move.l	d0, _cpuCmd
.no\~:
.endm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Procedure: HandleGpu
;
;      Dispatch a command from the the GPU on the 68k
;
HandleGpu:
		movem.l	d0-d2/a0-a1, -(sp)

		move.l	_cpuCmd, d0
		move.l	_cpuData0, d1
		move.l	_cpuData1, d2

		HANDLE_CPUCMD 1, _printStats
		HANDLE_CPUCMD 2, _ChangeMusic, d1
		HANDLE_CPUCMD 3, SetSpriteList
		HANDLE_CPUCMD 4, _intToStr, d1, d2
		HANDLE_CPUCMD 5, WaitStopGPU
		HANDLE_CPUCMD 6, PlaySound

		movem.l	(sp)+, d0-d2/a0-a1
		rts


WalkSpriteList:
		movem.l	d0-d3, -(sp)

.spriteLoop:
		move.l	#listbuf+((LISTSIZE-1)*8), d2	; Assume this is last bitmap

		move.l	(a0), d1
		tst.l	d1			; If next is NULL the assumption
		beq.s	.doLink			; was correct.

		move.l	a1, d2			; Otherwise, link to next bitmap object
		add.l	#$10, d2

.doLink:
		move.l	d2, d3			; Copy for lower half
		lsr.l	#8, d2			; Shift high half into place
		lsr.l	#3, d2

		swap	d3			; Place low half correctly
		clr.w	d3
		lsl.l	#5, d3

		; Build the first phrase
		move.l	8(a0), d0		; d0 = phrase 1 high dword
		or.l	d2, d0			; Add in link
		move.l	d0, (a1)+		; Store phrase 1 high dword

		move.l	4(a0), d0		; d0 = phrase 1 low dword
		or.l	24(a0), d0		; Add y position
		or.l	d3, d0			; Add in link
		move.l	d0, (a1)+		; Store phrase 1 low dword

		move.l	16(a0), (a1)+		; Store phrase 2 high dword

		move.l	12(a0), d0		; d0 = phrase 2 low dword
		or.l	20(a0), d0		; Add in x position
		move.l	d0, (a1)+		; Store phrase 2 low dword

		; XXX Handle double-buffering of start address.

		movea.l	d1, a0
		tst.l	d1			; If there is a next sprite,
		bne	.spriteLoop		; Loop.

		movem.l	(sp)+,d0-d3
		rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Procedure: UpdateList
;        Handle Video Interrupt and update object list fields
;        destroyed by the object processor.

UpdateList:
		move.l  a0,-(sp)

		tst.l	spriteList
		beq.s	.emptyList

		move.l	a1,-(sp)
		move.l	spriteList, a0
		move.l	#listbuf+BITMAP_OFF, a1
		jsr	WalkSpriteList
		move.l	(sp)+,a1

		bra.s	.doneUpdating
.emptyList:
		move.l  #listbuf+BITMAP_OFF,a0

		move.l  bmpupdate,(a0)      	; Phrase = d1.l/d0.l
		move.l  bmpupdate+4,4(a0)

.doneUpdating:
		add.l	#1,_ticks		; Increment ticks semaphore

		move.l  (sp)+,a0
		rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Procedure: _stop68k
;        Halt the 68k
_stop68k:
		stop	#$2000
		jsr	HandleGpu
		rts

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

		.data
		.long

stopmuscmds:	.dc.l	$01
		.dc.l	$11
		.dc.l	$21
		.dc.l	$31
		.dc.l	$0

_score_str:	.dc.b	'Score:',0
_level_str:	.dc.b	'Level:',0
_win_str:	.dc.b	'        Congratulations! You Won!',0
_lose_str:	.dc.b	'That number is not a multiple! Game Over.',0
_press_c_str:	.dc.b	'          Press "C" to continue',0

		.long
_m2_vals:	.dc.l	 2, 4, 6, 8,10,12,14,16,18,20, 1, 3, 5, 7, 9,11
		.dc.l	13,15,17,19, 2, 4, 6, 8,10,12,14,16,18,20, 1, 3
		.dc.l	 5, 7, 9,11,13,15,17,19, 2, 4, 6, 8,10,12,14,16
		.dc.l	18,20, 1, 3, 5, 7, 9,11,13,15,17,19, 2, 4, 1, 3
_m3_vals:	.dc.l	 3, 6, 9,12,15,18,21,24,27,30, 3, 6, 9,12,15,18
		.dc.l	21,24,27,30, 1, 2, 4, 5, 7, 8,10,11,13,14,16,17
		.dc.l	19,20,22,23,25,26,28,29,31,32, 1, 2, 4, 5, 7, 8
		.dc.l	10,11,13,14,16,17,19,20,22,23,25,26,28,29,31,32
_m4_vals:	.dc.l	 4, 8,12,16,20,24,28,32,36,40,44,48, 4, 8,12,16
		.dc.l	20,24,28,32, 1, 2, 3, 5, 6, 7, 9,10,11,13,14,15
		.dc.l	17,18,19,21,22,23,25,26,27,29,30,31,33,34,35,37
		.dc.l	38,39,41,42,43,45,46,47, 1, 2, 3, 5, 6, 7, 9,10
_m5_vals:	.dc.l	 5,10,15,20,25,30,35,40,45,50,55,60, 5,10,15,20
		.dc.l	25,30,35,40,45,50,55,60, 5, 7, 8, 9,11,12,13,14
		.dc.l	16,17,18,19,21,22,23,24,26,27,28,29,31,32,33,34
		.dc.l	36,37,38,39,41,42,43,44,46,47,48,49,51,52,53,54
_m6_vals:	.dc.l	 6,12,18,24,30,36,42,48,54,60,66,72, 6,12,18,24
		.dc.l	30,36,42,48,54,60,66,72, 6,12,18,24,30,36,42,48
		.dc.l	 2, 4, 5, 7, 8,10,13,14,16,20,21,22,23,25,26,28
		.dc.l	32,34,38,40,44,46,50,52,56,58,62,64,68,70,71,73
_m7_vals:	.dc.l	 7,14,21,28,35,42,49,56,63,70,77,84,91,98, 7,21
		.dc.l	35,42,56,84,91,98, 7,14,21,28,35,42,49,56,63,70
		.dc.l	11,17,23,32,39,44,43,50,51,58,59,64,68,71,73,74
		.dc.l	78,81,87,88,89,92,93,94,95,96,97,99,33,32,20,27
_m8_vals:	.dc.l	 8,16,24,32,40,48,56,64,72,80,88,96, 8,16,24,32
		.dc.l	40,48,56,64,72,80,88,96, 8,16,24,32,40,48,56,64
		.dc.l	 4,10,12,14,18,20,26,28,33,34,37,38,42,44,47,48
		.dc.l	52,54,58,60,62,68,70,74,78,82,84,86,90,92,94,98
_m9_vals:	.dc.l	 9,18,27,36,45,54,63,72,81,90,99, 9,18,27,36,45
		.dc.l	54,63,72,81,90,99, 9,18,27,36,45,54,63,72,81,90
		.dc.l	 3, 6,16,19,21,23,29,34,38,39,40,41,42,44,47,49
		.dc.l	52,56,59,64,66,69,70,74,78,79,82,89,91,92,97,98

		.bss
		.dphrase

listbuf:    	.ds.l   LISTSIZE*2  		; Object List
bmpupdate:  	.ds.l   2       		; One Phrase of Bitmap for Refresh
_ticks:		.ds.l	1			; Incrementing # of ticks
_gpu_running:	.ds.l	1			; 1 if GPU is running, 0 if it is stopped
a_hdb:  	.ds.w   1
a_hde:      	.ds.w   1
a_vdb:      	.ds.w   1
a_vde:      	.ds.w   1
width:      	.ds.w   1
height:     	.ds.w   1
_screen_off_x:	.ds.w	1
_screen_off_y:	.ds.w	1

		.long
spriteList:	.ds.l	1
_spriteData:	.ds.l	40*MAX_SPRITES		; MAX_SPRITES * sizeof(Sprite)
_gpu_str:	.ds.b	128
_dsp_str:	.ds.b	128
_levelnum_str:	.ds.b	8
_levelname_str:	.ds.b	64
_scoreval_str:	.ds.b	16
playsoundcmds:	.ds.l	3

		.phrase
_screenbmp:	.ds.l	BMP_WIDTH*BMP_HEIGHT*(PPP>>1)*2
		.phrase
_enemy_bitmap:	.ds.l	PLAYER_WIDTH*PLAYER_HEIGHT*(PPP>>1)*2

		.end
