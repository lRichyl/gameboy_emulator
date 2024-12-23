#include "CPU.h"
#include "ppu.h"
#include <stdio.h>

void init_cpu(CPU *cpu, Memory *memory){
    cpu->memory = memory;

    cpu->clock_speed = 4194304; // Hz
    cpu->frame_time = 1000.f/59.7f;
    cpu->period = 1000.f/(cpu->clock_speed);
    cpu->machine_cycles_per_frame = (u32)(cpu->frame_time / cpu->period);
    cpu->cycles_delta = 0;

    cpu->memory->data[0xFF40] = 0x91;
    

    cpu->do_first_fetch = true;
    cpu->IME = false;
    cpu->DMA_transfer_in_progress = false;

    cpu->memory->data[0xFF0F] = 0x0E; // IF register. Interrupt flags.
    cpu->memory->data[0xFFFF] = 0x09; // IE register. Interrupt enable flags.
    cpu->memory->data[0xFF04] = 0xAB; // DIV register.
    cpu->memory->data[0xFF47] = 0xE4; // Palette BGP register.
    cpu->memory->data[0xFF48] = 0xE4; // Palette OBJ0 register.
    cpu->memory->data[0xFF49] = 0xE4; // Palette OBJ1 register.

    cpu->memory->data[0xFF41] = 0x80; // STAT register.

    cpu->scheduled_ei = false;
    cpu->is_extended = false;
    cpu->handling_interrupt = false;
    cpu->fetched_next_instruction = false;

    cpu->PC = 0x0100; // Temporary
    cpu->internal_counter = 0xABCC;

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
    cpu->register_map[6] = &cpu->flags;
    cpu->register_map[7] = &cpu->A;

    cpu->AF = 0x01B0;
    cpu->BC = 0x0013;
    cpu->DE = 0x00D8;
    cpu->HL = 0x014D;
    cpu->SP = 0xFFFE;

    /*fopen_s(&cpu->fp, "Log" , "w" );
	if( !cpu->fp ){
		printf("File could not be opened\n" );
        assert(false)
    }*/
}

static u8 read_memory_cpu(CPU *cpu, u16 address){
    if(address >= 0x8000 && address <= 0x9FFF && cpu->memory->is_vram_locked){ // VRAM
            return 0xFF;
    }
    else if(address >= 0xFE00 && address <= 0xFE9F && cpu->memory->is_oam_locked){ // OAM
        return 0xFF;
    }
    else if(address == 0xFF00){
        return cpu->memory->data[address];
    }
    else if(address >= 0xFE00 && address <= 0xFE9F && cpu->memory->is_oam_locked){ // OAM
        return 0xFF;
    }
    
    return cpu->memory->data[address];
}

static void write_memory_cpu(CPU *cpu, u16 address, u8 value){
    assert(address < MEMORY_SIZE);

    if(address >= 0x8000 && address <= 0x9FFF && cpu->memory->is_vram_locked){ // VRAM
        return;
    }
    else if(address >= 0x0000 && address <= 0x7FFF){ // ROM
        return;
    }
    else if(address == 0xFF04){
        cpu->memory->data[address] = 0x00;
        return;
    }
    else if(address == 0xFF46){ // Initiate DMA transfer. The value written is the upper byte of the address to copy from.
        cpu->memory->data[address] = value;
        cpu->DMA_transfer_in_progress = true;
        cpu->DMA_source = value << 8;
        return;
    }

    cpu->memory->data[address] = value;
}

void update_joypad(CPU *cpu, const bool *input){
    if(!(read_memory_cpu(cpu, 0xFF00) & 0x10)){ // Dpad selected.
        u8 in = read_memory_cpu(cpu, 0xFF00); // All buttons released.
        in |= 0x0F;
        if(input[SDL_SCANCODE_DOWN])  in &= ~0x08;
        if(input[SDL_SCANCODE_UP])    in &= ~0x04;
        if(input[SDL_SCANCODE_LEFT])  in &= ~0x02;
        if(input[SDL_SCANCODE_RIGHT]) in &= ~0x01; 

        write_memory_cpu(cpu, 0xFF00, in);
    }
    else if(!(read_memory_cpu(cpu, 0xFF00) & 0x20)){ // Buttons selected.
        u8 in = read_memory_cpu(cpu, 0xFF00); // All buttons released.
        in |= 0x0F;
        if(input[SDL_SCANCODE_RETURN]) in &= ~0x08;
        if(input[SDL_SCANCODE_RSHIFT]) in &= ~0x04;
        if(input[SDL_SCANCODE_X])      in &= ~0x02;
        if(input[SDL_SCANCODE_Z])      in &= ~0x01; 

        write_memory_cpu(cpu, 0xFF00, in);
    }
}

u8 fetch(CPU *cpu){
    u8 byte = read_memory_cpu(cpu, cpu->PC);
    cpu->PC++;
    return byte;
}


void set_flag(CPU *cpu, Flag flag){
    cpu->flags = cpu->flags | flag;
}

void unset_flag(CPU *cpu, Flag flag){
    cpu->flags = cpu->flags & ~(flag);
}

static u8 sum_and_set_flags(CPU *cpu, u8 summand_left, u8 summand_right, bool add_carry = false, b32 check_carry = false, bool check_zero = false){
    u8 carry;
    add_carry ? carry = ((cpu->flags & FLAG_CARRY) >> 4) : carry = 0;

    u16 result = (u16)summand_left + (u16)summand_right + carry;
    
    if(check_zero){
        if(u8(result) == 0)     
            set_flag(cpu, FLAG_ZERO);
        else
            unset_flag(cpu, FLAG_ZERO);
    }

    unset_flag(cpu, FLAG_SUB);

    u8 nibble_left_summand     = summand_left    & 0x0F;
    u8 nibble_right_summand    = summand_right   & 0x0F;
    if((nibble_left_summand + nibble_right_summand + carry) > 0x0F) 
        set_flag(cpu, FLAG_HALFCARRY);
    else
        unset_flag(cpu, FLAG_HALFCARRY);

    if(check_carry){
        if(result > 0xFF) 
            set_flag(cpu, FLAG_CARRY);
        else
            unset_flag(cpu, FLAG_CARRY);
    }

    return (u8)result;
}

