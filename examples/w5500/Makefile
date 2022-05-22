# Clone https://github.com/cesanta/mip alongside MDK
MIP ?= ../../../mip
MG ?= ../../../mongoose
SOURCES = main.c mongoose_custom.c $(MIP)/mip.c $(MG)/mongoose.c
DEFS = -DMG_ARCH=0 -DMG_ENABLE_LINES=1 -DMIP_DEBUG=1 -DMG_ENABLE_LOG=0 -DMG_ENABLE_CUSTOM_MILLIS=1
EXTRA_CFLAGS += -I$(MIP) -I$(MG) -I../../tools $(DEFS)
include ../../make/build.mk
