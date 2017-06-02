#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"

int main(int argc, char** argv) {
  cpu* cpu = cpu_init();
  cpu_run(cpu);
  cpu_deinit(cpu);
  return EXIT_SUCCESS;
}
