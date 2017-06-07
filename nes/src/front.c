#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nfd.h>

#include "error.h"
#include "front.h"
#include "sys.h"

/**
 * front.c
 */

char* front_rom_dialog(void){
  nfdchar_t* nfd_path = NULL;
  nfdresult_t result = NFD_OpenDialog(NULL, NULL, &nfd_path);

  if (result == NFD_OKAY) {
    return (char*)nfd_path;
  }

  if (result != NFD_CANCEL) {
    fprintf(stderr, "Error: %s\n", NFD_GetError());
  }
  return NULL;
}

front* front_init(sys* sys) {
  front* ret = malloc(sizeof(front));
  ret->sys = sys;
  ret->tab = FT_SCREEN;
  ret->scale = 1;
  return ret;
}

void front_deinit(front* front) {
  free(front);
}
