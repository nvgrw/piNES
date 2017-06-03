#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Please provide 1 argument. You provided %d.", argc - 1);
  }

  cpu* cpu = cpu_init();

  /* Read image into memory */
  FILE* fp = fopen(argv[1], "r");
  fseek(fp, 0, SEEK_END);
  size_t size = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  fread(cpu->memory, 1, size, fp);
  fclose(fp);
  /* - */

  cpu->program_counter = 0x400;

  cpu_run(cpu);
  cpu_deinit(cpu);
  return EXIT_SUCCESS;
}
