LIBSPMP8K = ../..
PROFILE=0

TARGET	= tgemu

OBJS	= src/cpu/h6280.o \
  game.o ui.o pc_engine.o tgemu_logo.o \
  src/fileio.o \
  src/pce.o \
  src/psg.o \
  src/render.o \
  src/system.o \
  src/unzip.o \
  src/vce.o \
  src/vdc.o \

ifeq ($(PROFILE),1)
OBJS += profile.o
endif

LIBS	= -lgame -lz -lc -lgcc

include $(LIBSPMP8K)/main.cfg
include $(LIBGAME)/libgame.mk

CFLAGS += -Isrc -Isrc/cpu -DLSB_FIRST -DFAST_MEM -O3 -funroll-loops -fomit-frame-pointer

ifeq ($(PROFILE),1)
CFLAGS += -finstrument-functions -DPROFILE
profile.o: profile.c
	$(CC) -O2 -c $< -o $@
endif

text.o: hzktable.c
hzktable.c: chinese/BG2UBG.KU chinese/big5.py
	python chinese/big5.py chinese/BG2UBG.KU >hzktable.c

pc_engine.c: pc_engine.zrgb
	$(BIN2C) $< $@
tgemu_logo.c: tgemu_logo.zrgb
	$(BIN2C) $< $@
pc_engine.rgb: pc_engine.png
	ffmpeg -y -vcodec png -i $< -vf scale=200:150 -vcodec rawvideo -f rawvideo -pix_fmt rgb565 $@
tgemu_logo.rgb: tgemu_logo.png
	ffmpeg -y -vcodec png -i $< -vf scale=160:90 -vcodec rawvideo -f rawvideo -pix_fmt rgb565 $@
%.zrgb: %.rgb
	python -c "import zlib,sys; sys.stdout.write(zlib.compress(sys.stdin.read()))" < $< > $@
