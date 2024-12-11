#pragma once

#include "common.h"
#include <string.h>

static const i32 MEMORY_SIZE = 0x10000;

struct Memory{
    u8 data[MEMORY_SIZE];
    bool is_vram_locked;
    bool is_oam_locked;
};

void init_memory(Memory *memory, const char *rom_path);

u8 read_memory(Memory *memory, u16 address, bool from_gpu = false);
void write_memory(Memory *memory, u16 address, u8 value, bool from_gpu = false);

