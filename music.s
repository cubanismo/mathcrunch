; Copyright 2025 James Jones
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the “Software”), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in all
; copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
; SOFTWARE.

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
