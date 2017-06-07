#include "loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_SIZE 16
#define MAGIC_NO 0x1A53454E

rom_error rom_load(mapper** mappr, const char* path) {
  FILE* fp;
  if (!(fp = fopen(path, "r"))) {
    return RE_READ_ERROR;
  }

  fseek(fp, 0, SEEK_END);
  size_t rom_size = ftell(fp);
  rewind(fp);

  uint8_t header_data[HEADER_SIZE];
  if (fread(header_data, 1, HEADER_SIZE, fp) != HEADER_SIZE) {
    // No complete header present
    fclose(fp);
    return RE_INVALID_FILE_FORMAT;
  }

  // Make sure we're dealing with archaic NES/iNES/NES2.0
  if (*((uint32_t*)header_data) != MAGIC_NO) {
    fclose(fp);
    return RE_INVALID_FILE_FORMAT;
  }

  // Allocate the mapper & assign it to the double pointer
  mapper* mapper;
  mapper = *mappr = malloc(sizeof(mapper));

  // Allocate the header
  rom_header* header = malloc(sizeof(rom_header));
  // Populate the header
  memcpy(header, header_data + 4, HEADER_SIZE - 4);

  // Determine what type of NES format we're dealing with specifically
  // - Archaic doesn't use bytes 8 - 15
  // - iNES doesn't use bytes 10 - 15
  //   Although flags 10 is an unofficial extension
  rom_type type = ROMTYPE_ARCHAIC;
  uint8_t prg_rom_size =
      header->prg_rom | header->flags9.nes2.prg_rom_additional << 4;
  if (header->flags7.data.ines_version == 2 &&
      prg_rom_size <= rom_size / 0x4000) {
    type = ROMTYPE_NES2;
  } else if (header->flags7.data.ines_version == 0 &&
             *((uint32_t*)header_data) == 0) {
    type = ROMTYPE_INES;
  }

  mapper->header = header;
  mapper->type = type;

  // Populate the CPU memory struct

  // Populate the PPU memory struct

  // In case of error, call rom_destroy

  fclose(fp);

  return RE_SUCCESS;
}

void rom_destroy(mapper* mapper) {
  free(mapper->header);
  free(mapper->cpu_memory);
  free(mapper->ppu_memory);
  free(mapper);
}

// Utilities to query the header
size_t rom_get_prg_rom_size(mapper* mappr) {
  size_t size = mappr->header->prg_rom;
  if (mappr->type == ROMTYPE_NES2) {
    size |= ((size_t)mappr->header->flags9.nes2.prg_rom_additional) << 4;
  }

  return size * 0x4000;  // 16 KB units
}

size_t rom_get_chr_rom_size(mapper* mappr) {
  size_t size = mappr->header->chr_rom;
  if (mappr->type == ROMTYPE_NES2) {
    size |= ((size_t)mappr->header->flags9.nes2.chr_rom_additional) << 4;
  }

  return size * 0x4000;  // 16 KB units
}

mirror_type rom_get_mirror_type(mapper* mapper) {
  if (mapper->header->flags6.data.fs_vram) {
    return MIRRORTYPE_4SCREEN;
  }

  return mapper->header->flags6.data.mirroring ? MIRRORTYPE_VERTICAL
                                               : MIRRORTYPE_HORIZONTAL;
}

uint32_t rom_get_mapper_number(mapper* mapper) {
  // We don't currently support NES2.0 submappers

  uint32_t mapper_num = mapper->header->flags6.data.lower_mapper_num;
  mapper_num |= ((uint32_t)mapper->header->flags7.data.upper_mapper_num) << 4;

  if (mapper->type == ROMTYPE_NES2) {
    mapper_num |= ((uint32_t)mapper->header->flags8.nes2.extension_mapper_num)
                  << 8;
  }

  return mapper_num;
}

video_mode rom_get_video_mode(mapper* mapper) {
  if (mapper->type == ROMTYPE_NES2) {
    if (mapper->header->flags12.data.works_for_both) {
      return VMODE_UNIVERSAL;
    }

    if (mapper->header->flags12.data.mode == 1) {
      return VMODE_PAL;
    }

    // Else: Default (NTSC)
  }

  if (mapper->type == ROMTYPE_INES) {
    uint8_t tv_system = mapper->header->flags10.nes1.tv_system;
    if (tv_system == 2) {
      return VMODE_PAL;
    }

    if (tv_system != 0) {
      return VMODE_UNIVERSAL;
    }

    // Else: Default (NTSC)
  }

  return VMODE_NTSC;
}
