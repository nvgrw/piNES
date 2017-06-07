# directories
SPC_INCLUDE_DIR=include
SPC_SOURCE_DIR=src
SPC_BUILD_DIR=build
SPC_BINARY=nes
# COM_LIBRARY_DIR =../lib

# compiler configuration, may be overwritten with environment variables
CC ?= gcc
CFLAGS ?= -I$(SPC_INCLUDE_DIR) -Wall -g -D_POSIX_SOURCE -D_BSD_SOURCE -std=c99 -Werror -pedantic

# main rule
all: dir $(SPC_BUILD_DIR)/$(SPC_BINARY)

# specific dependencies
_SDEPS = cpu loader
SDEPS = $(patsubst %,$(SPC_INCLUDE_DIR)/%.h,$(_SDEPS))

# specific object files
_SOBJ = nes cpu loader
SOBJ = $(patsubst %,$(SPC_BUILD_DIR)/%.o,$(_SOBJ))

$(SPC_BUILD_DIR)/%.o: $(SPC_SOURCE_DIR)/%.c $(SDEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

dir:
	mkdir -p $(SPC_BUILD_DIR)

$(SPC_BUILD_DIR)/$(SPC_BINARY): $(SPC_BUILD_DIR)/$(SPC_BINARY).o $(SOBJ)
	gcc -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(SPC_BUILD_DIR)/*.o
