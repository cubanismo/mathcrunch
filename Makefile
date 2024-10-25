include $(JAGSDK)/tools/build/jagdefs.mk

# Change this to 0 to build without the skunk console dependency.
# Things like printf won't work.
SKUNKLIB := 1

CGPUOBJS=gpugame.o
OBJS=startup.o main.o $(CGPUOBJS)

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

$(PROGS): $(OBJS)
	$(LINK) $(LINKFLAGS) -o $@ $(OBJS)

startup.o: lb_box.rgb
main.o: gpu_68k_shr.h
gpugame.o: gpu_68k_shr.h

lb_box.rgb: $(JAGSDK)/jaguar/startup/lb_box.rgb
	cp "$<" "$@"

include $(JAGSDK)/tools/build/jagrules.mk
