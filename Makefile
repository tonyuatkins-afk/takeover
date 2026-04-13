# TAKEOVER - AI Takeover Simulator for DOS
# Build with OpenWatcom 2.0: wmake

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

# Source files
SRC_FILES = src/main.c src/engine.c src/effects.c src/audio.c &
            src/menu.c src/news.c src/display.c
LIB_FILES = lib/screen.c

# Object files
OBJ_FILES = $(SRC_FILES:.c=.obj) $(LIB_FILES:.c=.obj)

# Target
TARGET = TAKEOVER.EXE

all: $(TARGET)

.c.obj:
	$(CC) $(CFLAGS) -fo=$@ $<

$(TARGET): $(OBJ_FILES)
	$(LD) system dos file { $(OBJ_FILES) } name $(TARGET)

clean: .SYMBOLIC
	del src\*.obj
	del lib\*.obj
	del $(TARGET)
