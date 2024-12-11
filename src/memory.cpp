#include "memory.h"

void init_memory(Memory *memory){
    memset(memory->data, 0, array_size(memory->data));
    memory->is_vram_locked = false;
}

u8 read_memory(Memory *memory, u16 address, bool from_gpu){
    assert(address < MEMORY_SIZE);
    if(address >= 0x8000 && address <= 0x9FFF){ // VRAM
        return 0xFF;
    }

    return memory->data[address];
}

void write_memory(Memory *memory, u16 address, u8 value, bool from_gpu){
    assert(address < MEMORY_SIZE);
    if(address >= 0x8000 && address <= 0x9FFF){ // VRAM
        return;
    }
    memory->data[address] = value;
}
