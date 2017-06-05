#include <stdio.h>
#include <time.h>

#include "front.h"
#include "front_sdl.h"
#include "ppu.h"
#include "SDL.h"

int main(int argc, char** argv) {
  front* front = front_init();
  if (front == NULL) {
    fprintf(stderr, "Could not initialise front\n");
    return 1;
  }

  if (front_sdl_init(front)) {
    free(front);
    fprintf(stderr, "Could not initialise SDL front\n");
    return 1;
  }

  ppu* ppu = ppu_init();
  ppu_cycle(ppu);

  front_run(front);
  front_deinit(front);

  ppu_deinit(ppu);

  return 0;
}
