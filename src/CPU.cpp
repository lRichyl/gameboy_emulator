#include "CPU.h"

void init_cpu(CPU *cpu){
    cpu->clock_speed = 4194304; // Hz
}

void fetch(CPU *cpu){
    cpu->opcode = cpu->memory[cpu->PC];
    cpu->PC++;
}

void decode(CPU *cpu){

}

i32 execute_instruction(CPU *cpu){
    
}