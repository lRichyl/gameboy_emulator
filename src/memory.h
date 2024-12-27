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

#define MBC_ONE_ROM_BANKS 32
#define MBC_ONE_RAM_BANKS 4

struct MBC{
    MBCType type;
    bool RAM_enable;
    u8 ROM_bank_number;
    u8 RAM_bank_number;
    union{
        struct{
            bool has_ram;
            bool has_battery;
            bool mode;
            u8 used_rom_banks;
            u8 used_ram_banks;

            u8 *rom_banks[MBC_ONE_ROM_BANKS];
            u8 *ram_banks[MBC_ONE_RAM_BANKS];
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

u8 read_from_MBC(Memory *memory, u16 address);
void set_MBC_registers(Memory *memory, u16 address, u8 value);
void write_to_mbc_RAM(Memory *memory, u16 address, u8 value);
u8 read_ram_from_MBC(Memory *memory, u16 address);



