#include "gameboy.h"

void init_gameboy(Gameboy *gmb, const char *rom_path){
    init_memory(&gmb->memory, rom_path);
    init_cpu(&gmb->cpu, &gmb->memory);
    init_ppu(&gmb->ppu, &gmb->memory);
}

void run_gameboy(Gameboy *gmb){
    CPU *cpu = &gmb->cpu;
    PPU *ppu = &gmb->ppu;
    run_cpu(cpu);
    
    ppu_tick(ppu);
    ppu_tick(ppu);

    // Handle interrupts

    // Handle emulator timing
}