u8 sum_and_set_flags_adc(CPU *cpu, u8 summand_left, u8 summand_right){
    u8 carry = (cpu->flags & FLAG_CARRY) >> 4;
    u16 result = (u16)summand_left + (u16)summand_right + carry;
                            
    if(u8(result) == 0)     
        set_flag(cpu, FLAG_ZERO);
    else
        unset_flag(cpu, FLAG_ZERO);

    unset_flag(cpu, FLAG_SUB);

    u8 nibble_left_summand     = cpu->A          & 0x0F;
    u8 nibble_right_summand    = summand_right   & 0x0F;
    if((nibble_left_summand + nibble_right_summand + carry) > 0x0F) 
        set_flag(cpu, FLAG_HALFCARRY);
    else
        unset_flag(cpu, FLAG_HALFCARRY);

    if(result > 0xFF) 
        set_flag(cpu, FLAG_CARRY);
    else
        unset_flag(cpu, FLAG_CARRY);

    return (u8)result;
}

static u8 substract_and_set_flags(CPU *cpu, u8 minuend, u8 sustrahend, b32 check_carry = false, bool check_zero = false){
    u8 result = minuend - sustrahend;

    if(check_zero){
        if(result == 0)     
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
        if(minuend < sustrahend) 
            set_flag(cpu, FLAG_CARRY);
        else
            unset_flag(cpu, FLAG_CARRY);
    }

    return result;
}

static u8 substract_and_set_flags_sbc(CPU *cpu, u8 minuend, u8 sustrahend){
    u8 carry = (cpu->flags & FLAG_CARRY) >> 4;
    u8 result = minuend - (sustrahend + carry);

    if(result == 0)     
        set_flag(cpu, FLAG_ZERO);
    else
        unset_flag(cpu, FLAG_ZERO);

    set_flag(cpu, FLAG_SUB);
  
    u8 nibble_minuend    = (minuend    & 0x0F);
    u8 nibble_sustrahend = (sustrahend & 0x0F);

    if(nibble_sustrahend > nibble_minuend - carry) 
        set_flag(cpu, FLAG_HALFCARRY);
    else
        unset_flag(cpu, FLAG_HALFCARRY);

    if(minuend < sustrahend + carry) 
        set_flag(cpu, FLAG_CARRY);
    else
        unset_flag(cpu, FLAG_CARRY);

    return result;
}

u8 pop_stack(CPU *cpu){
    assert(cpu->SP > 0);
    u8 value = read_memory_cpu(cpu, cpu->SP);
    cpu->SP++;
    return value;
}

u8 push_stack(CPU *cpu, u8 value){
    assert(cpu->SP > 0);
    cpu->SP--;
    write_memory_cpu(cpu, cpu->SP, value);
    return value;
}


static void print_cpu(CPU *cpu){
    bool zero   = cpu->flags & FLAG_ZERO;
    bool half   = cpu->flags & FLAG_HALFCARRY;
    bool sub    = cpu->flags & FLAG_SUB;
    bool carry  = cpu->flags & FLAG_CARRY;
    u8 DIV      = read_memory_cpu(cpu, 0xFF04);
    u8 LCDC     = read_memory_cpu(cpu, 0xFF40);
    u8 LY       = read_memory_cpu(cpu, 0xFF44);
    u8 LYC      = read_memory_cpu(cpu, 0xFF45);
    u8 STAT     = read_memory_cpu(cpu, 0xFF41);

    fprintf(cpu->fp,"PC: \t%X\n", cpu->PC);
    fprintf(cpu->fp,"IME: \t%d\n", cpu->IME);
    fprintf(cpu->fp,"Opcode: \t%X\n", cpu->opcode);
    fprintf(cpu->fp,"Cycles: \t%d\n", cpu->cycles_delta);
    fprintf(cpu->fp,"DIV: \t%X  Internal counter: %X\n", DIV, cpu->internal_counter);
    fprintf(cpu->fp,"LCDC: \t%X\n", LCDC);
    fprintf(cpu->fp,"LY: \t%X\n", LY);
    fprintf(cpu->fp,"LYC: \t%X\n", LYC);
    fprintf(cpu->fp,"STAT: \t%X\n", STAT);
    fprintf(cpu->fp,"A: %X\tBC: %X\tDE: %X\tHL: %X\tSP: %X\tZ%X\tH%X\tN%X\tC%X\t\n\n", cpu->A, cpu->BC, cpu->DE, cpu->HL, cpu->SP, zero, half, sub, carry);
}


