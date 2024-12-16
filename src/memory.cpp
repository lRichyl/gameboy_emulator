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
        if(address >= 0x8000 && address <= 0x9FFF && memory->is_vram_locked){ // VRAM
            memory->data[address];
        }
    }
    else{
        if(address >= 0x8000 && address <= 0x9FFF && memory->is_vram_locked){ // VRAM
            return 0xFF;
        }
        else if(address == 0xFF00){
            u8 joy = memory->data[address];
            joy |= 0xC0;
            joy |= 0x0F; // This are modified with button or dpad presses. 1 means not pressed. TODO: Implement this.
            return joy;
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
            return;
        }
    }
    memory->data[address] = value;
}
