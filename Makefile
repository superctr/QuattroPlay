#==============================================================================
#
#  QuattroPlay
#  Copyright 2016 Ian Karlsson
#
#==============================================================================

# uncomment if you're building on these platforms
# WINDOWS = 1
# MACOSX = 1

ifndef MACOSX
ifndef WINDOWS
# Recommended on GNU/Linux.
USE_SDL_CONFIG = 1
endif
endif

CC = gcc
CXX = g++
AR = ar
LD = g++

ifdef DEBUG
CFLAGS  := -g -DDEBUG
else
CFLAGS  := -O3 -g0 -DRELEASE
endif
LIB      :=
LIBDIR   :=
INC      := 
LDFLAGS  :=

ifdef USE_SDL_CONFIG
INC      += $(shell sdl2-config --cflags)
LIB      += $(shell sdl2-config --libs)
else 
ifdef WINDOWS
LIB      += -lmingw32 -lSDL2main
LDFLAGS  += -static-libgcc
endif
# Include paths go here (for macOS etc)
INC      +=
LIBDIR   +=
LIB      += -lSDL2
endif

SRC = ./src
OBJ = ./obj
OUT = ./bin
OUTBIN = $(OUT)/QuattroPlay

OBJS = \
	$(OBJ)/drv/helper.o \
	$(OBJ)/drv/quattro.o \
	$(OBJ)/drv/tables.o \
	$(OBJ)/drv/track.o \
	$(OBJ)/drv/track_cmd.o \
	$(OBJ)/drv/update.o \
	$(OBJ)/drv/voice.o \
	$(OBJ)/drv/voice_env.o \
	$(OBJ)/drv/voice_lfo.o \
	$(OBJ)/drv/voice_pan.o \
	$(OBJ)/drv/voice_pitch.o \
	$(OBJ)/drv/wave.o \
	$(OBJ)/ui/info.o \
	$(OBJ)/ui/lib.o \
	$(OBJ)/ui/pattern.o \
	$(OBJ)/ui/scr_about.o \
	$(OBJ)/ui/scr_main.o \
	$(OBJ)/ui/ui.o \
	$(OBJ)/audio.o \
	$(OBJ)/c352.o \
	$(OBJ)/fileio.o \
	$(OBJ)/ini.o \
	$(OBJ)/loader.o \
	$(OBJ)/main.o \
	$(OBJ)/vgm.o

build: $(OBJS)
	@echo linking...
	@mkdir -p $(OUT)
	@$(LD) $(LIBDIR) -o $(OUTBIN) $(OBJS) $(LDFLAGS) $(LIB)

$(OBJ)/%.o: $(SRC)/%.c
	@echo Compiling $< ...
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) $(INC) -c $< -o $@

clean:
	rm -f $(OBJ)/drv/*.o $(OBJ)/*.o $(OUTBIN)

clean_dirs: clean
	rm -rf $(OBJ) $(OUT)

.PHONY: build clean clean_dirs