static void go_to_next_instruction(CPU *cpu){
    //print_cpu(cpu);
    if(cpu->opcode != 0xFB && cpu->scheduled_ei){
        cpu->IME = true;
        cpu->scheduled_ei = false;
    }

    //assert(cpu->PC != 0x904);
    cpu->opcode = fetch(cpu);
    cpu->machine_cycle = 0; 
    cpu->fetched_next_instruction = true;
    
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


void run_cpu(CPU *cpu){
    if(cpu->fetched_next_instruction) cpu->fetched_next_instruction = false;
    if(cpu->do_first_fetch){
        cpu->machine_cycle = 0;
        cpu->opcode = fetch(cpu);
        cpu->do_first_fetch = false;
        // return 4;
    }
    cpu->machine_cycle++;
    if(!cpu->is_extended){
        switch(cpu->opcode & 0xC0){
        case 0x00:{
            switch(cpu->opcode){
                case 0x00:{  // NOP
                    go_to_next_instruction(cpu);

                    break;
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

                    break;
                }

                case 0x02:
                case 0x12:{ // LD [r16], A.    Only BC and DE.
                    
                    if(cpu->machine_cycle == 1){
                        u8 reg = cpu->opcode & 0x30;
                        reg >>= 4;
                        assert(reg <= 1);
                        write_memory_cpu(cpu, *cpu->wide_register_map[reg], cpu->A);
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }

                    break;
                }


                
                case 0x22:{ // LD [HL+], A
                    
                    if(cpu->machine_cycle == 1){
                        write_memory_cpu(cpu, cpu->HL, cpu->A);
                        cpu->HL++;
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }
                    break;
                }

                case 0x32:{ // LD [HL-], A
                    
                    if(cpu->machine_cycle == 1){
                        write_memory_cpu(cpu, cpu->HL, cpu->A);
                        cpu->HL--;
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }
                    break;
                }

                case 0x0A:
                case 0x1A:{ // LD A, [r16]   only for BC and DE
                    
                    if(cpu->machine_cycle == 1){
                        u8 reg = cpu->opcode & 0x30;
                        reg >>= 4;
                        assert(reg <= 1);
                        mem_value = read_memory_cpu(cpu, *cpu->wide_register_map[reg]);
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = mem_value;

                        go_to_next_instruction(cpu);
                    }

                    break;
                }

                case 0x2A:{ // LD A, [HL+]
                    
                    if(cpu->machine_cycle == 1){
                        mem_value = read_memory_cpu(cpu, cpu->HL);
                        cpu->HL++;
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = mem_value;

                        go_to_next_instruction(cpu);
                    }
                    break;
                }

                case 0x3A:{  // LD A, [HL+]
                    
                    if(cpu->machine_cycle == 1){
                        mem_value = read_memory_cpu(cpu, cpu->HL);
                        cpu->HL--;
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = mem_value;

                        go_to_next_instruction(cpu);
                    }
                    break;
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
                    write_memory_cpu(cpu, imm, cpu->SP & 0x00FF);
                    imm++;
                    }
                    else if(cpu->machine_cycle == 4){
                    write_memory_cpu(cpu, imm, (cpu->SP & 0xFF00) >> 8);
                    }
                    else if(cpu->machine_cycle == 5){
                        go_to_next_instruction(cpu);
                    }

                    break;
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
                    break;
                }

                case 0x33:{ // INC SP
                    
                    if(cpu->machine_cycle == 1){
                        cpu->SP++;
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }
                    break;
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
                    break;
                }

                case 0x3B:{ // DEC SP
                    
                    if(cpu->machine_cycle == 1){
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }
                    break;
                }

                case 0x09:
                case 0x19:
                case 0x29:{ // ADD HL, r16  for BC, DE and HL.
                    
                    if(cpu->machine_cycle == 1){
                        u8 reg = cpu->opcode & 0x30;
                        reg >>= 4;
                        assert(reg <= 2);

                        u8 reg_low = ((*cpu->wide_register_map[reg]) & 0x00FF);
                        cpu->L = sum_and_set_flags(cpu, cpu->L, reg_low, false, true, false);
                    }
                    else if(cpu->machine_cycle == 2){
                        u8 reg = cpu->opcode & 0x30;
                        reg >>= 4;
                        u8 reg_high = ((*cpu->wide_register_map[reg]) & 0xFF00) >> 8;
                        u8 carry = (cpu->flags & FLAG_CARRY) >> 4;
                        cpu->H =  sum_and_set_flags(cpu, cpu->H, reg_high, true, true, false);

                        go_to_next_instruction(cpu);
                    }
                    break;
                }

                case 0x39:{ // ADD HL, SP
                    
                    if(cpu->machine_cycle == 1){
                        u8 reg_low = ((cpu->SP) & 0x00FF);
                        cpu->L = sum_and_set_flags(cpu, cpu->L, reg_low, false, true, false);
                    }
                    else if(cpu->machine_cycle == 2){
                        u8 reg_high = ((cpu->SP) & 0xFF00) >> 8;
                        u8 carry = (cpu->flags & FLAG_CARRY) >> 4;
                        cpu->H =  sum_and_set_flags(cpu, cpu->H, reg_high, true, true, false);

                        go_to_next_instruction(cpu);
                    }
                    break;
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
                        (*cpu->register_map[reg]) = sum_and_set_flags(cpu, (*cpu->register_map[reg]), 1, false, false, true);
                    else if(reg == 7){
                        cpu->A = sum_and_set_flags(cpu, cpu->A, 1, false, false, true);
                    }

                    go_to_next_instruction(cpu);

                    break;
                }
                
                case 0x34:{ // INC [HL]
                    
                    if(cpu->machine_cycle == 1){
                        mem_value = read_memory_cpu(cpu, cpu->HL);
                    }
                    else if(cpu->machine_cycle == 2){
                        mem_value = sum_and_set_flags(cpu, mem_value, 1, false, false, true);
                        write_memory_cpu(cpu, cpu->HL, mem_value);
                    }
                    else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
                    }

                    break;
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

                    break;
                }

                case 0x35:{ // DEC [HL]
                    
                    if(cpu->machine_cycle == 1){
                        mem_value = read_memory_cpu(cpu, cpu->HL);
                    }
                    else if(cpu->machine_cycle == 2){
                        mem_value = substract_and_set_flags(cpu, mem_value, 1, false, true);
                        write_memory_cpu(cpu, cpu->HL, mem_value);
                    }
                    else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
                    }

                    break;
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

                    break;
                }

                case 0x36:{ // LD [HL], imm8
                    
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);    
                    }
                    else if(cpu->machine_cycle == 2){
                        write_memory_cpu(cpu, cpu->HL, immr8);

                    }else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
                    }

                    break;
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
                    break;
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

                    break;
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

                    break;
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

                    break;
                }

                case 0x27:{ // DAA
                    if (!(cpu->flags & FLAG_SUB)) {  // after an addition, adjust if (half-)carry occurred or if result is out of bounds
                        if (cpu->flags & FLAG_CARRY || cpu->A > 0x99) { 
                            cpu->A += 0x60;
                            set_flag(cpu, FLAG_CARRY);
                        }
                        if((cpu->A & 0x0F) > 9 || cpu->flags & FLAG_HALFCARRY){
                            cpu->A += 6;
                        }
                    } else {  // after a subtraction, only adjust if (half-)carry occurred
                        if (cpu->flags & FLAG_CARRY){
                            cpu->A -= 0x60; 
                        }
                        if (cpu->flags & FLAG_HALFCARRY){ 
                            cpu->A -= 0x6; 
                        }
                    }

                    unset_flag(cpu, FLAG_HALFCARRY);

                    if(cpu->A == 0)     
                        set_flag(cpu, FLAG_ZERO);
                    else
                        unset_flag(cpu, FLAG_ZERO);

                    go_to_next_instruction(cpu);

                    break;
                }

                case 0x2F:{ // CPL
                    cpu->A = ~cpu->A;

                    set_flag(cpu, FLAG_HALFCARRY);
                    set_flag(cpu, FLAG_SUB);

                    go_to_next_instruction(cpu);

                    break;
                }

                case 0x37:{ // SCF
                    set_flag(cpu, FLAG_CARRY);

                    unset_flag(cpu, FLAG_HALFCARRY);
                    unset_flag(cpu, FLAG_SUB);

                    go_to_next_instruction(cpu);
                    break;
                }

                case 0x3F:{ // CCF
                    (cpu->flags & FLAG_CARRY) ? unset_flag(cpu, FLAG_CARRY) : set_flag(cpu, FLAG_CARRY);

                    unset_flag(cpu, FLAG_HALFCARRY);
                    unset_flag(cpu, FLAG_SUB);

                    go_to_next_instruction(cpu);

                    break;
                }

                case 0x18:{ // JR imm8
                    
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);    
                    }
                    else if(cpu->machine_cycle == 2){
                        // bool sign = immr8 & 0x80;
                        // immr8  = sum_and_set_flags(cpu, immr8, cpu->PCL, true, true);
                        // i8 adj = 0;
                        // if((cpu->flags & FLAG_CARRY) && !sign){
                        //     adj = 1;
                        // }
                        // else if(!(cpu->flags & FLAG_CARRY) && sign){
                        //     adj = -1;
                        // }

                        // mem_value = cpu->PCH + adj;


                        i8 imm8s = (i8)immr8;
                        u16 target = (mem_value << 8) | immr8;
                        cpu->PC += imm8s;

                    }else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
                    }

                    break;
                }

                case 0x20:{ // JR NZ, imm8
                    
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);    
                    }
                    else if(cpu->machine_cycle == 2){
                        if(!(cpu->flags & FLAG_ZERO)){
                            i8 imm8s = (i8)immr8;
                            u16 target = cpu->PC + imm8s;
                            cpu->PC = target;
                        }
                        else{
                            go_to_next_instruction(cpu);    
                        }

                    }else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
                    }

                    break;
                }

                case 0x30:{ // JR NC, imm8
                    
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);    
                    }
                    else if(cpu->machine_cycle == 2){
                        if(!(cpu->flags & FLAG_CARRY)){
                            i8 imm8s = (i8)immr8;
                            u16 target = cpu->PC + imm8s;
                            cpu->PC = target;
                        }
                        else{
                            go_to_next_instruction(cpu);    
                        }

                    }else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
                    }

                    break;
                }

                case 0x28:{ // JR Z, imm8
                    
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);    
                    }
                    else if(cpu->machine_cycle == 2){
                        if((cpu->flags & FLAG_ZERO)){
                            i8 imm8s = (i8)immr8;
                            u16 target = cpu->PC  + imm8s;
                            cpu->PC = target;
                        }
                        else{
                            go_to_next_instruction(cpu);    
                        }

                    }else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
                    }

                    break;
                }

                case 0x38:{ // JR C, imm8
                    
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);    
                    }
                    else if(cpu->machine_cycle == 2){
                        if((cpu->flags & FLAG_CARRY)){
                            i8 imm8s = (i8)immr8;
                            u16 target = cpu->PC + imm8s;
                            cpu->PC = target;
                        }
                        else{
                            go_to_next_instruction(cpu);    
                        }

                    }else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
                    }

                    break;
                }

                // TODO
                 case 0x10:{ // STOP
                     
                     
                     break;
                 }

                default:{
                    //printf("Opcode %X not implemented\n", cpu->opcode);
                    break;
                }

                
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
                    cpu->halt = true;
                    go_to_next_instruction(cpu);
                }
                else if(dest == 6 && src != 6){ // LD [HL], r8
                    write_memory_cpu(cpu, cpu->HL, *cpu->register_map[src]);
                }
                else if(dest != 6 && src == 6){ // LD r8, [HL]
                    mem_value = read_memory_cpu(cpu, cpu->HL);
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
            break;
        }

        case 0x80:{
            switch(cpu->opcode & 0x38){
                case 0x00:{ // ADD A, r
                    if(cpu->machine_cycle == 1){
                        u8 reg  = (cpu->opcode & 0x07);
                        assert(reg <= 7 && reg >= 0);

                        if(reg != 6){
                            cpu->A = sum_and_set_flags(cpu, cpu->A, *cpu->register_map[reg], false, true, true);
                            go_to_next_instruction(cpu);
                        }
                        else if (reg == 6){ // ADD A, [HL]
                            mem_value = read_memory_cpu(cpu, cpu->HL);
                        }

                    }
                    else if(cpu->machine_cycle == 2){
                        // ADD A, [HL]
                        cpu->A = sum_and_set_flags(cpu, cpu->A, mem_value, false, true, true);
                        
                        go_to_next_instruction(cpu);
                    }
                    break;
                }

                case 0x08:{ // ADC A, r
                    if(cpu->machine_cycle == 1){
                        u8 reg  = (cpu->opcode & 0x07);
                        assert(reg <= 7 && reg >= 0);

                        if(reg != 6){
                            cpu->A = sum_and_set_flags_adc(cpu, cpu->A, *cpu->register_map[reg]);

                            go_to_next_instruction(cpu);
                        }
                        else if (reg == 6){ // ADC A, [HL]
                            mem_value = read_memory_cpu(cpu, cpu->HL);
                        }

                    }
                    else if(cpu->machine_cycle == 2){
                        // ADC A, [HL]
                        cpu->A = sum_and_set_flags_adc(cpu, cpu->A, mem_value);
                        
                        go_to_next_instruction(cpu);
                    }

                    break;
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
                            mem_value = read_memory_cpu(cpu, cpu->HL);
                        }

                    }
                    else if(cpu->machine_cycle == 2){
                        // SUB A, [HL]
                        cpu->A = substract_and_set_flags(cpu, cpu->A, mem_value, true, true);
                        
                        go_to_next_instruction(cpu);
                    }
                    break;
                }

                case 0x18:{ // SBC A, r
                    if(cpu->machine_cycle == 1){
                        u8 reg  = (cpu->opcode & 0x07);
                        assert(reg <= 7 && reg >= 0);

                        if(reg != 6){
                            cpu->A = substract_and_set_flags_sbc(cpu, cpu->A, *cpu->register_map[reg]);
                            go_to_next_instruction(cpu);
                        }
                        else if (reg == 6){ // SBC A, [HL]
                            mem_value = read_memory_cpu(cpu, cpu->HL);
                        }

                    }
                    else if(cpu->machine_cycle == 2){
                        // SBC A, [HL]
                        cpu->A = substract_and_set_flags_sbc(cpu, cpu->A, mem_value);
                        
                        go_to_next_instruction(cpu);
                    }

                    break;
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
                            mem_value = read_memory_cpu(cpu, cpu->HL);
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

                    break;
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
                            mem_value = read_memory_cpu(cpu, cpu->HL);
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

                    break;
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
                            mem_value = read_memory_cpu(cpu, cpu->HL);
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

                    break;
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
                            mem_value = read_memory_cpu(cpu, cpu->HL);
                        }

                    }
                    else if(cpu->machine_cycle == 2){
                        // CP A, [HL]
                        substract_and_set_flags(cpu, cpu->A, mem_value, true, true);
                        
                        go_to_next_instruction(cpu);
                    }

                    break;
                }

                default:{
                    printf("Opcode %X not implemented\n", cpu->opcode);
                    assert(false);
                    break;
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
                        cpu->A = sum_and_set_flags(cpu, cpu->A, immr8, false, true, true);     
                        go_to_next_instruction(cpu);
                    }

                    break;
                }

                case 0xCE:{ // ADC A, imm8
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = sum_and_set_flags_adc(cpu, cpu->A, immr8);   

                        go_to_next_instruction(cpu);
                    }

                    break;
                }

                case 0xD6:{ // SUB A, imm8
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = substract_and_set_flags(cpu, cpu->A, immr8, true, true);     
                        go_to_next_instruction(cpu);
                    }

                    break;
                }

                case 0xDE:{ // SBC A, imm8
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = substract_and_set_flags_sbc(cpu, cpu->A, immr8);     
                        go_to_next_instruction(cpu);
                    }

                    break;
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

                    break;
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

                    break;
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

                    break;
                }

                case 0xFE:{ // CP A, imm8
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        substract_and_set_flags(cpu, cpu->A, immr8, true, true);  

                        go_to_next_instruction(cpu);
                    }

                    break;
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
                        cpu->is_extended = cpu->was_extended;
                    }

                    break;
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
                        cpu->is_extended = cpu->was_extended;
                    }

                    break;
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
                        cpu->is_extended = cpu->was_extended;
                    }

                    break;
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
                        cpu->is_extended = cpu->was_extended;
                    }

                    break;
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

                    break;
                }

                case 0xD9:{ // RETI
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
                        cpu->is_extended = cpu->was_extended;
                    }

                    break;
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
                    break;
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

                    break;
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
                    break;
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

                    break;
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

                    break;
                }

                case 0xE9:{ // JP HL
                    cpu->PC = cpu->HL;
                    go_to_next_instruction(cpu);

                    break;
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
                        write_memory_cpu(cpu, cpu->SP, cpu->PCH);
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 5){
                        write_memory_cpu(cpu, cpu->SP, cpu->PCL);
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 6){
                        go_to_next_instruction(cpu);
                    }

                    break;
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
                        write_memory_cpu(cpu, cpu->SP, cpu->PCH);
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 5){
                        write_memory_cpu(cpu, cpu->SP, cpu->PCL);
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 6){
                        go_to_next_instruction(cpu);
                    }

                    break;
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
                        write_memory_cpu(cpu, cpu->SP, cpu->PCH);
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 5){
                        write_memory_cpu(cpu, cpu->SP, cpu->PCL);
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 6){
                        go_to_next_instruction(cpu);
                    }

                    break;
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
                        write_memory_cpu(cpu, cpu->SP, cpu->PCH);
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 5){
                        write_memory_cpu(cpu, cpu->SP, cpu->PCL);
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 6){
                        go_to_next_instruction(cpu);
                    }

                    break;
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
                        write_memory_cpu(cpu, cpu->SP, cpu->PCH);
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 5){
                        write_memory_cpu(cpu, cpu->SP, cpu->PCL);
                        imm = (imm_high << 8) | (imm_low);
                        cpu->PC = imm;
                    }
                    else if(cpu->machine_cycle == 6){
                        go_to_next_instruction(cpu);
                    }

                    break;
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
                        write_memory_cpu(cpu, cpu->SP, cpu->PCH);
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 3){
                        write_memory_cpu(cpu, cpu->SP, cpu->PCL);
                        u8 target = (cpu->opcode & 0x38) >> 3;
                        u16 address = target * 0x08;
                        cpu->PC = address;
                    }
                    else if(cpu->machine_cycle == 4){
                        go_to_next_instruction(cpu);
                    }

                    break;
                }

                case 0xC1:
                case 0xD1:
                case 0xE1:
                case 0xF1:{ // POP r16
                    if(cpu->machine_cycle == 1){
                        imm_low = read_memory_cpu(cpu, cpu->SP);
                        cpu->SP++;
                    }
                    else if(cpu->machine_cycle == 2){
                        assert(cpu->SP > 0);
                        imm_high = read_memory_cpu(cpu, cpu->SP);
                        cpu->SP++;
                    }
                    else if(cpu->machine_cycle == 3){
                        u8 target = (cpu->opcode & 0x30) >> 4;
                        *(cpu->wide_register_map[target]) = (imm_high << 8) | imm_low;
                        if(cpu->opcode == 0xF1){
                            *(cpu->wide_register_map[target]) &= 0xFFF0;
                        }
                        go_to_next_instruction(cpu);
                    }

                    break;
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
                        write_memory_cpu(cpu, cpu->SP, (u8)((*(cpu->wide_register_map[src]) & 0xFF00) >> 8));
                        cpu->SP--;
                    }
                    else if(cpu->machine_cycle == 3){
                        u8 src = (cpu->opcode & 0x30) >> 4;
                        write_memory_cpu(cpu, cpu->SP, (u8)((*(cpu->wide_register_map[src]) & 0x00FF)));
                    }
                    else if(cpu->machine_cycle == 4){
                        go_to_next_instruction(cpu);
                    }

                    break;
                }

                case 0xCB:{
                    cpu->is_extended = true;
                    go_to_next_instruction(cpu);
                    break;
                }

                case 0xE2:{ // LDH [C], A
                    if(cpu->machine_cycle == 1){
                        u16 address = 0xFF00 + cpu->C;
                        write_memory_cpu(cpu, address, cpu->A);
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }

                    break;
                } 

                case 0xE0:{ // LDH [imm8], A
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        u16 address = 0xFF00 + immr8;
                        write_memory_cpu(cpu, address, cpu->A);
                    }
                    else if(cpu->machine_cycle == 3){
                        go_to_next_instruction(cpu);
                    }
                        
                    break;
                }

                case 0xEA:{ // LDH [imm16], A
                    if(cpu->machine_cycle == 1){
                        imm_low = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 3){
                        write_memory_cpu(cpu, imm, cpu->A);
                    }
                    else if(cpu->machine_cycle == 4){
                        go_to_next_instruction(cpu);
                    }
                        
                    break;
                }

                case 0xF2:{ // LDH A, [C]
                    if(cpu->machine_cycle == 1){
                        u16 address = 0xFF00 + cpu->C;
                        mem_value = read_memory_cpu(cpu, address);
                    }
                    else if(cpu->machine_cycle == 2){
                        cpu->A = mem_value;
                        go_to_next_instruction(cpu);
                    }

                    break;
                } 

                case 0xF0:{ // LDH A, [imm8]
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        u16 address = 0xFF00 + immr8;
                        mem_value = read_memory_cpu(cpu, address);
                    }
                    else if(cpu->machine_cycle == 3){
                        cpu->A = mem_value;
                        go_to_next_instruction(cpu);
                    }
                        
                    break;
                }

                case 0xFA:{ // LDH A, [imm16]
                    if(cpu->machine_cycle == 1){
                        imm_low = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        imm_high = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 3){
                        mem_value = read_memory_cpu(cpu, imm);
                    }
                    else if(cpu->machine_cycle == 4){
                        cpu->A = mem_value;
                        go_to_next_instruction(cpu);
                    }
                        
                    break;
                }

                case 0xE8:{ // ADD SP, e
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                        // sign = immr8 & 0x80;
                    }
                    else if(cpu->machine_cycle == 2){
                        sum_and_set_flags(cpu, cpu->SPL, immr8, false, true);
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
                        unset_flag(cpu, FLAG_ZERO);

                        i8 imm8s = (i8)immr8;
                        cpu->SP += imm8s;
                        go_to_next_instruction(cpu);
                    }
                    break;
                }

                case 0xF8:{ // LD HL, SP+e
                    if(cpu->machine_cycle == 1){
                        immr8 = fetch(cpu);
                    }
                    else if(cpu->machine_cycle == 2){
                        sum_and_set_flags(cpu, cpu->SPL, immr8, false, true);
                    }
                    else if(cpu->machine_cycle == 3){
                        unset_flag(cpu, FLAG_ZERO);

                        i8 imm8s = (i8)immr8;
                        cpu->HL = cpu->SP + imm8s;
                        go_to_next_instruction(cpu);
                    }
                    break;
                }

                case 0xF9:{ // LD SP, HL
                    if(cpu->machine_cycle == 1){
                        cpu->SP = cpu->HL;
                    }
                    else if(cpu->machine_cycle == 2){
                        go_to_next_instruction(cpu);
                    }

                    break;
                }

                case 0xF3:{ // DI
                    cpu->IME = false;
                    cpu->scheduled_ei = false;
                    go_to_next_instruction(cpu);
                    break;
                }

                case 0xFB:{ // EI
                    cpu->scheduled_ei = true;
                    go_to_next_instruction(cpu);
                    break;
                }
                
                default:{
                    printf("Opcode %X not implemented\n", cpu->opcode);
                    assert(false);
                    break;
                }

            }
            

            break;
        }

        default:{
            printf("Opcode %X not implemented\n", cpu->opcode);
            assert(false);
            break;
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
                                cpu->is_extended = false;
                            }
                            else{
                                mem_value = read_memory_cpu(cpu, cpu->HL);
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

                            write_memory_cpu(cpu, cpu->HL, mem_value);
                        }
                        else if(cpu->machine_cycle == 3){
                            go_to_next_instruction(cpu);
                            cpu->is_extended = false;
                        }

                        break;
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
                                cpu->is_extended = false;
                            }
                            else{
                                mem_value = read_memory_cpu(cpu, cpu->HL);
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

                            write_memory_cpu(cpu, cpu->HL, mem_value);
                        }
                        else if(cpu->machine_cycle == 3){
                            go_to_next_instruction(cpu);
                            cpu->is_extended = false;
                        }

                        break;
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
                                cpu->is_extended = false;
                            }
                            else{
                                mem_value = read_memory_cpu(cpu, cpu->HL);
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

                            write_memory_cpu(cpu, cpu->HL, mem_value);
                        }
                        else if(cpu->machine_cycle == 3){
                            go_to_next_instruction(cpu);
                            cpu->is_extended = false;
                        }

                        break;
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
                                cpu->is_extended = false;
                            }
                            else{
                                mem_value = read_memory_cpu(cpu, cpu->HL);
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

                            write_memory_cpu(cpu, cpu->HL, mem_value);
                        }
                        else if(cpu->machine_cycle == 3){
                            go_to_next_instruction(cpu);
                            cpu->is_extended = false;
                        }

                        break;
                    }

                    case 0x20:{ // SLA r
                        if(cpu->machine_cycle == 1){
                            u8 reg = cpu->opcode & 0x07;
                            if(reg != 6){
                                u8 previous_bit_7 = (*(cpu->register_map[reg]) & 0x80) >> 7;
                                previous_bit_7 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);
                                *(cpu->register_map[reg]) <<= 1;

                                if(*(cpu->register_map[reg]) == 0)
                                    set_flag(cpu, FLAG_ZERO);
                                else
                                    unset_flag(cpu, FLAG_ZERO); 

                                unset_flag(cpu, FLAG_SUB);
                                unset_flag(cpu, FLAG_HALFCARRY);

                                go_to_next_instruction(cpu);
                                cpu->is_extended = false;
                            }
                            else{
                                mem_value = read_memory_cpu(cpu, cpu->HL);
                            }
                        
                        }
                        else if(cpu->machine_cycle == 2){
                            u8 previous_bit_7 = (mem_value & 0x80) >> 7;
                            previous_bit_7 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);
                            mem_value <<= 1;

                            if(mem_value == 0)
                                set_flag(cpu, FLAG_ZERO);
                            else
                                unset_flag(cpu, FLAG_ZERO); 

                            unset_flag(cpu, FLAG_SUB);
                            unset_flag(cpu, FLAG_HALFCARRY);

                            write_memory_cpu(cpu, cpu->HL, mem_value);
                        }
                        else if(cpu->machine_cycle == 3){
                            go_to_next_instruction(cpu);
                            cpu->is_extended = false;
                        }

                        break;
                    }

                    case 0x28:{ // SRA r
                        if(cpu->machine_cycle == 1){
                            u8 reg = cpu->opcode & 0x07;
                            if(reg != 6){
                                u8 previous_bit_0 = *(cpu->register_map[reg]) & 0x01;
                                previous_bit_0 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);

                                u8 bit_7 = *(cpu->register_map[reg]) & 0x80;
                                *(cpu->register_map[reg]) >>= 1;
                                *(cpu->register_map[reg]) |= bit_7;

                                if(*(cpu->register_map[reg]) == 0)
                                    set_flag(cpu, FLAG_ZERO);
                                else
                                    unset_flag(cpu, FLAG_ZERO); 

                                unset_flag(cpu, FLAG_SUB);
                                unset_flag(cpu, FLAG_HALFCARRY);

                                go_to_next_instruction(cpu);
                                cpu->is_extended = false;
                            }
                            else{
                                mem_value = read_memory_cpu(cpu, cpu->HL);
                            }
                        
                        }
                        else if(cpu->machine_cycle == 2){
                            u8 previous_bit_0 = mem_value & 0x01;
                            previous_bit_0 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);
                            
                            u8 bit_7 = mem_value & 0x80;
                            mem_value >>= 1;
                            mem_value |= bit_7;

                            if(mem_value == 0)
                                set_flag(cpu, FLAG_ZERO);
                            else
                                unset_flag(cpu, FLAG_ZERO); 

                            unset_flag(cpu, FLAG_SUB);
                            unset_flag(cpu, FLAG_HALFCARRY);

                            write_memory_cpu(cpu, cpu->HL, mem_value);
                        }
                        else if(cpu->machine_cycle == 3){
                            go_to_next_instruction(cpu);
                            cpu->is_extended = false;
                        }

                        break;
                    }

                    case 0x30:{ // SWAP r
                        if(cpu->machine_cycle == 1){
                            u8 reg = cpu->opcode & 0x07;
                            if(reg != 6){
                                u8 upper_nibble = *(cpu->register_map[reg]) & 0xF0;
                                u8 lower_nibble = *(cpu->register_map[reg]) & 0x0F;

                                *(cpu->register_map[reg]) = (upper_nibble >> 4) | (lower_nibble << 4);

                                if(*(cpu->register_map[reg]) == 0)
                                    set_flag(cpu, FLAG_ZERO);
                                else
                                    unset_flag(cpu, FLAG_ZERO); 

                                unset_flag(cpu, FLAG_CARRY);
                                unset_flag(cpu, FLAG_SUB);
                                unset_flag(cpu, FLAG_HALFCARRY);

                                go_to_next_instruction(cpu);
                                cpu->is_extended = false;
                            }
                            else{
                                mem_value = read_memory_cpu(cpu, cpu->HL);
                            }
                        
                        }
                        else if(cpu->machine_cycle == 2){
                            u8 upper_nibble = mem_value & 0xF0;
                            u8 lower_nibble = mem_value & 0x0F;

                            mem_value = (upper_nibble >> 4) | (lower_nibble << 4);

                            if(mem_value == 0)
                                set_flag(cpu, FLAG_ZERO);
                            else
                                unset_flag(cpu, FLAG_ZERO); 

                            unset_flag(cpu, FLAG_CARRY);
                            unset_flag(cpu, FLAG_SUB);
                            unset_flag(cpu, FLAG_HALFCARRY);

                            write_memory_cpu(cpu, cpu->HL, mem_value);
                        }
                        else if(cpu->machine_cycle == 3){
                            go_to_next_instruction(cpu);
                            cpu->is_extended = false;
                        }

                        break;
                    }

                    case 0x38:{ // SRL r
                        if(cpu->machine_cycle == 1){
                            u8 reg = cpu->opcode & 0x07;
                            if(reg != 6){
                                u8 previous_bit_0 = *(cpu->register_map[reg]) & 0x01;
                                previous_bit_0 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);

                                *(cpu->register_map[reg]) >>= 1;

                                if(*(cpu->register_map[reg]) == 0)
                                    set_flag(cpu, FLAG_ZERO);
                                else
                                    unset_flag(cpu, FLAG_ZERO); 

                                unset_flag(cpu, FLAG_SUB);
                                unset_flag(cpu, FLAG_HALFCARRY);

                                go_to_next_instruction(cpu);
                                cpu->is_extended = false;
                            }
                            else{
                                mem_value = read_memory_cpu(cpu, cpu->HL);
                            }
                        
                        }
                        else if(cpu->machine_cycle == 2){
                            u8 previous_bit_0 = mem_value & 0x01;
                            previous_bit_0 ? set_flag(cpu, FLAG_CARRY) : unset_flag(cpu, FLAG_CARRY);
                            
                            mem_value >>= 1;

                            if(mem_value == 0)
                                set_flag(cpu, FLAG_ZERO);
                            else
                                unset_flag(cpu, FLAG_ZERO); 

                            unset_flag(cpu, FLAG_SUB);
                            unset_flag(cpu, FLAG_HALFCARRY);

                            write_memory_cpu(cpu, cpu->HL, mem_value);
                        }
                        else if(cpu->machine_cycle == 3){
                            go_to_next_instruction(cpu);
                            cpu->is_extended = false;
                        }

                        break;
                    }
                    default:{
                        printf("Opcode %X not implemented\n", cpu->opcode);
                        assert(false);
                        break;
                    }
                }
                break;
            }

            case 0x40:{ // Bit instructions
                if(cpu->machine_cycle == 1){
                    u8 reg = cpu->opcode & 0x07;
                    u8 bit = (cpu->opcode & 0x38) >> 3;
                    if(reg != 6){
                        (*(cpu->register_map[reg])) & (1 << bit) ? unset_flag(cpu, FLAG_ZERO) : set_flag(cpu, FLAG_ZERO);

                        unset_flag(cpu, FLAG_SUB);
                        set_flag(cpu, FLAG_HALFCARRY);

                        go_to_next_instruction(cpu);
                        cpu->is_extended = false;
                    }
                    else{
                        mem_value = read_memory_cpu(cpu, cpu->HL);
                    }
                    
                }
                else if(cpu->machine_cycle == 2){
                    u8 bit = (cpu->opcode & 0x38) >> 3;
                    mem_value & (1 << bit) ? unset_flag(cpu, FLAG_ZERO) : set_flag(cpu, FLAG_ZERO);

                    unset_flag(cpu, FLAG_SUB);
                    set_flag(cpu, FLAG_HALFCARRY);

                    write_memory_cpu(cpu, cpu->HL, mem_value);
                    go_to_next_instruction(cpu);
                    cpu->is_extended = false;
                }

                break;
            }

            case 0x80:{ // RES instructions
                if(cpu->machine_cycle == 1){
                    u8 reg = cpu->opcode & 0x07;
                    u8 bit = (cpu->opcode & 0x38) >> 3;
                    if(reg != 6){
                        (*(cpu->register_map[reg])) &= ~(1 << bit);

                        go_to_next_instruction(cpu);
                        cpu->is_extended = false;
                    }
                    else{
                        mem_value = read_memory_cpu(cpu, cpu->HL);
                    }
                    
                }
                else if(cpu->machine_cycle == 2){
                    u8 bit = (cpu->opcode & 0x38) >> 3;
                    mem_value &= ~(1 << bit);

                    write_memory_cpu(cpu, cpu->HL, mem_value);
                }
                else if(cpu->machine_cycle == 3){
                    go_to_next_instruction(cpu);
                    cpu->is_extended = false;
                }

                break;
            }

            case 0xC0:{ // SET instructions
                if(cpu->machine_cycle == 1){
                    u8 reg = cpu->opcode & 0x07;
                    u8 bit = (cpu->opcode & 0x38) >> 3;
                    if(reg != 6){
                        (*(cpu->register_map[reg])) |= (1 << bit);

                        go_to_next_instruction(cpu);
                        cpu->is_extended = false;
                    }
                    else{
                        mem_value = read_memory_cpu(cpu, cpu->HL);
                    }
                    
                }
                else if(cpu->machine_cycle == 2){
                    u8 bit = (cpu->opcode & 0x38) >> 3;
                    mem_value |= (1 << bit);

                    write_memory_cpu(cpu, cpu->HL, mem_value);
                }
                else if(cpu->machine_cycle == 3){
                    go_to_next_instruction(cpu);
                    cpu->is_extended = false;
                }

                break;
            }

            default:{
                printf("Prefixed Opcode %X not implemented\n", cpu->opcode);
                break;
            } 
        }
    }
}


