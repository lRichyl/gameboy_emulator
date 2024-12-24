#pragma once

#include "common.h"
#include <string.h>

static const i32 MEMORY_SIZE = 0x10000;


enum MBCType{
    MBC_NONE    = 0x00,
    MBC_ONE     = 0x01,
    MBC_ONE_RAM = 0x02,
    MBC_ONE_RAM_BATTERY = 0x03,
};

struct MBC{
    MBCType type;
    union{
        struct{
            bool has_ram;
            bool has_battery;
            u8 used_rom_banks;
            u8 used_ram_banks;

            u8 **rom_banks;
            u8 **ram_banks;
        }one;
    };
};

struct Memory{
    MBC mbc;

    u8 data[MEMORY_SIZE];
    bool is_vram_locked;
    bool is_oam_locked;
};

void init_memory(Memory *memory, const char *rom_path);



