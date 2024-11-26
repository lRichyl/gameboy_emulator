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

static void write_mem(CPU *cpu, u16 address, u8 value){
    assert(address <= 0xFFFF);
    cpu->memory[address] = value;
}

static u8 read_mem(CPU *cpu, u16 address){
    return cpu->memory[address];
}

void set_flag(CPU *cpu, Flag flag){
    cpu->flags = cpu->flags | flag;
}

void unset_flag(CPU *cpu, Flag flag){
    cpu->flags = cpu->flags & ~(flag);
}

u8 sum_and_set_flags(CPU *cpu, u8 summand_left, u8 summand_right, b32 check_carry = false, bool check_zero = false){
    u16 result = (u16)summand_left + (u16)summand_right;
    
    if(check_zero){
        if(u8(result) == 0)     
            set_flag(cpu, FLAG_ZERO);
        else
            unset_flag(cpu, FLAG_ZERO);
    }

    unset_flag(cpu, FLAG_SUB);

    u8 nibble_left_summand     = summand_left    & 0x0F;
    u8 nibble_right_summand    = summand_right   & 0x0F;
    if((nibble_left_summand + nibble_right_summand) & 0x10) 
        set_flag(cpu, FLAG_HALFCARRY);
    else
        unset_flag(cpu, FLAG_HALFCARRY);

    if(check_carry){
        if(result & 0x100) 
            set_flag(cpu, FLAG_CARRY);
        else
            unset_flag(cpu, FLAG_CARRY);
    }

    return (u8)result;
}

static u8 substract_and_set_flags(CPU *cpu, u8 minuend, u8 sustrahend, b32 check_carry = false, bool check_zero = false){
    u8 result = minuend - sustrahend;

    if(check_zero){
        if(u8(result) == 0)     
            set_flag(cpu, FLAG_ZERO);
        else
            unset_flag(cpu, FLAG_ZERO);
    }

    set_flag(cpu, FLAG_SUB);
  
    u8 nibble_minuend    = minuend    & 0x0F;
    u8 nibble_sustrahend = sustrahend & 0x0F;

    if(nibble_sustrahend > nibble_minuend) 
        set_flag(cpu, FLAG_HALFCARRY);
    else
        unset_flag(cpu, FLAG_HALFCARRY);

    if(check_carry){
        if(sustrahend > minuend){
            if(minuend < sustrahend) 
                set_flag(cpu, FLAG_CARRY);
            else
                unset_flag(cpu, FLAG_CARRY);
        }
    }

    return result;
}

