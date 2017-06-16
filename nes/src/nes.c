#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "error.h"
#include "front.h"
#include "front_impl.h"
#include "ppu.h"
#include "region.h"
#include "sys.h"

/**
 * nes.c
 * 
 * The main entry point of the program.
 */

int main(int argc, char** argv) {
  bool preload_rom = false;

  // Parse arguments
  if (argc > 1) {
    if (!strcmp(argv[1], "-?") || !strcmp(argv[1], "-h") ||
        !strcmp(argv[1], "--help")) {
      printf("usage:\n");
      printf("  build/nes\n");
      printf("    - runs the emulator with no ROM preloaded\n\n");
      printf("  build/nes <rom path>\n");
      printf("    - runs the emulator with the given ROM preloaded,\n");
      printf("      and the system automatically initialised\n");
      printf("    - if <rom path> does not exist, exits immediately\n\n");
      return EXIT_SUCCESS;
    }
    // Check for read permission (thus also existence) of ROM
    if (access(argv[1], R_OK) == -1){
      fprintf(stderr, "cannot read ROM file\n");
      fprintf(stderr, "(build/nes --help for usage info)\n");
      return EXIT_FAILURE;
    }
    preload_rom = true;
  }

  // Initialise the system
  sys_t* sys = sys_init();

  // Initialise the front
  front_t* front = front_init(sys);
  if (front == NULL) {
    fprintf(stderr, "Could not initialise front\n");
    sys_deinit(sys);
    return EXIT_FAILURE;
  }

  // Initialise the front implementation
  front_impl_t* impl = front_impl_init(front);
  if (impl == NULL) {
    fprintf(stderr, "Could not initialise front implementation\n");
    front_deinit(front);
    sys_deinit(sys);
    return EXIT_FAILURE;
  }

  // Load ROM if provided
  if (preload_rom) {
    sys_rom(sys, argv[1]);
    sys_start(sys);
  }

  // Enter main loop
  front_impl_run(impl);

  // Deinit everything, free memory
  front_impl_deinit(impl);
  front_deinit(front);
  sys_deinit(sys);
  return EXIT_SUCCESS;
}
