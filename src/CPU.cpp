#include "CPU.h"
#include <stdio.h>

void init_cpu(CPU *cpu){
    cpu->clock_speed = 4194304; // Hz
    cpu->do_first_fetch = true;

    cpu->wide_register_map[0] = &cpu->BC;
    cpu->wide_register_map[1] = &cpu->DE;
    cpu->wide_register_map[2] = &cpu->HL;
    cpu->wide_register_map[3] = &cpu->AF;

    cpu->register_map[0] = &cpu->B;
    cpu->register_map[1] = &cpu->C;
    cpu->register_map[2] = &cpu->D;
    cpu->register_map[3] = &cpu->E;
    cpu->register_map[4] = &cpu->H;
    cpu->register_map[5] = &cpu->L;
    cpu->register_map[6] = &cpu->A;
}

u8 fetch(CPU *cpu){
    u8 byte = cpu->memory[cpu->PC];
    cpu->PC++;
    return byte;
}

static u8 imm_low;
static u8 imm_high;
static u16 imm;

i32 run_cpu(CPU *cpu){
    if(cpu->do_first_fetch){
        cpu->machine_cycle = 0;
        cpu->opcode = fetch(cpu);
        cpu->do_first_fetch = false;
        return 4;
    }

    switch(cpu->opcode){
        case 0x00:{  // NOP

            return 4;
        }

        case 0x01:
        case 0x11:
        case 0x21:
        case 0x31:{ // LD r16, imm16
            if(cpu->machine_cycle == 0){
                imm_low = fetch(cpu);
                cpu->machine_cycle++;
            }
            else if(cpu->machine_cycle == 1){
                imm_high = fetch(cpu);
                cpu->machine_cycle++;
            }
            else if(cpu->machine_cycle == 2){
                imm = (imm_high << 8) | imm_low;
                u8 reg = cpu->opcode & 0x30;
                reg >>= 4;
                if(reg <= 2)
                    *cpu->wide_register_map[reg] = imm;
                else
                    cpu->SP = imm;

                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }

            return 4;
        }

        default:{
            printf("Opcode %X not implemented\n", cpu->opcode);
            return 4;
        }
    }
}