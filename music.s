		.globl _mus_title
		.globl _mus_main
		.globl samplebank

.data
		.long
		; https://modarchive.org/index.php?request=view_by_moduleid&query=170000
_mus_title:
		.incbin "evil_minded.mod"

		.long
		; https://modarchive.org/index.php?request=view_by_moduleid&query=189202
_mus_main:
		.incbin "dog16.mod"

		.long
jagroar_start:	; 22050Hz two's complement 8-bit mono PCM sample
		.incbin "jaguar.pcm"
jagroar_end:

		.dphrase
samplebank:
	; Sample 0
		dc.l	jagroar_start
		dc.l	jagroar_end
		dc.l	0		; RBASE (For repeating sounds)
		dc.l	0		; REND  (For repeating sounds)
		dc.l	64		; Volume & fine tune. 64 = max volume.
		dc.l	11025		; Frequency / 2
