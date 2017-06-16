## Extension - NES emulator "pines"

### Compiling

This is the main directory for the extension. The emulator has the following dependencies:

 - [SDL2](https://www.libsdl.org/) - used for audio / graphics output and keyboard input
 - [SDL_Image 2.0](https://www.libsdl.org/projects/SDL_image/) - used for opening the PNG file containing the emulator UI
 - [nativefiledialog](https://github.com/mlabbe/nativefiledialog) - used to show a file open dialog (to select ROM files) on any OS

The preferred way to build the project is to use CMake:

```
cd build
cmake ..
make
```

There is a Makefile included with the project, although it is currently mostly OS X specific (the compiler receives `-framework` flags). To compile, you need to have `sdl2-config` installed and in `$PATH` (otherwise set `$SDL_CFLAGS` and `$SDL_LDFLAGS` manually). The `$SDL_IMAGE_DIR` and `$NFD_DIR` environmental variables have to be set before running `make`, otherwise they default to `libs/SDL_image` and `libs/nativefiledialog`.

### Usage

After compilation, the emulator needs to be invoked from the `nes` directory. For example:

```
build/nes
```

This enables it to find `assets/pines.png`.

If an argument is provided on the command line, the emulator treats it as a path to a ROM file, which it will start immediately.
