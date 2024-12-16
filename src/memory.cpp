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