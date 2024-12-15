#include "memory.h"
#include "file_handling.h"
#include <stdlib.h>

void init_memory(Memory *memory, const char *rom_path){
    memset(memory->data, 0, array_size(memory->data));

    u32 rom_size;
    u8 *rom_data = load_binary_file(rom_path, &rom_size);
    memcpy(memory->data, rom_data, rom_size);

    memory->is_vram_locked = false;
    memory->is_oam_locked = false;
    free(rom_data);
}

u8 read_memory(Memory *memory, u16 address, bool from_gpu){
    assert(address < MEMORY_SIZE);
    if(from_gpu){

    }
    else{
        if(address >= 0x8000 && address <= 0x9FFF && memory->is_vram_locked){ // VRAM
            return 0xFF;
        }
    }
    
    if(address >= 0xFE00 && address <= 0xFE9F && memory->is_oam_locked){ // OAM
        return 0xFF;
    }

    return memory->data[address];
}

void write_memory(Memory *memory, u16 address, u8 value, bool from_gpu){
    assert(address < MEMORY_SIZE);
    if(from_gpu){

    }
    else{
        if(address >= 0x8000 && address <= 0x9FFF && memory->is_vram_locked){ // VRAM
            return;
        }
        else if(address >= 0x0000 && address <= 0x7FFF){ // ROM
            return;
        }
        else if(address == 0xFF04){
            memory->data[address] = 0x00;
        }
    }
    memory->data[address] = value;
}