void handle_interrupts(CPU *cpu, PPU *ppu){
    if(cpu->handling_interrupt){
        static i32 cycle = 0;
        cpu->IME = false;
        if(cycle < 2){

        }
        else if(cycle == 2){
            push_stack(cpu, cpu->PCH);
        }
        else if(cycle == 3){
            push_stack(cpu, cpu->PCL);
        }
        else if(cycle == 4){
            u16 address = 0;
            switch(cpu->interrupt){
                case INT_VBLANK:{address = 0x40; break;}
                case INT_LCD:   {address = 0x48; ppu->stat_interrupt_set = false; break;}
                case INT_TIMER: {address = 0x50; break;}
                case INT_SERIAL:{address = 0x58; break;}
                case INT_JOYPAD:{address = 0x60; break;}
            }
            cpu->was_extended = cpu->is_extended;
            cpu->is_extended = false;
            cpu->PC = address;
            go_to_next_instruction(cpu);
            cycle = 0;
            cpu->handling_interrupt = false;
        } 
        cycle++;
    }
    else{
        u8 IE = read_memory_cpu(cpu, 0xFFFF);
        u8 IF = read_memory_cpu(cpu, 0xFF0F);

        if((IE & IF)) {
            cpu->halt = false;
        }

        if(cpu->IME && !cpu->handling_interrupt){ // If interrupts are enabled.
            if((IE & INT_VBLANK) && (IF & INT_VBLANK)){
                cpu->PC--;
                unset_interrupt(cpu, INT_VBLANK);
                cpu->interrupt = INT_VBLANK;
                cpu->handling_interrupt = true;
            }
            else if((IE & INT_LCD) && (IF & INT_LCD)){
                cpu->PC--;
                unset_interrupt(cpu, INT_LCD);
                cpu->interrupt = INT_LCD;
                cpu->handling_interrupt = true;
            }
            else if((IE & INT_TIMER) && (IF & INT_TIMER)){
                cpu->PC--;
                unset_interrupt(cpu, INT_TIMER);
                cpu->interrupt = INT_TIMER;
                cpu->handling_interrupt = true;
            }
            else if((IE & INT_SERIAL) && (IF & INT_SERIAL)){
                cpu->PC--;
                unset_interrupt(cpu, INT_SERIAL);
                cpu->interrupt = INT_TIMER;
                cpu->handling_interrupt = true;
            }
            else if((IE & INT_JOYPAD) && (IF & INT_JOYPAD)){
                cpu->PC--;
                unset_interrupt(cpu, INT_JOYPAD);
                cpu->interrupt = INT_JOYPAD;
                cpu->handling_interrupt = true;
            }
        }
    }
}

