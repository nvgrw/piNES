# directories
SPC_INCLUDE_DIR = include
SPC_SOURCE_DIR = src
SPC_BUILD_DIR = build
SPC_BINARY = nes

# library configuration
SDL_CFLAGS ?= $(shell sdl2-config --cflags)
SDL_LDFLAGS ?= $(shell sdl2-config --libs)
SDL_IMAGE_DIR ?= libs/SDL_image
SDL_IMAGE_CFLAGS = -I$(SDL_IMAGE_DIR)/include/SDL2
SDL_IMAGE_LDFLAGS = -L$(SDL_IMAGE_DIR)/lib -lSDL2_image
NFD_DIR ?= libs/nativefiledialog
NFD_CFLAGS = -I$(NFD_DIR)/src/include
NFD_LDFLAGS = -L$(NFD_DIR)/build/lib/Release/x64 -lnfd

# compiler configuration, may be overwritten with environment variables
CC ?= gcc
CFLAGS ?= -I$(SPC_INCLUDE_DIR) -Wall -O3 -D_POSIX_SOURCE -D_BSD_SOURCE -std=c99 -Werror -pedantic
MOD_CFLAGS = $(CFLAGS) $(SDL_CFLAGS) $(SDL_IMAGE_CFLAGS) $(NFD_CFLAGS) -D_THREAD_SAFE -DALT_FULLSCREEN
#-DTCP_HOST
MOD_LDFLAGS = $(SDL_LDFLAGS) $(SDL_IMAGE_LDFLAGS) $(NFD_LDFLAGS) -framework AppKit -framework OpenGL -framework Cocoa

# main rule
all: dir $(SPC_BUILD_DIR)/$(SPC_BINARY)

# specific dependencies
_SDEPS = apu apu_channels apu_units controller controller_nes controller_sdl \
	controller_tcp cpu error front front_impl front_sdl mappers ppu profiler \
	region rom sys
SDEPS = $(patsubst %,$(SPC_INCLUDE_DIR)/%.h,$(_SDEPS))

# specific object files
_SOBJ = apu apu_channels apu_units controller controller_nes controller_sdl \
	controller_tcp cpu front front_sdl mappers ppu profiler region rom sys
SOBJ = $(patsubst %,$(SPC_BUILD_DIR)/%.o,$(_SOBJ))

$(SPC_BUILD_DIR)/%.o: $(SPC_SOURCE_DIR)/%.c $(SDEPS)
	$(CC) -c -o $@ $< $(MOD_CFLAGS)

dir:
	mkdir -p $(SPC_BUILD_DIR)

$(SPC_BUILD_DIR)/$(SPC_BINARY): $(SPC_BUILD_DIR)/$(SPC_BINARY).o $(SOBJ)
	$(CC) -o $@ $^ $(MOD_CFLAGS) $(MOD_LDFLAGS)

.PHONY: all clean dir

clean:
	rm -f $(SPC_BUILD_DIR)/*
