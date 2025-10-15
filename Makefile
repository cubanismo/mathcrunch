# Copyright 2025 James Jones
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the “Software”), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

include $(JAGSDK)/tools/build/jagdefs.mk

# Change one of these to 1 to build with the skunk console dependency
# or the GameDrive library. They're only used for printf-style debugging.
SKUNKLIB := 0
GDLIB := 0

OBJS=startup.o main.o u235sec.o sprintf.o util.o $(CGPUOBJS) music.o sprites.o gpuasm.o

CGPUOVERLAYOBJS := gpulevelinit.o gpuplaylevel.o
CGPUOBJS=gpucommon.o $(CGPUOVERLAYOBJS)

ifeq ($(SKUNKLIB),1)
	OBJS += skunkc.o skunk.o
endif

ifeq ($(GDLIB),1)
	OBJS += gdbios_bindings.o
	CFLAGS += -DUSE_GD=1
endif

# Additional M68K CFLAGS
CFLAGS += -fno-builtin

# Additional AGPU CFLAGS
CFLAGS_JRISC += -fno-builtin -mstk=4000

PROGS = mathcrunch.cof

ifeq ($(SKUNKLIB),1)
	include $(JAGSDK)/jaguar/skunk/skunk.mk

skunkc.o: $(SKUNKDIR)/lib/skunkc.s
	$(ASM) $(ASMFLAGS) -o $@ $<
endif

# Include some objects we don't want to clean here too
ALLOBJS = $(OBJS) u235se/dsp.obj

$(PROGS): $(ALLOBJS)
	$(LINK) $(LINKFLAGS) -o $@ $^

SPRITE_GFX = g_jagcrunch.cry g_u235se.cry g_title.cry g_enemy1.cry g_enemy2.cry
GENERATED += $(SPRITE_GFX)

music.o: *.mod *.pcm
main.o: gpu_68k_shr.h u235se.h startup.h sprintf.h music.h
gpulevelinit.o: gpu_68k_shr.h startup.h music.h u235se.h sprites.h gpucommon.h gpucommon.o
gpuplaylevel.o: gpu_68k_shr.h startup.h music.h u235se.h sprites.h gpucommon.h gpucommon.o
gpuasm.o: g_gpugame_codesize.inc
sprites.o: $(SPRITE_GFX)

$(CGPUOVERLAYOBJS): CFLAGS_JRISC += -mstk=0 -morg=`sh codedata.sh gpucommon.o _gpucommon_size endaddr`

g_gpugame_codesize.inc: gpuplaylevel.o gpulevelinit.o
	@echo "GPUPLAYLEVEL_CODESIZE	.equ \$$`symval gpuplaylevel.o _gpuplaylevel_size`" | tee $@
	@echo "GPULEVELINIT_CODESIZE	.equ \$$`symval gpulevelinit.o _gpulevelinit_size`" | tee -a $@
	@echo "GPUCOMMON_CODESIZE	.equ \$$`sh codedata.sh gpucommon.o _gpucommon_size size`" | tee -a $@
	@echo ".if GPUPLAYLEVEL_CODESIZE > GPULEVELINIT_CODESIZE" | tee -a $@
	@echo "  GPUGAME_CODESIZE	.equ (GPUPLAYLEVEL_CODESIZE + GPUCOMMON_CODESIZE)" | tee -a $@
	@echo ".else" | tee -a $@
	@echo "  GPUGAME_CODESIZE	.equ (GPULEVELINIT_CODESIZE + GPUCOMMON_CODESIZE)" | tee -a $@
	@echo ".endif" | tee -a $@
GENERATED += g_gpugame_codesize.inc

g_%.cry:%.tga
	 tga2cry -binary -f cry -o $@ $<

include $(JAGSDK)/tools/build/jagrules.mk
