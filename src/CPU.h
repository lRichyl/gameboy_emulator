#pragma once

#include "common.h"

struct CPU{
    u8 opcode;
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
};

enum Flag {
    FLAG_CARRY     = 0x10,
    FLAG_HALFCARRY = 0x20,
    FLAG_SUB       = 0x40,
    FLAG_ZERO      = 0x80
};

void init_cpu(CPU *cpu);
void decode(CPU *cpu);
i32 execute_instruction(CPU *cpu);
void fetch(CPU *cpu);