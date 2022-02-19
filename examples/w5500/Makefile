# Clone https://github.com/cesanta/mip alongside MDK
MIP ?= ../../../mip
MG ?= ../../../mongoose
SOURCES = main.c $(MIP)/mip.c $(MG)/mongoose.c

EXTRA_CFLAGS += -I$(MIP) -I$(MG) -I../../tools -DMG_ARCH=0 -DMG_ENABLE_LINES=1 -DMIP_DEBUG=1

include ../../make/build.mk
