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

		.data

		.phrase
_jagcrunchbmp::	; 40 x 32
		.incbin	"g_jagcrunch.cry"

		.phrase
_enemy1bmp::	; 40 x 32
		.incbin	"g_enemy1.cry"

		.phrase
_enemy2bmp::	; 40 x 32
		.incbin	"g_enemy2.cry"

		.phrase
_u235sebmp::	; 320 x 240
		.incbin	"g_u235se.cry"

		.phrase
_titlebmp::	; 320 x 240
		.incbin	"g_title.cry"

		.phrase
_spritebmps::	dc.l _jagcrunchbmp
		dc.l _enemy1bmp
		dc.l _enemy2bmp

		.end
