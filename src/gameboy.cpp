#include "gameboy.h"

void init_gameboy(Gameboy *gmb){
    init_cpu(&gmb->cpu);
}

void run_gameboy(Gameboy *gmb){
    CPU *cpu = &gmb->cpu;
    run_cpu(cpu);
}