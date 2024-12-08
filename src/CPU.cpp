#include "CPU.h"
#include <stdio.h>

void init_cpu(CPU *cpu){
    cpu->clock_speed = 4194304; // Hz
    cpu->do_first_fetch = true;
    cpu->IME = false;
    cpu->scheduled_ei = false;
    cpu->is_extended = false;

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
    cpu->register_map[6] = NULL; // Unused
    cpu->register_map[7] = &cpu->A;
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

u8 pop_stack(CPU *cpu){
    assert(cpu->SP > 0);
    u8 value = read_mem(cpu, cpu->SP);
    cpu->SP++;
    return value;
}

u8 push_stack(CPU *cpu, u8 value){
    assert(cpu->SP > 0);
    cpu->SP--;
    write_mem(cpu, cpu->SP, value);
    return value;
}

static void go_to_next_instruction(CPU *cpu){
    if(cpu->opcode != 0xFB && cpu->scheduled_ei){
        cpu->IME = true;
        cpu->scheduled_ei = false;
    }
    cpu->opcode = fetch(cpu);
    cpu->machine_cycle = 0; 
}

static u8 immr8;


static union{
    u16 imm;
    struct{
        u8 imm_low;
        u8 imm_high;
    };
};
static u8 dest;
static u8 src;

static u8 mem_value;
static bool sign;


i32 run_cpu(CPU *cpu){
    if(cpu->do_first_fetch){
        cpu->machine_cycle = 0;
        cpu->opcode = fetch(cpu);
        cpu->do_first_fetch = false;
        return 4;
    }
    cpu->machine_cycle++;
    if(!cpu->is_extended){
        switch(cpu->opcode & 0xC0){
        case 0x00:{
            switch(cpu->opcode){
                case 0x00:{  // NOP
                    go_to_next_instruction(cpu);

                    return 4;
                }

                case 0x01:
                case 0x11:
                case 0x21:
                case 0x31:{ // LD r16, imm16
                    
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

                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0x02:
                case 0x12:{ // LD [r16], A.    Only BC and DE.
                    
                    if(cpu->machine_cycle == 1){
                        u8 reg = cpu->opcode & 0x30;
                        reg >>= 4;
                        assert(reg <= 1);
                        write_mem(cpu, *cpu->wide_register_map[reg], cpu->A);
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }


                
                case 0x22:{ // LD [HL+], A
                    
                    if(cpu->machine_cycle == 1){
                        write_mem(cpu, cpu->HL, cpu->A);
                        cpu->HL++;
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }
                    return 4;
                }

                case 0x32:{ // LD [HL-], A
                    
                    if(cpu->machine_cycle == 1){
                        write_mem(cpu, cpu->HL, cpu->A);
                        cpu->HL--;
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }
                    return 4;
                }

                case 0x0A:
                case 0x1A:{ // LD A, [r16]   only for BC and DE
                    
                    if(cpu->machine_cycle == 1){
                        u8 reg = cpu->opcode & 0x30;
                        reg >>= 4;
                        assert(reg <= 1);
                        mem_value = read_mem(cpu, *cpu->wide_register_map[reg]);
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = mem_value;

                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0x2A:{ // LD A, [HL+]
                    
                    if(cpu->machine_cycle == 1){
                        mem_value = read_mem(cpu, cpu->HL);
                        cpu->HL++;
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = mem_value;

                        go_to_next_instruction(cpu);
                    }
                    return 4;
                }

                case 0x3A:{  // LD A, [HL+]
                    
                    if(cpu->machine_cycle == 1){
                        mem_value = read_mem(cpu, cpu->HL);
                        cpu->HL--;
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = mem_value;

                        go_to_next_instruction(cpu);
                    }
                    return 4;
                }

                case 0x08:{ // LD [a16], SP
                    
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
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0x03:
                case 0x13:
                case 0x23:{ // INC r16   for BC, DE, and HL
                    
                    if(cpu->machine_cycle == 1){
                        u8 reg = cpu->opcode & 0x30;
                        reg >>= 4;
                        assert(reg <= 2);

                        (*cpu->wide_register_map[reg])++;
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }
                    return 4;
                }

                case 0x33:{ // INC SP
                    
                    if(cpu->machine_cycle == 1){
                        cpu->SP++;
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }
                    return 4;
                }

                case 0x0B:
                case 0x1B:
                case 0x2B:{ // DEC r16   for BC, DE, and HL
                    
                    if(cpu->machine_cycle == 1){
                        u8 reg = cpu->opcode & 0x30;
                        reg >>= 4;
                        assert(reg <= 2);

                        (*cpu->wide_register_map[reg])--;
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }
                    return 4;
                }

                case 0x3B:{ // DEC SP
                    
                    if(cpu->machine_cycle == 1){
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }
                    return 4;
                }

                case 0x09:
                case 0x19:
                case 0x29:{ // ADD HL, r16  for BC, DE and HL.
                    
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

                        go_to_next_instruction(cpu);
                    }
                    return 4;
                }

                case 0x39:{ // ADD HL, SP
                    
                    if(cpu->machine_cycle == 1){
                        u8 reg_low = ((cpu->SP) & 0x00FF);
                        cpu->L = sum_and_set_flags(cpu, cpu->L, reg_low, true, false);
                    }
                    else if(cpu->machine_cycle == 2){
                        u8 reg_high = ((cpu->SP) & 0xFF00) >> 8;
                        u8 carry = (cpu->flags & FLAG_CARRY) >> 4;
                        cpu->H =  sum_and_set_flags(cpu, cpu->H, reg_high + carry, true, false);

                        go_to_next_instruction(cpu);
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

                    go_to_next_instruction(cpu);

                    return 4;
                }
                
                case 0x34:{ // INC [HL]
                    
                    if(cpu->machine_cycle == 1){
                        mem_value = read_mem(cpu, cpu->HL);
                    }
                    else if(cpu->machine_cycle == 2){
                        mem_value = sum_and_set_flags(cpu, mem_value, 1, false, true);
                        write_mem(cpu, cpu->HL, mem_value);
                    }
                    else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
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

                    go_to_next_instruction(cpu);

                    return 4;
                }

                case 0x35:{ // DEC [HL]
                    
                    if(cpu->machine_cycle == 1){
                        mem_value = read_mem(cpu, cpu->HL);
                    }
                    else if(cpu->machine_cycle == 2){
                        mem_value = substract_and_set_flags(cpu, mem_value, 1, false, true);
                        write_mem(cpu, cpu->HL, mem_value);
                    }
                    else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
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


                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0x36:{ // LD [HL], imm8
                    
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);    
                    }
                    else if(cpu->machine_cycle == 2){
                        write_mem(cpu, cpu->HL, immr8);

                    }else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
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

                    go_to_next_instruction(cpu);
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

                    go_to_next_instruction(cpu);

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

                    go_to_next_instruction(cpu);

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

                    go_to_next_instruction(cpu);

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

                    go_to_next_instruction(cpu);

                    return 4;
                }

                case 0x2F:{ // CPL
                    cpu->A = ~cpu->A;

                    set_flag(cpu, FLAG_HALFCARRY);
                    set_flag(cpu, FLAG_SUB);

                    go_to_next_instruction(cpu);

                    return 4;
                }

                case 0x37:{ // SCF
                    set_flag(cpu, FLAG_CARRY);

                    unset_flag(cpu, FLAG_HALFCARRY);
                    unset_flag(cpu, FLAG_SUB);

                    go_to_next_instruction(cpu);
                    return 4;
                }

                case 0x3F:{ // CCF
                    (cpu->flags & FLAG_CARRY) ? unset_flag(cpu, FLAG_CARRY) : set_flag(cpu, FLAG_CARRY);

                    unset_flag(cpu, FLAG_HALFCARRY);
                    unset_flag(cpu, FLAG_SUB);

                    go_to_next_instruction(cpu);

                    return 4;
                }

                case 0x18:{ // JR imm8
                    
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);    
                    }
                    else if(cpu->machine_cycle == 2){
                        bool sign = immr8 & 0x80;
                        immr8  = sum_and_set_flags(cpu, immr8, cpu->PCL, true, true);
                        i8 adj = 0;
                        if((cpu->flags & FLAG_CARRY) && !sign){
                            adj = 1;
                        }
                        else if(!(cpu->flags & FLAG_CARRY) && sign){
                            adj = -1;
                        }

                        mem_value = cpu->PCH + adj;


                        // i8 imm8s = (i8)immr8;
                        u16 target = (mem_value << 8) | immr8;
                        cpu->PC = target;

                    }else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0x20:{ // JR NZ, imm8
                    
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);    
                    }
                    else if(cpu->machine_cycle == 2){
                        if(!(cpu->flags & FLAG_ZERO)){
                            i8 imm8s = (i8)immr8;
                            u16 target = cpu->PC - 1 + imm8s;
                            cpu->PC = target;
                        }
                        else{
                            go_to_next_instruction(cpu);    
                        }

                    }else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0x30:{ // JR NC, imm8
                    
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);    
                    }
                    else if(cpu->machine_cycle == 2){
                        if(!(cpu->flags & FLAG_CARRY)){
                            i8 imm8s = (i8)immr8;
                            u16 target = cpu->PC - 1 + imm8s;
                            cpu->PC = target;
                        }
                        else{
                            go_to_next_instruction(cpu);    
                        }

                    }else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0x28:{ // JR Z, imm8
                    
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);    
                    }
                    else if(cpu->machine_cycle == 2){
                        if((cpu->flags & FLAG_ZERO)){
                            i8 imm8s = (i8)immr8;
                            u16 target = cpu->PC - 1 + imm8s;
                            cpu->PC = target;
                        }
                        else{
                            go_to_next_instruction(cpu);    
                        }

                    }else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0x38:{ // JR C, imm8
                    
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);    
                    }
                    else if(cpu->machine_cycle == 2){
                        if((cpu->flags & FLAG_CARRY)){
                            i8 imm8s = (i8)immr8;
                            u16 target = cpu->PC - 1 + imm8s;
                            cpu->PC = target;
                        }
                        else{
                            go_to_next_instruction(cpu);    
                        }

                    }else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                // TODO
                // case 0x10:{ // STOP
                //     immr8 = fetch(cpu);

                //     cpu->opcode = fetch(cpu);
                //     cpu->machine_cycle = 0;
                //     return 4;
                // }



                
            }                
            break;
        }

        case 0x40:{ // LD r8, r8 instructions
            
            if(cpu->machine_cycle == 1){
                dest = (cpu->opcode & 0x38) >> 3;
                src  = (cpu->opcode & 0x07);

                assert(dest <= 7);
                assert(src <= 7);

                if(dest == 6 && src == 6){
                    // TODO: HALT instruction.
                    go_to_next_instruction(cpu);
                }
                else if(dest == 6 && src != 6){ // LD [HL], r8
                    write_mem(cpu, cpu->HL, *cpu->register_map[src]);
                }
                else if(dest != 6 && src == 6){ // LD r8, [HL]
                    mem_value = read_mem(cpu, cpu->HL);
                }
                else{
                    assert(dest != 6);
                    assert(src  != 6);
                    (*cpu->register_map[dest]) = (*cpu->register_map[src]);
                    go_to_next_instruction(cpu); 
                }
            }
            else if(cpu->machine_cycle == 2){
                if(dest != 6 && src == 6){ // LD r8, [HL]
                    *cpu->register_map[dest] = mem_value;
                }

                go_to_next_instruction(cpu);
            }
            return 4;
        }

        case 0x80:{
            switch(cpu->opcode & 0x38){
                case 0x00:{ // ADD A, r
                    if(cpu->machine_cycle == 1){
                        u8 reg  = (cpu->opcode & 0x07);
                        assert(reg <= 7 && reg >= 0);

                        if(reg != 6){
                            cpu->A = sum_and_set_flags(cpu, cpu->A, *cpu->register_map[reg], true, true);
                            go_to_next_instruction(cpu);
                        }
                        else if (reg == 6){ // ADD A, [HL]
                            mem_value = read_mem(cpu, cpu->HL);
                        }

                    }
                    else if(cpu->machine_cycle == 2){
                        // ADD A, [HL]
                        cpu->A = sum_and_set_flags(cpu, cpu->A, mem_value, true, true);
                        
                        go_to_next_instruction(cpu);
                    }
                    return 4;
                }

                case 0x08:{ // ADC A, r
                    if(cpu->machine_cycle == 1){
                        u8 reg  = (cpu->opcode & 0x07);
                        assert(reg <= 7 && reg >= 0);

                        u8 carry = (cpu->flags & FLAG_CARRY) >> 4;

                        if(reg != 6){
                            cpu->A = sum_and_set_flags(cpu, cpu->A, *cpu->register_map[reg] + carry, true, true);
                            go_to_next_instruction(cpu);
                        }
                        else if (reg == 6){ // ADC A, [HL]
                            mem_value = read_mem(cpu, cpu->HL);
                        }

                    }
                    else if(cpu->machine_cycle == 2){
                        // ADC A, [HL]
                        u8 carry = (cpu->flags & FLAG_CARRY) >> 4;
                        cpu->A = sum_and_set_flags(cpu, cpu->A, mem_value + carry, true, true);
                        
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0x10:{ // SUB A, r
                    if(cpu->machine_cycle == 1){
                        u8 reg  = (cpu->opcode & 0x07);
                        assert(reg <= 7 && reg >= 0);

                        if(reg != 6){
                            cpu->A = substract_and_set_flags(cpu, cpu->A, *cpu->register_map[reg], true, true);
                            go_to_next_instruction(cpu);
                        }
                        else if (reg == 6){ // SUB A, [HL]
                            mem_value = read_mem(cpu, cpu->HL);
                        }

                    }
                    else if(cpu->machine_cycle == 2){
                        // SUB A, [HL]
                        cpu->A = substract_and_set_flags(cpu, cpu->A, mem_value, true, true);
                        
                        go_to_next_instruction(cpu);
                    }
                    return 4;
                }

                case 0x18:{ // SBC A, r
                    if(cpu->machine_cycle == 1){
                        u8 reg  = (cpu->opcode & 0x07);
                        assert(reg <= 7 && reg >= 0);

                        u8 carry = (cpu->flags & FLAG_CARRY) >> 4;

                        if(reg != 6){
                            cpu->A = substract_and_set_flags(cpu, cpu->A, *cpu->register_map[reg] + carry, true, true);
                            go_to_next_instruction(cpu);
                        }
                        else if (reg == 6){ // SBC A, [HL]
                            mem_value = read_mem(cpu, cpu->HL);
                        }

                    }
                    else if(cpu->machine_cycle == 2){
                        // SBC A, [HL]
                        u8 carry = (cpu->flags & FLAG_CARRY) >> 4;
                        cpu->A = substract_and_set_flags(cpu, cpu->A, mem_value + carry, true, true);
                        
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0x20:{ // AND A, r
                    if(cpu->machine_cycle == 1){
                        u8 reg  = (cpu->opcode & 0x07);
                        assert(reg <= 7 && reg >= 0);

                        if(reg != 6){
                            cpu->A = cpu->A & (*cpu->register_map[reg]);
                            
                            cpu->A == 0 ? set_flag(cpu, FLAG_ZERO) : unset_flag(cpu, FLAG_ZERO);
                            unset_flag(cpu, FLAG_CARRY);
                            unset_flag(cpu, FLAG_SUB);
                            set_flag(cpu, FLAG_HALFCARRY);
                                
                            go_to_next_instruction(cpu);
                        }
                        else if (reg == 6){ // AND A, [HL]
                            mem_value = read_mem(cpu, cpu->HL);
                        }

                    }
                    else if(cpu->machine_cycle == 2){
                        // AND A, [HL]
                        cpu->A = cpu->A & mem_value;
                            
                        cpu->A == 0 ? set_flag(cpu, FLAG_ZERO) : unset_flag(cpu, FLAG_ZERO);
                        unset_flag(cpu, FLAG_CARRY);
                        unset_flag(cpu, FLAG_SUB);
                        set_flag(cpu, FLAG_HALFCARRY);
                        
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0x28:{ // XOR A, r
                    if(cpu->machine_cycle == 1){
                        u8 reg  = (cpu->opcode & 0x07);
                        assert(reg <= 7 && reg >= 0);

                        if(reg != 6){
                            cpu->A = cpu->A ^ (*cpu->register_map[reg]);
                            
                            cpu->A == 0 ? set_flag(cpu, FLAG_ZERO) : unset_flag(cpu, FLAG_ZERO);
                            unset_flag(cpu, FLAG_CARRY);
                            unset_flag(cpu, FLAG_SUB);
                            unset_flag(cpu, FLAG_HALFCARRY);
                                
                            go_to_next_instruction(cpu);
                        }
                        else if (reg == 6){ // XOR A, [HL]
                            mem_value = read_mem(cpu, cpu->HL);
                        }

                    }
                    else if(cpu->machine_cycle == 2){
                        // XOR A, [HL]
                        cpu->A = cpu->A ^ mem_value;
                            
                        cpu->A == 0 ? set_flag(cpu, FLAG_ZERO) : unset_flag(cpu, FLAG_ZERO);
                        unset_flag(cpu, FLAG_CARRY);
                        unset_flag(cpu, FLAG_SUB);
                        unset_flag(cpu, FLAG_HALFCARRY);
                        
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0x30:{ // OR A, r
                    if(cpu->machine_cycle == 1){
                        u8 reg  = (cpu->opcode & 0x07);
                        assert(reg <= 7 && reg >= 0);

                        if(reg != 6){
                            cpu->A = cpu->A | (*cpu->register_map[reg]);
                            
                            cpu->A == 0 ? set_flag(cpu, FLAG_ZERO) : unset_flag(cpu, FLAG_ZERO);
                            unset_flag(cpu, FLAG_CARRY);
                            unset_flag(cpu, FLAG_SUB);
                            unset_flag(cpu, FLAG_HALFCARRY);
                                
                            go_to_next_instruction(cpu);
                        }
                        else if (reg == 6){ // OR A, [HL]
                            mem_value = read_mem(cpu, cpu->HL);
                        }

                    }
                    else if(cpu->machine_cycle == 2){
                        // OR A, [HL]
                        cpu->A = cpu->A | mem_value;
                            
                        cpu->A == 0 ? set_flag(cpu, FLAG_ZERO) : unset_flag(cpu, FLAG_ZERO);
                        unset_flag(cpu, FLAG_CARRY);
                        unset_flag(cpu, FLAG_SUB);
                        unset_flag(cpu, FLAG_HALFCARRY);
                        
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0x38:{ // CP A, r
                    if(cpu->machine_cycle == 1){
                        u8 reg  = (cpu->opcode & 0x07);
                        assert(reg <= 7 && reg >= 0);

                        if(reg != 6){
                            substract_and_set_flags(cpu, cpu->A, *cpu->register_map[reg], true, true);
                            go_to_next_instruction(cpu);
                        }
                        else if (reg == 6){ // CP A, [HL]
                            mem_value = read_mem(cpu, cpu->HL);
                        }

                    }
                    else if(cpu->machine_cycle == 2){
                        // CP A, [HL]
                        substract_and_set_flags(cpu, cpu->A, mem_value, true, true);
                        
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }
            }

            break;
        }

        case 0xC0:{
            switch(cpu->opcode){
                case 0xC6:{ // ADD A, imm8
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = sum_and_set_flags(cpu, cpu->A, immr8, true, true);     
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xCE:{ // ADC A, imm8
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        u8 carry = (cpu->flags & FLAG_CARRY) >> 4;
                        cpu->A = sum_and_set_flags(cpu, cpu->A, immr8 + carry, true, true);     
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xD6:{ // SUB A, imm8
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = substract_and_set_flags(cpu, cpu->A, immr8, true, true);     
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xDE:{ // SBC A, imm8
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        u8 carry = (cpu->flags & FLAG_CARRY) >> 4;
                        cpu->A = substract_and_set_flags(cpu, cpu->A, immr8 + carry, true, true);     
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xE6:{ // AND A, imm8
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = cpu->A & immr8;

                        cpu->A == 0 ? set_flag(cpu, FLAG_ZERO) : unset_flag(cpu, FLAG_ZERO);
                        unset_flag(cpu, FLAG_CARRY);
                        unset_flag(cpu, FLAG_SUB);
                        set_flag(cpu, FLAG_HALFCARRY);

                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xEE:{ // XOR A, imm8
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = cpu->A ^ immr8;
                        
                        cpu->A == 0 ? set_flag(cpu, FLAG_ZERO) : unset_flag(cpu, FLAG_ZERO);
                        unset_flag(cpu, FLAG_CARRY);
                        unset_flag(cpu, FLAG_SUB);
                        unset_flag(cpu, FLAG_HALFCARRY);

                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xF6:{ // OR A, imm8
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = cpu->A | immr8;
                        
                        cpu->A == 0 ? set_flag(cpu, FLAG_ZERO) : unset_flag(cpu, FLAG_ZERO);
                        unset_flag(cpu, FLAG_CARRY);
                        unset_flag(cpu, FLAG_SUB);
                        unset_flag(cpu, FLAG_HALFCARRY);

                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xFE:{ // CP A, imm8
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        substract_and_set_flags(cpu, cpu->A, immr8, true, true);  

                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xC0:{ // RET NZ
                    if(cpu->machine_cycle == 1){
                        if(cpu->flags & FLAG_ZERO) cpu->machine_cycle = 4; // Will be 5 next cycle
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_low = pop_stack(cpu);
                    }
                    else if(cpu->machine_cycle == 3){
                        imm_high = pop_stack(cpu);
                    }
                    else if(cpu->machine_cycle == 4){
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 5){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xC8:{ // RET Z
                    if(cpu->machine_cycle == 1){
                        if(!(cpu->flags & FLAG_ZERO)) cpu->machine_cycle = 4; // Will be 5 next cycle
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_low = pop_stack(cpu);
                    }
                    else if(cpu->machine_cycle == 3){
                        imm_high = pop_stack(cpu);
                    }
                    else if(cpu->machine_cycle == 4){
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 5){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xD0:{ // RET NC
                    if(cpu->machine_cycle == 1){
                        if(cpu->flags & FLAG_CARRY) cpu->machine_cycle = 4; // Will be 5 next cycle
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_low = pop_stack(cpu);
                    }
                    else if(cpu->machine_cycle == 3){
                        imm_high = pop_stack(cpu);
                    }
                    else if(cpu->machine_cycle == 4){
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 5){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xD8:{ // RET C
                    if(cpu->machine_cycle == 1){
                        if(!(cpu->flags & FLAG_CARRY)) cpu->machine_cycle = 4; // Will be 5 next cycle
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_low = pop_stack(cpu);
                    }
                    else if(cpu->machine_cycle == 3){
                        imm_high = pop_stack(cpu);
                    }
                    else if(cpu->machine_cycle == 4){
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 5){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xC9:{ // RET
                    if(cpu->machine_cycle == 1){
                        imm_low = pop_stack(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = pop_stack(cpu);
                    }
                    else if(cpu->machine_cycle == 3){
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 4){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xD9:{ // RET
                    if(cpu->machine_cycle == 1){
                        imm_low = pop_stack(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = pop_stack(cpu);
                    }
                    else if(cpu->machine_cycle == 3){
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                        cpu->IME = true;
                    }
                    else if(cpu->machine_cycle == 4){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xC2:{ // JP NZ, imm16
                    if(cpu->machine_cycle == 1){
                        imm_low = fetch(cpu);;
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = fetch(cpu);
                        if((cpu->flags & FLAG_ZERO)){
                            cpu->machine_cycle = 3;
                        }
                    }
                    else if(cpu->machine_cycle == 3){
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 4){
                        go_to_next_instruction(cpu);
                    }
                    return 4;
                }

                case 0xCA:{ // JP Z, imm16
                    if(cpu->machine_cycle == 1){
                        imm_low = fetch(cpu);;
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = fetch(cpu);
                        if(!(cpu->flags & FLAG_ZERO)){
                            cpu->machine_cycle = 3;
                        }
                    }
                    else if(cpu->machine_cycle == 3){
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 4){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xD2:{ // JP NC, imm16
                    if(cpu->machine_cycle == 1){
                        imm_low = fetch(cpu);;
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = fetch(cpu);
                        if((cpu->flags & FLAG_CARRY)){
                            cpu->machine_cycle = 3;
                        }
                    }
                    else if(cpu->machine_cycle == 3){
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 4){
                        go_to_next_instruction(cpu);
                    }
                    return 4;
                }

                case 0xDA:{ // JP C, imm16
                    if(cpu->machine_cycle == 1){
                        imm_low = fetch(cpu);;
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = fetch(cpu);
                        if(!(cpu->flags & FLAG_CARRY)){
                            cpu->machine_cycle = 3;
                        }
                    }
                    else if(cpu->machine_cycle == 3){
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 4){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xC3:{ // JP imm16
                    if(cpu->machine_cycle == 1){
                        imm_low = fetch(cpu);;
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 3){
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 4){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xE9:{ // JP HL
                    cpu->PC = cpu->HL;
                    go_to_next_instruction(cpu);

                    return 4;
                }

                case 0xC4:{ // CALL NZ, imm16
                    if(cpu->machine_cycle == 1){
                        imm_low = fetch(cpu);;
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = fetch(cpu);
                        if(cpu->flags & FLAG_ZERO){
                            cpu->machine_cycle = 5;
                        }
                    }
                    else if(cpu->machine_cycle == 3){
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 4){
                        assert(cpu->SP > 0);
                        write_mem(cpu, cpu->SP, cpu->PCH);
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 5){
                        write_mem(cpu, cpu->SP, cpu->PCL);
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 6){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xCC:{ // CALL Z, imm16
                    if(cpu->machine_cycle == 1){
                        imm_low = fetch(cpu);;
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = fetch(cpu);
                        if(!(cpu->flags & FLAG_ZERO)){
                            cpu->machine_cycle = 5;
                        }
                    }
                    else if(cpu->machine_cycle == 3){
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 4){
                        assert(cpu->SP > 0);
                        write_mem(cpu, cpu->SP, cpu->PCH);
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 5){
                        write_mem(cpu, cpu->SP, cpu->PCL);
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 6){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xD4:{ // CALL NC, imm16
                    if(cpu->machine_cycle == 1){
                        imm_low = fetch(cpu);;
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = fetch(cpu);
                        if(cpu->flags & FLAG_CARRY){
                            cpu->machine_cycle = 5;
                        }
                    }
                    else if(cpu->machine_cycle == 3){
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 4){
                        assert(cpu->SP > 0);
                        write_mem(cpu, cpu->SP, cpu->PCH);
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 5){
                        write_mem(cpu, cpu->SP, cpu->PCL);
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 6){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xDC:{ // CALL C, imm16
                    if(cpu->machine_cycle == 1){
                        imm_low = fetch(cpu);;
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = fetch(cpu);
                        if(!(cpu->flags & FLAG_CARRY)){
                            cpu->machine_cycle = 5;
                        }
                    }
                    else if(cpu->machine_cycle == 3){
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 4){
                        assert(cpu->SP > 0);
                        write_mem(cpu, cpu->SP, cpu->PCH);
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 5){
                        write_mem(cpu, cpu->SP, cpu->PCL);
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 6){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xCD:{ // CALL imm16
                    if(cpu->machine_cycle == 1){
                        imm_low = fetch(cpu);;
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 3){
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 4){
                        assert(cpu->SP > 0);
                        write_mem(cpu, cpu->SP, cpu->PCH);
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 5){
                        write_mem(cpu, cpu->SP, cpu->PCL);
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 6){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xC7:
                case 0xD7:
                case 0xE7:
                case 0xF7:
                case 0xCF:
                case 0xDF:
                case 0xEF:
                case 0xFF:{ // RST
                    if(cpu->machine_cycle == 1){
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 2){
                        assert(cpu->SP > 0);
                        write_mem(cpu, cpu->SP, cpu->PCH);
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 3){
                        write_mem(cpu, cpu->SP, cpu->PCL);
                        u8 target = (cpu->opcode & 0x38) >> 3;
                        u16 address = target * 0x08;
                        cpu->PC = address;
                    }
                    else if(cpu->machine_cycle == 4){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xC1:
                case 0xD1:
                case 0xE1:
                case 0xF1:{ // POP r16
                    if(cpu->machine_cycle == 1){
                        imm_low = read_mem(cpu, cpu->SP);
                        cpu->SP++;
                    }
                    else if(cpu->machine_cycle == 2){
                        assert(cpu->SP > 0);
                        imm_high = read_mem(cpu, cpu->SP);
                        cpu->SP++;
                    }
                    else if(cpu->machine_cycle == 3){
                        u8 target = (cpu->opcode & 0x30) >> 4;
                        *(cpu->wide_register_map[target]) = (imm_high << 8) | imm_low;
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xC5:
                case 0xD5:
                case 0xE5:
                case 0xF5:{ // PUSH r16
                    if(cpu->machine_cycle == 1){
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 2){
                        assert(cpu->SP > 0);
                        u8 src = (cpu->opcode & 0x30) >> 4;
                        write_mem(cpu, cpu->SP, *(cpu->register_map[(src * 2)]));
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 3){
                        u8 src = (cpu->opcode & 0x30) >> 4;
                        write_mem(cpu, cpu->SP, *(cpu->register_map[(src * 2) + 1]));
                    }
                    else if(cpu->machine_cycle == 4){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xCB:{
                    cpu->is_extended = true;
                    go_to_next_instruction(cpu);
                    return 4;
                }

                case 0xE2:{ // LDH [C], A
                    if(cpu->machine_cycle == 1){
                        u16 address = 0xFF00 + cpu->C;
                        write_mem(cpu, address, cpu->A);
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                } 

                case 0xE0:{ // LDH [imm8], A
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        u16 address = 0xFF00 + immr8;
                        write_mem(cpu, address, cpu->A);
                    }
                    else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
                    }
                        
                    return 4;
                }

                case 0xEA:{ // LDH [imm16], A
                    if(cpu->machine_cycle == 1){
                        imm_low = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 3){
                        write_mem(cpu, imm, cpu->A);
                    }
                    else if(cpu->machine_cycle == 4){
                        go_to_next_instruction(cpu);
                    }
                        
                    return 4;
                }

                case 0xF2:{ // LDH A, [C]
                    if(cpu->machine_cycle == 1){
                        u16 address = 0xFF00 + cpu->C;
                        mem_value = read_mem(cpu, address);
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = mem_value;
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                } 

                case 0xF0:{ // LDH A, [imm8]
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        u16 address = 0xFF00 + immr8;
                        mem_value = read_mem(cpu, address);
                    }
                    else if(cpu->machine_cycle == 3){
                        cpu->A = mem_value;
                        go_to_next_instruction(cpu);
                    }
                        
                    return 4;
                }

                case 0xFA:{ // LDH A, [imm16]
                    if(cpu->machine_cycle == 1){
                        imm_low = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 3){
                        mem_value = read_mem(cpu, imm);
                    }
                    else if(cpu->machine_cycle == 4){
                        cpu->A = mem_value;
                        go_to_next_instruction(cpu);
                    }
                        
                    return 4;
                }

                case 0xE8:{ // ADD SP, e
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                        // sign = immr8 & 0x80;
                    }
                    else if(cpu->machine_cycle == 2){
                        sum_and_set_flags(cpu, cpu->SPL, immr8, true, true);
                    }
                    else if(cpu->machine_cycle == 3){
                        
                        // i8 adj = 0;
                        // if((cpu->flags & FLAG_CARRY) && !sign){
                        //     adj = 1;
                        // }
                        // else if(!(cpu->flags & FLAG_CARRY) && sign){
                        //     adj = -1;
                        // }
                        // mem_value = cpu->SPH + adj + ((cpu->flags & FLAG_CARRY) >> 4);   
                    }
                    else if(cpu->machine_cycle == 4){
                        i8 imm8s = (i8)immr8;
                        cpu->SP += imm8s;
                        go_to_next_instruction(cpu);
                    }
                    return 4;
                }

                case 0xF8:{ // LD HL, SP+e
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        sum_and_set_flags(cpu, cpu->SPL, immr8 + cpu->SPL, true, true);
                    }
                    else if(cpu->machine_cycle == 3){
                        i8 imm8s = (i8)immr8;
                        cpu->HL = cpu->SP + imm8s;
                        go_to_next_instruction(cpu);
                    }
                    return 4;
                }

                case 0xF9:{ // LD SP, HL
                    if(cpu->machine_cycle == 1){
                        cpu->SP = cpu->HL;
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }

                    return 4;
                }

                case 0xF3:{ // DI
                    cpu->IME = false;
                    cpu->scheduled_ei = false;
                    go_to_next_instruction(cpu);
                    return 4;
                }

                case 0xFB:{ // EI
                    cpu->scheduled_ei = true;
                    go_to_next_instruction(cpu);
                    return 4;
                }

            }
            

            break;
        }

        default:{
            printf("Opcode %X not implemented\n", cpu->opcode);
            return 4;
        }
    }
    }
    else{ // Opcodes prefixed with 0xCB
        switch(cpu->opcode & 0xC0){
            case 0x00:{
                switch(cpu->opcode & 0x38){
                    case 0x00:{ // RLC r
                        if(cpu->machine_cycle == 1){
                            u8 reg = cpu->opcode & 0x07;
                            if(reg != 6){
                                u8 previous_bit_7 = (*(cpu->register_map[reg]) & 0x80) >> 7;
                                previous_bit_7 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);
                                *(cpu->register_map[reg]) <<= 1;
                                *(cpu->register_map[reg]) = (*(cpu->register_map[reg]) & (~(0x01))) | previous_bit_7;

                                if(*(cpu->register_map[reg]) == 0)
                                    set_flag(cpu, FLAG_ZERO);
                                else
                                    unset_flag(cpu, FLAG_ZERO);    
                                unset_flag(cpu, FLAG_SUB);
                                unset_flag(cpu, FLAG_HALFCARRY);

                                go_to_next_instruction(cpu);
                            }
                            else{
                                mem_value = read_mem(cpu, cpu->HL);
                            }
                        
                        }
                        else if(cpu->machine_cycle == 2){
                            u8 previous_bit_7 = (mem_value & 0x80) >> 7;
                            previous_bit_7 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);
                            mem_value <<= 1;
                            mem_value = (mem_value & (~(0x01))) | previous_bit_7;

                            if(mem_value == 0)
                                set_flag(cpu, FLAG_ZERO);
                            else
                                unset_flag(cpu, FLAG_ZERO);    

                            unset_flag(cpu, FLAG_SUB);
                            unset_flag(cpu, FLAG_HALFCARRY); 

                            write_mem(cpu, cpu->HL, mem_value);
                        }
                        else if(cpu->machine_cycle == 3){
                            go_to_next_instruction(cpu);
                        }

                        return 4;
                    }

                    case 0x08:{ // RRC r
                        if(cpu->machine_cycle == 1){
                            u8 reg = cpu->opcode & 0x07;
                            if(reg != 6){
                                u8 previous_bit_0 = (*(cpu->register_map[reg]) & 0x01);
                                previous_bit_0 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);
                                *(cpu->register_map[reg]) >>= 1;
                                *(cpu->register_map[reg]) = (*(cpu->register_map[reg]) & (~(0x80))) | (previous_bit_0 << 7);

                                if(*(cpu->register_map[reg]) == 0)
                                    set_flag(cpu, FLAG_ZERO);
                                else
                                    unset_flag(cpu, FLAG_ZERO); 

                                unset_flag(cpu, FLAG_SUB);
                                unset_flag(cpu, FLAG_HALFCARRY);

                                go_to_next_instruction(cpu);
                            }
                            else{
                                mem_value = read_mem(cpu, cpu->HL);
                            }
                        
                        }
                        else if(cpu->machine_cycle == 2){
                            u8 previous_bit_0 = (mem_value & 0x01);
                            previous_bit_0 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);
                            mem_value >>= 1;
                            mem_value = (mem_value & (~(0x80))) | (previous_bit_0 << 7);

                            if(mem_value == 0)
                                set_flag(cpu, FLAG_ZERO);
                            else
                                unset_flag(cpu, FLAG_ZERO); 

                            unset_flag(cpu, FLAG_SUB);
                            unset_flag(cpu, FLAG_HALFCARRY);

                            write_mem(cpu, cpu->HL, mem_value);
                        }
                        else if(cpu->machine_cycle == 3){
                            go_to_next_instruction(cpu);
                        }

                        return 4;
                    }

                    case 0x10:{ // RL r
                        if(cpu->machine_cycle == 1){
                            u8 reg = cpu->opcode & 0x07;
                            if(reg != 6){
                                u8 previous_bit_7 = (*(cpu->register_map[reg]) & 0x80) >> 7;
                                
                                *(cpu->register_map[reg]) <<= 1;
                                *(cpu->register_map[reg]) = (*(cpu->register_map[reg]) & (~(0x01))) | ((cpu->flags & FLAG_CARRY) >> 4);

                                previous_bit_7 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);

                                if(*(cpu->register_map[reg]) == 0)
                                    set_flag(cpu, FLAG_ZERO);
                                else
                                    unset_flag(cpu, FLAG_ZERO); 

                                unset_flag(cpu, FLAG_SUB);
                                unset_flag(cpu, FLAG_HALFCARRY);

                                go_to_next_instruction(cpu);
                            }
                            else{
                                mem_value = read_mem(cpu, cpu->HL);
                            }
                        
                        }
                        else if(cpu->machine_cycle == 2){
                            u8 previous_bit_7 = (mem_value & 0x80) >> 7;
                                
                            mem_value <<= 1;
                            mem_value = (mem_value & (~(0x01))) | ((cpu->flags & FLAG_CARRY) >> 4);

                            previous_bit_7 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);

                            if(mem_value == 0)
                                set_flag(cpu, FLAG_ZERO);
                            else
                                unset_flag(cpu, FLAG_ZERO); 

                            unset_flag(cpu, FLAG_SUB);
                            unset_flag(cpu, FLAG_HALFCARRY);

                            write_mem(cpu, cpu->HL, mem_value);
                        }
                        else if(cpu->machine_cycle == 3){
                            go_to_next_instruction(cpu);
                        }

                        return 4;
                    }

                    case 0x18:{ // RR r
                        if(cpu->machine_cycle == 1){
                            u8 reg = cpu->opcode & 0x07;
                            if(reg != 6){
                                u8 previous_bit_0 = (*(cpu->register_map[reg]) & 0x01);
                    
                                *(cpu->register_map[reg]) >>= 1;
                                *(cpu->register_map[reg]) = (*(cpu->register_map[reg]) & (~(0x80))) | ((cpu->flags & FLAG_CARRY) << 3);

                                previous_bit_0 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);

                                if(*(cpu->register_map[reg]) == 0)
                                    set_flag(cpu, FLAG_ZERO);
                                else
                                    unset_flag(cpu, FLAG_ZERO); 

                                unset_flag(cpu, FLAG_SUB);
                                unset_flag(cpu, FLAG_HALFCARRY);

                                go_to_next_instruction(cpu);
                            }
                            else{
                                mem_value = read_mem(cpu, cpu->HL);
                            }
                        
                        }
                        else if(cpu->machine_cycle == 2){
                            u8 previous_bit_0 = (mem_value & 0x01);
                    
                            mem_value >>= 1;
                            mem_value = (mem_value & (~(0x80))) | ((cpu->flags & FLAG_CARRY) << 3);

                            previous_bit_0 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);

                            if(mem_value == 0)
                                set_flag(cpu, FLAG_ZERO);
                            else
                                unset_flag(cpu, FLAG_ZERO); 
                                
                            unset_flag(cpu, FLAG_SUB);
                            unset_flag(cpu, FLAG_HALFCARRY);

                            write_mem(cpu, cpu->HL, mem_value);
                        }
                        else if(cpu->machine_cycle == 3){
                            go_to_next_instruction(cpu);
                        }

                        return 4;
                    }

                    case 0x20:{ // SLA r
                        if(cpu->machine_cycle == 1){
                            u8 reg = cpu->opcode & 0x07;
                            if(reg != 6){
                                u8 previous_bit_7 = (*(cpu->register_map[reg]) & 0x80) >> 7;
                                previous_bit_7 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);
                                *(cpu->register_map[reg]) <<= 1;

                                go_to_next_instruction(cpu);
                            }
                            else{
                                mem_value = read_mem(cpu, cpu->HL);
                            }
                        
                        }
                        else if(cpu->machine_cycle == 2){
                            u8 previous_bit_7 = (mem_value & 0x80) >> 7;
                            previous_bit_7 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);
                            mem_value <<= 1;

                            write_mem(cpu, cpu->HL, mem_value);
                        }
                        else if(cpu->machine_cycle == 3){
                            go_to_next_instruction(cpu);
                        }

                        return 4;
                    }
                }
                
            }

            default:{
                printf("Prefixed Opcode %X not implemented\n", cpu->opcode);
                return 4;
            } 
        }
    }
}