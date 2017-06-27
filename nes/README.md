# NES emulator "piNES"
## Compiling

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

CMake will automatically find the SDL libraries and compile nativefiledialog.

## Usage

After compilation, the emulator needs to be invoked from the `nes` directory. For example:

```
build/nes
```

This enables it to find `assets/pines.png`.

If an argument is provided on the command line, the emulator treats it as a path to a ROM file, which it will start immediately.
