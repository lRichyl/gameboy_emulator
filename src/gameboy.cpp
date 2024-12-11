#include "gameboy.h"

void init_gameboy(Gameboy *gmb){
    init_memory(&gmb->memory);
    init_cpu(&gmb->cpu, &gmb->memory);

}

void run_gameboy(Gameboy *gmb){
    CPU *cpu = &gmb->cpu;
    run_cpu(cpu);
    // ppu_run(ppu, cpu);

    // Handle interrupts

    // Handle emulator timing
}