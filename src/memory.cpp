#include "memory.h"
#include "file_handling.h"
#include <stdlib.h>
#include <stdio.h>

#define CARTRIDGE_TYPE_LOC 0x147
#define ROM_SIZE_LOC 0x148
#define RAM_SIZE_LOC 0x149

#define BANK_SIZE 16

static void init_mbc_one(Memory *memory, u8 *rom_data){
    MBC *mbc = &memory->mbc;
    
    u8 value = rom_data[ROM_SIZE_LOC];
    u32 rom_size = 32 * (1 << value);
    mbc->one.used_rom_banks = rom_size / BANK_SIZE; 
    
}

void init_memory(Memory *memory, const char *rom_path){
    memset(memory->data, 0, array_size(memory->data));

    u32 rom_size;
    u8 *rom_data = load_binary_file(rom_path, &rom_size);
    assert(rom_data);
    u8 cartridge_type = rom_data[CARTRIDGE_TYPE_LOC];
    memory->mbc.type = (MBCType)cartridge_type;

    switch(memory->mbc.type){
        case MBC_NONE:{
            memcpy(memory->data, rom_data, rom_size);
            break;
        }

        case MBC_ONE:
        case MBC_ONE_RAM:
        case MBC_ONE_RAM_BATTERY:{
            init_mbc_one(memory, rom_data);

            break;
        }

        default:
            printf("Unknown mapper type\n");
    }   


    memory->is_vram_locked = false;
    memory->is_oam_locked = false;
    free(rom_data);
}