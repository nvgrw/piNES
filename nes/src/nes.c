#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
#include "rom.h"

int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Please provide 1 argument. You provided %d.\n", argc - 1);
    return EXIT_FAILURE;
  }

  mapper* mapper = NULL;
  rom_error error = rom_load(&mapper, argv[1]);
  printf("Error code: %d\n", error);

  return EXIT_SUCCESS;

  cpu* cpu = cpu_init();

  // /* Read image into memory */
  // FILE* fp = fopen(argv[1], "r");
  // fseek(fp, 0, SEEK_END);
  // size_t size = ftell(fp);
  // fseek(fp, 0, SEEK_SET);
  // fread(cpu->memory, 1, size, fp);
  // fclose(fp);
  // /* - */

  cpu_run(cpu);
  cpu_deinit(cpu);
  return EXIT_SUCCESS;
}