void set_interrupt(CPU *cpu, Interrupt interrupt){
    u16 address = 0xFF0F;
    u8 IF = read_memory_cpu(cpu, address);
    IF |= interrupt;
    write_memory_cpu(cpu, address, IF);
}

void unset_interrupt(CPU *cpu, Interrupt interrupt){
    u16 address = 0xFF0F;
    u8 IF = read_memory_cpu(cpu, address);
    IF &= ~(interrupt);
    write_memory_cpu(cpu, address, IF);
}

void enable_interrupt(CPU *cpu, Interrupt interrupt){
    u16 address = 0xFFFF;
    u8 IE = read_memory_cpu(cpu, address);
    IE |= interrupt;
    write_memory_cpu(cpu, address, IE);
}

void disable_interrupt(CPU *cpu, Interrupt interrupt){
    u16 address = 0xFFFF;
    u8 IE = read_memory_cpu(cpu, address);
    IE &= ~(interrupt);
    write_memory_cpu(cpu, 0xFF0F, IE);
}

static void increment_tima(CPU *cpu){
    u8 tima = read_memory_cpu(cpu, 0xFF05);
    u8 previous_tima = tima;
    tima++;
    write_memory_cpu(cpu, 0xFF05, tima);
    if(tima == 0x00 && previous_tima == 0xFF){
        set_interrupt(cpu, INT_TIMER);
        tima = read_memory_cpu(cpu, 0xFF06);
    }
}

