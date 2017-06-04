# directories
SPC_INCLUDE_DIR = include
SPC_SOURCE_DIR = src
SPC_BUILD_DIR = build
SPC_BINARY = nes

# library configuration
SDL_CFLAGS ?= $(shell /DevProjects/Libs/SDL/bin/sdl2-config --cflags)
SDL_LDFLAGS ?= $(shell /DevProjects/Libs/SDL/bin/sdl2-config --libs)
SDL_IMAGE_CFLAGS ?= -I/DevProjects/Libs/SDL_image/include/SDL2
SDL_IMAGE_LDFLAGS ?= -L/DevProjects/Libs/SDL_image/lib -lSDL2_image
NFD_CFLAGS = -I/DevProjects/Libs/tpl/nativefiledialog/src/include
NFD_LDFLAGS = -L/DevProjects/Libs/tpl/nativefiledialog/build/lib/Release/x64 -lnfd

# compiler configuration, may be overwritten with environment variables
CC ?= gcc
CFLAGS ?= -I$(SPC_INCLUDE_DIR) -Wall -g -D_POSIX_SOURCE -D_BSD_SOURCE -std=c99 -Werror -pedantic
MOD_CFLAGS = $(CFLAGS) $(SDL_CFLAGS) $(SDL_IMAGE_CFLAGS) $(NFD_CFLAGS)
MOD_LDFLAGS = $(SDL_LDFLAGS) $(SDL_IMAGE_LDFLAGS) $(NFD_LDFLAGS) -framework AppKit

# main rule
all: dir $(SPC_BUILD_DIR)/$(SPC_BINARY)

# specific dependencies
_SDEPS = front front_sdl
SDEPS = $(patsubst %,$(SPC_INCLUDE_DIR)/%.h,$(_SDEPS))

# specific object files
_SOBJ = front front_sdl
SOBJ = $(patsubst %,$(SPC_BUILD_DIR)/%.o,$(_SOBJ))

$(SPC_BUILD_DIR)/%.o: $(SPC_SOURCE_DIR)/%.c $(SDEPS)
	$(CC) -c -o $@ $< $(MOD_CFLAGS)

dir:
	mkdir -p $(SPC_BUILD_DIR)

$(SPC_BUILD_DIR)/$(SPC_BINARY): $(SPC_BUILD_DIR)/$(SPC_BINARY).o $(SOBJ)
	$(CC) -o $@ $^ $(MOD_CFLAGS) $(MOD_LDFLAGS)

.PHONY: clean

clean:
	rm -f $(SPC_BUILD_DIR)/*.o
