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

front_t* front_init(sys_t* sys) {
  front_t* front = malloc(sizeof(front_t));
  front->sys = sys;
  front->tab = FT_SCREEN;
  front->scale = 1;
  return front;
}

void front_deinit(front_t* front) {
  free(front);
}
