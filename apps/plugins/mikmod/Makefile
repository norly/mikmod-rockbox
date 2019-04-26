#             __________               __   ___.
#   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
#   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
#   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
#   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
#                     \/            \/     \/    \/            \/
# $Id: Makefile 13515 2007-05-29 16:33:16Z nls $
#

INCLUDES = -I$(APPSDIR) \
			-I.. \
			-I. $(TARGET_INC) \
			-I$(FIRMDIR)/include \
			-I$(FIRMDIR)/export \
			-I$(FIRMDIR)/common \
			-I$(FIRMDIR)/drivers \
			-I$(OUTDIR) \
			-I$(BUILDDIR) \
			-I./include
CFLAGS = $(INCLUDES) $(GCCOPTS) -O2 $(TARGET) $(EXTRA_DEFINES) \
 -DTARGET_ID=$(TARGET_ID) -DMEM=${MEMORYSIZE} -DPLUGIN -DHAVE_SNPRINTF

ifdef APPEXTRA
   INCLUDES += $(patsubst %,-I$(APPSDIR)/%,$(subst :, ,$(APPEXTRA)))
endif

LINKFILE := $(OBJDIR)/link.lds
DEPFILE = $(OBJDIR)/dep-mikmod
SRC = mikmod.c \
	loaders/load_669.c \
	loaders/load_amf.c \
	loaders/load_asy.c \
	loaders/load_dsm.c \
	loaders/load_far.c \
	loaders/load_gdm.c \
	loaders/load_imf.c \
	loaders/load_it.c \
	loaders/load_m15.c \
	loaders/load_med.c \
	loaders/load_mod.c \
	loaders/load_mtm.c \
	loaders/load_s3m.c \
	loaders/load_stm.c \
	loaders/load_stx.c \
	loaders/load_ult.c \
	loaders/load_uni.c \
	loaders/load_xm.c \
	mmio/mmalloc.c \
	mmio/mmerror.c \
	mmio/mmio.c \
	playercode/mdriver.c \
	playercode/mloader.c \
	playercode/mlreg.c \
	playercode/mlutil.c \
	playercode/mplayer.c \
	playercode/munitrk.c \
	playercode/mwav.c \
	playercode/npertab.c \
	playercode/sloader.c \
	playercode/virtch2.c \
	playercode/virtch.c \
	playercode/virtch_common.c

SOURCES = $(SRC)
OBJS := $(SRC:%.c=$(OBJDIR)/%.o)
DIRS = .

ifndef SIMVER
ifneq (,$(strip $(foreach tgt,RECORDER ONDIO,$(findstring $(tgt),$(TARGET)))))
    LDS := archos.lds
    OUTPUT = $(OUTDIR)/mikmod.ovl
else  ## iRiver target
    LDS := ../plugin.lds
    OUTPUT = $(OUTDIR)/mikmod.rock
endif
else  ## simulators
    OUTPUT = $(OUTDIR)/mikmod.rock
endif

all: $(OUTPUT)

ifndef SIMVER
$(OBJDIR)/mikmod.elf: $(OBJS) $(LINKFILE) $(BITMAPLIBS)
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) -o $@ $(OBJS) -L$(BUILDDIR) -lplugin -lgcc \
           $(LINKBITMAPS) -T$(LINKFILE) -Wl,-Map,$(OBJDIR)/mikmod.map

$(OUTPUT): $(OBJDIR)/mikmod.elf
	$(call PRINTS,OBJCOPY $(@F))$(OC) -O binary $< $@
else

ifeq ($(SIMVER), x11)
###################################################
# This is the X11 simulator version

$(OUTPUT): $(OBJS)
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) $(SHARED_FLAG) $(OBJS) -L$(BUILDDIR) -lplugin $(LINKBITMAPS) -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif

else # end of x11-simulator
ifeq ($(SIMVER), sdl)
###################################################
# This is the SDL simulator version

$(OUTPUT): $(OBJS)
	$(call PRINTS,LD $(@F))$(CC) $(CFLAGS) $(SHARED_FLAG) $(OBJS) -L$(BUILDDIR) -lplugin $(LINKBITMAPS) -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif

else # end of sdl-simulator
###################################################
# This is the win32 simulator version
DLLTOOLFLAGS = --export-all
DLLWRAPFLAGS = -s --entry _DllMain@12 --target=i386-mingw32 -mno-cygwin

$(OUTPUT): $(OBJS)
	$(call PRINTS,DLL $(@F))$(DLLTOOL) $(DLLTOOLFLAGS) -z $(OBJDIR)/$*.def $(OBJS)
	$(SILENT)$(DLLWRAP) $(DLLWRAPFLAGS) --def $(OBJDIR)/$*.def $(OBJS) \
                $(BUILDDIR)/libplugin.a $(BITMAPLIBS) -o $@
ifeq ($(findstring CYGWIN,$(UNAME)),CYGWIN)
# 'x' must be kept or you'll have "Win32 error 5"
#     $ fgrep 5 /usr/include/w32api/winerror.h | head -1
#         #define ERROR_ACCESS_DENIED 5L
else
	@chmod -x $@
endif
endif # end of win32-simulator
endif
endif # end of simulator section


include $(TOOLSDIR)/make.inc

# MEMORYSIZE should be passed on to this makefile with the chosen memory size
# given in number of MB
$(LINKFILE): $(LDS)
	$(call PRINTS,build $(@F))cat $< | $(CC) -DMEMORYSIZE=$(MEMORYSIZE) $(INCLUDES) $(TARGET) \
	$(DEFINES) -E -P - >$@

clean:
	$(call PRINTS,cleaning mikmod)rm -rf $(OBJDIR)/mikmod
	$(SILENT)rm -f $(OBJDIR)/mikmod.* $(DEPFILE)

-include $(DEPFILE)
