include $(JAGSDK)/tools/build/jagdefs.mk

# Change this to 0 to build without the skunk console dependency.
# Things like printf won't work.
SKUNKLIB := 1
GDLIB := 0

OBJS=startup.o main.o u235sec.o sprintf.o util.o $(CGPUOBJS) music.o sprites.o gpuasm.o

CFLAGS_JRISC += -mstk=4000
CGPUOBJS=gpugame.o

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
CFLAGS_JRISC += -fno-builtin

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

SPRITE_GFX = g_jagcrunch.cry
GENERATED += $(SPRITE_GFX)

music.o: *.mod
main.o: gpu_68k_shr.h u235se.h startup.h sprintf.h music.h
gpugame.o: gpu_68k_shr.h startup.h music.h u235se.h sprites.h
gpuasm.o: g_gpugame_codesize.inc
sprites.o: $(SPRITE_GFX)

g_gpugame_codesize.inc: gpugame.o
	@echo "GPUGAME_CODESIZE .equ $$`symval $< _gpugame_size`" | tee $@
GENERATED += g_gpugame_codesize.inc

g_%.cry:%.tga
	 tga2cry -binary -f cry -o $@ $<

gpugame.s: gpugame.c
	gcc263 -DJAGUAR -DUSE_SKUNK -I/home/jjones/Documents/Projects/jaguar-sdk/jaguar/include -I/home/jjones/Documents/Projects/jaguar-sdk/jaguar/skunk/include -b agpu -O2 -fomit-frame-pointer -fno-builtin -S gpugame.c

include $(JAGSDK)/tools/build/jagrules.mk