void update_timers(CPU *cpu){
    cpu->internal_counter += 4;
    cpu->memory->data[0xFF04] = (cpu->internal_counter & 0xFF00) >> 8;

    u8 TAC = read_memory_cpu(cpu, 0xFF07);
    if(TAC & 0x04){
        u8 clock = TAC & 0x03;
        u32 every = 0;
        switch (clock){
            case 0x00:{every = 256; break;}
            case 0x01:{every = 4;   break;}
            case 0x02:{every = 16;  break;}
            case 0x03:{every = 64;  break;}
        }

        if(cpu->cycles_delta % (every*4) == 0){
            increment_tima(cpu);
        }
        
    }
}

void handle_DMA_transfer(CPU *cpu){
    if(cpu->DMA_transfer_in_progress){

        // 4 bytes are transferred at a time because the emulator timestep is 4 machine cycles and
        // DMA transfers 1 byte per machine cycle.
        u16 source = cpu->DMA_source + cpu->transferred_bytes;
        u8 byte0 = read_memory_cpu(cpu, source);
        u8 byte1 = read_memory_cpu(cpu, source + 1);
        u8 byte2 = read_memory_cpu(cpu, source + 2);
        u8 byte3 = read_memory_cpu(cpu, source + 3);
        
        u16 destination = 0xFE00 + cpu->transferred_bytes;
        write_memory_cpu(cpu, destination, byte0);
        write_memory_cpu(cpu, destination + 1, byte1);
        write_memory_cpu(cpu, destination + 2, byte2);
        write_memory_cpu(cpu, destination + 3, byte3);

        cpu->transferred_bytes += 4;

        // 160 is the size of the OAM table, DMA transfer always copies the entire table.
        if(cpu->transferred_bytes == 160){
            cpu->DMA_transfer_in_progress = false;
            cpu->transferred_bytes = 0;
        } 
    }
}