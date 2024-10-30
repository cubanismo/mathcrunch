include $(JAGSDK)/tools/build/jagdefs.mk

# Change this to 0 to build without the skunk console dependency.
# Things like printf won't work.
SKUNKLIB := 1

OBJS=startup.o main.o u235sec.o $(CGPUOBJS) music.o

CGPUOBJS=gpugame.o

ifeq ($(SKUNKLIB),1)
	OBJS += skunkc.o skunk.o
	OBJS += sprintf.o util.o
endif

# Additional M68K CFLAGS
CFLAGS += -fno-builtin

# Additional AGPU CFLAGS
CFLAGS_JRISC += -v -fno-builtin

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

music.o: *.mod
main.o: gpu_68k_shr.h u235se.h startup.h sprintf.h music.h
gpugame.o: gpu_68k_shr.h

include $(JAGSDK)/tools/build/jagrules.mk
