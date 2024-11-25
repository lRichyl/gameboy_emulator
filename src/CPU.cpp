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

void write_mem(CPU *cpu, u16 address, u8 value){
    assert(address <= 0xFFFF);
    cpu->memory[address] = value;
}

u8 read_mem(CPU *cpu, u16 address){
    return cpu->memory[address];
}

static u8 imm_low;
static u8 imm_high;
static u16 imm;
static u8 mem_value;

i32 run_cpu(CPU *cpu){
    if(cpu->do_first_fetch){
        cpu->machine_cycle = 0;
        cpu->opcode = fetch(cpu);
        cpu->do_first_fetch = false;
        return 4;
    }

    switch(cpu->opcode){
        case 0x00:{  // NOP
            cpu->opcode = fetch(cpu);
            return 4;
        }

        case 0x01:
        case 0x11:
        case 0x21:
        case 0x31:{ // LD r16, imm16
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                imm_low = fetch(cpu);
            }
            else if(cpu->machine_cycle == 2){
                imm_high = fetch(cpu);
            }
            else if(cpu->machine_cycle == 3){
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

        case 0x02:
        case 0x12:{ // LD [r16], A.    Only BC and DE.
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                u8 reg = cpu->opcode & 0x30;
                reg >>= 4;
                assert(reg <= 1);
                write_mem(cpu, *cpu->wide_register_map[reg], cpu->A);
            }
            else if(cpu->machine_cycle == 2){
                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }

            return 4;
        }


        
        case 0x22:{ // LD [HL+], A
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                write_mem(cpu, cpu->HL, cpu->A);
                cpu->HL++;
            }
            else if(cpu->machine_cycle == 2){
                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }
            return 4;
        }

        case 0x32:{ // LD [HL-], A
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                write_mem(cpu, cpu->HL, cpu->A);
                cpu->HL--;
            }
            else if(cpu->machine_cycle == 2){
                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }
            return 4;
        }

        case 0x0A:
        case 0x1A:{ // LD A, [r16]   only for BC and DE
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                u8 reg = cpu->opcode & 0x30;
                reg >>= 4;
                assert(reg <= 1);
                mem_value = read_mem(cpu, *cpu->wide_register_map[reg]);
            }
            else if(cpu->machine_cycle == 2){
                cpu->A = mem_value;

                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }

            return 4;
        }

        case 0x2A:{ // LD A, [HL+]
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                mem_value = read_mem(cpu, cpu->HL);
                cpu->HL++;
            }
            else if(cpu->machine_cycle == 2){
                cpu->A = mem_value;

                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }
            return 4;
        }

        case 0x3A:{  // LD A, [HL+]
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                mem_value = read_mem(cpu, cpu->HL);
                cpu->HL--;
            }
            else if(cpu->machine_cycle == 2){
                cpu->A = mem_value;

                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }
            return 4;
        }

        case 0x08:{ // LD [a16], SP
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
               imm_low = fetch(cpu);
            }
            if(cpu->machine_cycle == 2){
               imm_high = fetch(cpu);
            }
            if(cpu->machine_cycle == 3){
               imm = imm = (imm_high << 8) | imm_low;
               write_mem(cpu, imm, cpu->SP | 0x0F);
               imm++;
            }
            if(cpu->machine_cycle == 4){
               write_mem(cpu, imm, (cpu->SP | 0xF0) >> 4);
            }
            else if(cpu->machine_cycle == 5){
                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }

            return 4;
        }

        case 0x03:
        case 0x13:
        case 0x23:{ // INC r16   for BC, DE, and HL
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                u8 reg = cpu->opcode & 0x30;
                reg >>= 4;
                assert(reg <= 2);

                *cpu->wide_register_map[reg]++;
            }
            if(cpu->machine_cycle == 2){
                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }
            return 4;
        }

        case 0x33:{ // INC SP
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                cpu->SP++;
            }
            if(cpu->machine_cycle == 2){
                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }
            return 4;
        }

        case 0x0B:
        case 0x1B:
        case 0x2B:{ // DEC r16   for BC, DE, and HL
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                u8 reg = cpu->opcode & 0x30;
                reg >>= 4;
                assert(reg <= 2);

                *cpu->wide_register_map[reg]--;
            }
            if(cpu->machine_cycle == 2){
                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }
            return 4;
        }

        case 0x3B:{ // DEC SP
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                cpu->SP--;
            }
            if(cpu->machine_cycle == 2){
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