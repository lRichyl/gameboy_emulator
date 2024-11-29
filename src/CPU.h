#pragma once

#include "common.h"

const i32 NUM_REGISTERS = 8;
const i32 NUM_WIDE_REGISTERS = 4;

struct CPU{
    u8 opcode;
    bool do_first_fetch;
    i32 machine_cycle;
    float clock_speed;

    union{
        u8 instruction;
        u8 data_byte;
    };
    // Registers
    union{
        u16 BC;
        struct{
            u8 C;
            u8 B;
        };
    };
    union{
        u16 DE;
        struct{
            u8 E;
            u8 D;
        };
    };
    union{
        u16 HL;
        struct{
            u8 L;
            u8 H;
        };
    };

    union{
        u16 AF;
        struct{
            u8 flags;
            u8 A; // Accumulator
        };
    };

    u16 SP; // Stack pointer
    u16 PC; // Program counter

    u8 memory[0x10000];

    u16 *wide_register_map[NUM_WIDE_REGISTERS];
    u8  *register_map[NUM_REGISTERS];

    bool IME;
};

enum Flag {
    FLAG_CARRY     = 0x10,
    FLAG_HALFCARRY = 0x20,
    FLAG_SUB       = 0x40,
    FLAG_ZERO      = 0x80
};

void init_cpu(CPU *cpu);
i32 run_cpu(CPU *cpu);
u8 fetch(CPU *cpu);

void set_flag(CPU *cpu, Flag flag);
void unset_flag(CPU *cpu, Flag flag);
u8 pop_stack(CPU *cpu);
u8 push_stack(CPU *cpu, u8 value);
u8 sum_and_set_flags(CPU *cpu, u8 summand_left, u8 summand_right, b32 check_carry, bool check_zero);