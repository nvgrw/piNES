#include "controller.h"
#include "controller_nes.h"
#include "controller_sdl.h"

/**
 * controller.c
 */

#define CTRL_JOYPAD1 0x4016
#define CTRL_JOYPAD2 0x4017

/**
 * Public functions
 * 
 * See controller.h for descriptions.
 */
controller_t* controller_init(void) {
  return calloc(1, sizeof(controller_t));
}

void controller_clear(controller_t* ctrl) {
  ctrl->pressed1.a = 0;
  ctrl->pressed1.b = 0;
  ctrl->pressed1.select = 0;
  ctrl->pressed1.start = 0;
  ctrl->pressed1.up = 0;
  ctrl->pressed1.down = 0;
  ctrl->pressed1.left = 0;
  ctrl->pressed1.right = 0;
  ctrl->pressed2.a = 0;
  ctrl->pressed2.b = 0;
  ctrl->pressed2.select = 0;
  ctrl->pressed2.start = 0;
  ctrl->pressed2.up = 0;
  ctrl->pressed2.down = 0;
  ctrl->pressed2.left = 0;
  ctrl->pressed2.right = 0;
}

void controller_deinit(controller_t* ctrl) {
  free(ctrl);
}

void controller_mem_write(controller_t* ctrl, uint16_t address, uint8_t value) {
  switch (address) {
    case CTRL_JOYPAD1:
      if (value & 0x1) {
        ctrl->status1 = ctrl->status2 = CS_STROBE;
      } else {
        ctrl->status1 = ctrl->status2 = CS_PULL_A;
      }
      break;
  }
}

uint8_t controller_mem_read(controller_t* ctrl, uint16_t address) {
  if (address == CTRL_JOYPAD1 || address == CTRL_JOYPAD2) {
    controller_status_t* ctrl_status = &ctrl->status1;
    controller_pressed_t* ctrl_pressed = &ctrl->pressed1;
    if (address == CTRL_JOYPAD2) {
      ctrl_status = &ctrl->status2;
      ctrl_pressed = &ctrl->pressed2;
    }
    switch (*ctrl_status) {
      case CS_STROBE: return ctrl_pressed->a;
      case CS_PULL_A: (*ctrl_status)++; return ctrl_pressed->a;
      case CS_PULL_B: (*ctrl_status)++; return ctrl_pressed->b;
      case CS_PULL_SELECT: (*ctrl_status)++; return ctrl_pressed->select;
      case CS_PULL_START: (*ctrl_status)++; return ctrl_pressed->start;
      case CS_PULL_UP: (*ctrl_status)++; return ctrl_pressed->up;
      case CS_PULL_DOWN: (*ctrl_status)++; return ctrl_pressed->down;
      case CS_PULL_LEFT: (*ctrl_status)++; return ctrl_pressed->left;
      case CS_PULL_RIGHT: (*ctrl_status)++; return ctrl_pressed->right;
      default: return 0;
    }
  }
  return 0;
}

const controller_driver_t CONTROLLER_DRIVERS[NUM_CONTROLLER_DRIVERS] = {
#ifdef IS_PI
  {.init = &controller_nes_init, .poll = &controller_nes_poll, .deinit = &controller_nes_deinit},
#endif
  {.init = &controller_sdl_init, .poll = &controller_sdl_poll, .deinit = &controller_sdl_deinit},
};
