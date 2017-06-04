#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nfd.h>

#include "error.h"
#include "front.h"

/**
 * front.c
 */

int front_rom_dialog(char* path){
  nfdchar_t* nfd_path = NULL;
  nfdresult_t result = NFD_OpenDialog(NULL, NULL, &nfd_path);

  if (result == NFD_OKAY) {
    memcpy(path, nfd_path, strlen(nfd_path) + 1);
    free(nfd_path);
    return EC_SUCCESS;
  } else if (result != NFD_CANCEL) {
    fprintf(stderr, "Error: %s\n", NFD_GetError());
  }

  return EC_ERROR;
}

void front_run(front* front){
  front->run(front);
}

void front_deinit(front* front){
  front->deinit(front);
}