static u8 immr8;
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
            cpu->machine_cycle = 0;

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
                else if(reg == 3)
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
            else if(cpu->machine_cycle == 2){
               imm_high = fetch(cpu);
            }
            else if(cpu->machine_cycle == 3){
               imm = imm = (imm_high << 8) | imm_low;
               write_mem(cpu, imm, cpu->SP & 0x00FF);
               imm++;
            }
            else if(cpu->machine_cycle == 4){
               write_mem(cpu, imm, (cpu->SP & 0xFF00) >> 8);
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

                (*cpu->wide_register_map[reg])++;
            }
            else if(cpu->machine_cycle == 2){
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
            else if(cpu->machine_cycle == 2){
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

                (*cpu->wide_register_map[reg])--;
            }
            else if(cpu->machine_cycle == 2){
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
            else if(cpu->machine_cycle == 2){
                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }
            return 4;
        }

        case 0x09:
        case 0x19:
        case 0x29:{ // ADD HL, r16  for BC, DE and HL.
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                u8 reg = cpu->opcode & 0x30;
                reg >>= 4;
                assert(reg <= 2);

                u8 reg_low = ((*cpu->wide_register_map[reg]) & 0x00FF);
                cpu->L = sum_and_set_flags(cpu, cpu->L, reg_low, true, false);
            }
            else if(cpu->machine_cycle == 2){
                u8 reg = cpu->opcode & 0x30;
                reg >>= 4;
                u8 reg_high = ((*cpu->wide_register_map[reg]) & 0xFF00) >> 8;
                cpu->H =  sum_and_set_flags(cpu, cpu->H, reg_high, true, false);

                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }
            return 4;
        }

        case 0x39:{ // ADD HL, SP
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                u8 reg_low = ((cpu->SP) & 0x00FF);
                cpu->L = sum_and_set_flags(cpu, cpu->L, reg_low, true, false);
            }
            else if(cpu->machine_cycle == 2){
                u8 reg_high = ((cpu->SP) & 0xFF00) >> 8;
                u8 carry = (cpu->flags & FLAG_CARRY) >> 4;
                cpu->H =  sum_and_set_flags(cpu, cpu->H, reg_high + carry, true, false);

                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }
            return 4;
        }

        case 0x04:
        case 0x14:
        case 0x24:
        case 0x0C:
        case 0x1C:
        case 0x2C:
        case 0x3C:{ // INC r8, for B,C,D,E,H,L
            u8 reg = cpu->opcode & 0x38;
            reg >>= 3;
            assert(reg <= 7);

            if(reg < 6)
                (*cpu->register_map[reg]) = sum_and_set_flags(cpu, (*cpu->register_map[reg]), 1, false, true);
            else if(reg == 7){
                cpu->A = sum_and_set_flags(cpu, cpu->A, 1, false, true);
            }

            cpu->opcode = fetch(cpu);
            cpu->machine_cycle = 0;

            return 4;
        }
        
        case 0x34:{ // INC [HL]
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                mem_value = read_mem(cpu, cpu->HL);
            }
            else if(cpu->machine_cycle == 2){
                mem_value = sum_and_set_flags(cpu, mem_value, 1, false, true);
                write_mem(cpu, cpu->HL, mem_value);
            }
            else if(cpu->machine_cycle == 3){
                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }

            return 4;
        }

        case 0x05:
        case 0x15:
        case 0x25:
        case 0x0D:
        case 0x1D:
        case 0x2D:
        case 0x3D:{ // DEC r8, for B,C,D,E,H,L
            u8 reg = cpu->opcode & 0x38;
            reg >>= 3;
            assert(reg <= 7);

            if(reg < 6)
                (*cpu->register_map[reg]) = substract_and_set_flags(cpu, (*cpu->register_map[reg]), 1, false, true);
            else if(reg == 7){
                cpu->A = substract_and_set_flags(cpu, cpu->A, 1, false, true);
            }

            cpu->opcode = fetch(cpu);
            cpu->machine_cycle = 0;

            return 4;
        }

        case 0x35:{ // DEC [HL]
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                mem_value = read_mem(cpu, cpu->HL);
            }
            else if(cpu->machine_cycle == 2){
                mem_value = substract_and_set_flags(cpu, mem_value, 1, false, true);
                write_mem(cpu, cpu->HL, mem_value);
            }
            else if(cpu->machine_cycle == 3){
                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }

            return 4;
        }

        case 0x06:
        case 0x16:
        case 0x26:
        case 0x0E:
        case 0x1E:
        case 0x2E:
        case 0x3E:{ // LD r8, imm8
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                immr8 = fetch(cpu);    
            }
            else if(cpu->machine_cycle == 2){
                u8 reg = cpu->opcode & 0x38;
                reg >>= 3;
                assert(reg <= 7);

                if(reg < 6){
                    (*cpu->register_map[reg]) = immr8;
                }
                else if(reg == 7){
                    cpu->A = immr8;
                }


                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }

            return 4;
        }

        case 0x36:{ // LD [HL], imm8
            cpu->machine_cycle++;
            if(cpu->machine_cycle == 1){
                immr8 = fetch(cpu);    
            }
            else if(cpu->machine_cycle == 2){
                write_mem(cpu, cpu->HL, immr8);

            }else if(cpu->machine_cycle == 3){
                cpu->opcode = fetch(cpu);
                cpu->machine_cycle = 0;
            }

            return 4;
        }

        case 0x07:{ // RLCA
            u8 previous_bit_7 = (cpu->A & 0x80) >> 7;
            previous_bit_7 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);
            cpu->A <<= 1;
            cpu->A = (cpu->A & (~(0x01))) | previous_bit_7;

            unset_flag(cpu, FLAG_ZERO);
            unset_flag(cpu, FLAG_SUB);
            unset_flag(cpu, FLAG_HALFCARRY);

            cpu->opcode = fetch(cpu);
            cpu->machine_cycle = 0;
            return 4;
        }

        case 0x0F:{ // RRCA
            u8 previous_bit_0 = (cpu->A & 0x01);
            previous_bit_0 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);
            cpu->A >>= 1;
            cpu->A = (cpu->A & (~(0x80))) | (previous_bit_0 << 7);

            unset_flag(cpu, FLAG_ZERO);
            unset_flag(cpu, FLAG_SUB);
            unset_flag(cpu, FLAG_HALFCARRY);

            cpu->opcode = fetch(cpu);
            cpu->machine_cycle = 0;

            return 4;
        }

        case 0x17:{ // RLA
            u8 previous_bit_7 = (cpu->A & 0x80) >> 7;
            
            cpu->A <<= 1;
            cpu->A = (cpu->A & (~(0x01))) | ((cpu->flags & FLAG_CARRY) >> 4);

            previous_bit_7 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);

            unset_flag(cpu, FLAG_ZERO);
            unset_flag(cpu, FLAG_SUB);
            unset_flag(cpu, FLAG_HALFCARRY);

            cpu->opcode = fetch(cpu);
            cpu->machine_cycle = 0;

            return 4;
        }

        case 0x1F:{ // RRA
            u8 previous_bit_0 = (cpu->A & 0x01);
            
            cpu->A >>= 1;
            cpu->A = (cpu->A & (~(0x80))) | ((cpu->flags & FLAG_CARRY) << 3);

            previous_bit_0 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);

            unset_flag(cpu, FLAG_ZERO);
            unset_flag(cpu, FLAG_SUB);
            unset_flag(cpu, FLAG_HALFCARRY);

            cpu->opcode = fetch(cpu);
            cpu->machine_cycle = 0;

            return 4;
        }

        case 0x27:{ // DAA
            if((cpu->A & 0x0F) > 9 || cpu->flags & FLAG_HALFCARRY){
                cpu->A += 6;
            }

            unset_flag(cpu, FLAG_HALFCARRY);

            if(cpu->A > 0x99 || cpu->flags & FLAG_CARRY) {
                cpu->A += 0x60;
                set_flag(cpu, FLAG_CARRY);
            }
            else{
                unset_flag(cpu, FLAG_CARRY);
            }

            if(cpu->A == 0)     
                set_flag(cpu, FLAG_ZERO);
            else
                unset_flag(cpu, FLAG_ZERO);

            cpu->opcode = fetch(cpu);
            cpu->machine_cycle = 0;

            return 4;
        }

        case 0x2F:{ // CPL
            cpu->A = ~cpu->A;

            set_flag(cpu, FLAG_HALFCARRY);
            set_flag(cpu, FLAG_SUB);

            cpu->opcode = fetch(cpu);
            cpu->machine_cycle = 0;

            return 4;
        }

        case 0x37:{ // SCF
            set_flag(cpu, FLAG_CARRY);

            unset_flag(cpu, FLAG_HALFCARRY);
            unset_flag(cpu, FLAG_SUB);

            cpu->opcode = fetch(cpu);
            cpu->machine_cycle = 0;
            return 4;
        }

        case 0x3F:{ // CCF
            (cpu->flags & FLAG_CARRY) ? unset_flag(cpu, FLAG_CARRY) : set_flag(cpu, FLAG_CARRY);

            unset_flag(cpu, FLAG_HALFCARRY);
            unset_flag(cpu, FLAG_SUB);

            cpu->opcode = fetch(cpu);
            cpu->machine_cycle = 0;

            return 4;
        }


        default:{
            printf("Opcode %X not implemented\n", cpu->opcode);
            return 4;
        }
    }
}