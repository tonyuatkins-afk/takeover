# TAKEOVER - AI Takeover Simulator for DOS
# Build with OpenWatcom 2.0: wmake
#
# Prerequisites:
#   - OpenWatcom 2.0 installed (WATCOM env var set)
#   - NASM (optional, for future assembly modules)

CC = wcc
LD = wlink
ASM = nasm

# OpenWatcom flags:
#   -0    8088 instructions only (maximum compatibility)
#   -fpi  inline 8087 emulation (no coprocessor required)
#   -ms   small memory model
#   -s    remove stack overflow checks
#   -ox   maximum optimization
#   -w4   maximum warnings
#   -zq   quiet mode
#   -bt=dos  target DOS
CFLAGS = -0 -fpi -ms -s -ox -w4 -zq -bt=dos

# Include paths
CFLAGS += -i=src -i=lib

# Object files (explicit list for wmake compatibility)
OBJS = src\main.obj src\engine.obj src\effects.obj src\audio.obj &
       src\menu.obj src\news.obj src\display.obj src\hwdetect.obj &
       src\vga13h.obj src\title.obj src\adlib.obj src\climax.obj &
       src\cracktro.obj lib\screen.obj

# Target
TARGET = TAKEOVER.EXE

all: $(TARGET) .SYMBOLIC

# Explicit compilation rules (wmake inference rules are fragile across dirs)
src\main.obj: src\main.c src\engine.h src\effects.h src\menu.h src\hwdetect.h src\title.h src\cracktro.h lib\screen.h
	$(CC) $(CFLAGS) -fo=$^@ src\main.c

src\engine.obj: src\engine.c src\engine.h src\effects.h src\audio.h src\adlib.h src\hwdetect.h lib\screen.h
	$(CC) $(CFLAGS) -fo=$^@ src\engine.c

src\effects.obj: src\effects.c src\effects.h src\engine.h src\hwdetect.h src\audio.h lib\screen.h
	$(CC) $(CFLAGS) -fo=$^@ src\effects.c

src\audio.obj: src\audio.c src\audio.h src\engine.h
	$(CC) $(CFLAGS) -fo=$^@ src\audio.c

src\menu.obj: src\menu.c src\menu.h src\engine.h src\cracktro.h src\hwdetect.h lib\screen.h
	$(CC) $(CFLAGS) -fo=$^@ src\menu.c

src\news.obj: src\news.c src\news.h
	$(CC) $(CFLAGS) -fo=$^@ src\news.c

src\display.obj: src\display.c src\display.h src\hwdetect.h lib\screen.h
	$(CC) $(CFLAGS) -fo=$^@ src\display.c

src\hwdetect.obj: src\hwdetect.c src\hwdetect.h
	$(CC) $(CFLAGS) -fo=$^@ src\hwdetect.c

src\vga13h.obj: src\vga13h.c src\vga13h.h src\engine.h
	$(CC) $(CFLAGS) -fo=$^@ src\vga13h.c

src\title.obj: src\title.c src\title.h src\vga13h.h src\hwdetect.h src\engine.h lib\screen.h
	$(CC) $(CFLAGS) -fo=$^@ src\title.c

src\adlib.obj: src\adlib.c src\adlib.h
	$(CC) $(CFLAGS) -fo=$^@ src\adlib.c

src\climax.obj: src\climax.c src\climax.h src\vga13h.h src\engine.h lib\screen.h
	$(CC) $(CFLAGS) -fo=$^@ src\climax.c

src\cracktro.obj: src\cracktro.c src\cracktro.h src\vga13h.h src\adlib.h src\engine.h src\hwdetect.h lib\screen.h
	$(CC) $(CFLAGS) -fo=$^@ src\cracktro.c

lib\screen.obj: lib\screen.c lib\screen.h
	$(CC) $(CFLAGS) -fo=$^@ lib\screen.c

$(TARGET): $(OBJS)
	$(LD) system dos name $(TARGET) option stack=3072 file { $(OBJS) }

clean: .SYMBOLIC
	-del src\*.obj
	-del lib\*.obj
	-del $(TARGET)
