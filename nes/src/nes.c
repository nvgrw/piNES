#include <stdio.h>
#include <time.h>

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
  // Initialise the system
  sys* sys = sys_init();

  // Initialise the front
  front* front = front_init(sys);
  if (front == NULL) {
    fprintf(stderr, "Could not initialise front\n");
    sys_deinit(sys);
    return EXIT_FAILURE;
  }

  // Initialise the front implementation
  front_impl* impl = front_impl_init(front);
  if (impl == NULL) {
    fprintf(stderr, "Could not initialise front implementation\n");
    front_deinit(front);
    sys_deinit(sys);
    return EXIT_FAILURE;
  }

  // Enter main loop
  front_impl_run(impl);

  // Deinit everything, free memory
  front_impl_deinit(impl);
  front_deinit(front);
  sys_deinit(sys);
  return EXIT_SUCCESS;
}
