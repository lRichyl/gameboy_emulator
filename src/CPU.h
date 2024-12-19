#pragma once

#include "common.h"
#include "memory.h"

const i32 NUM_REGISTERS = 8;
const i32 NUM_WIDE_REGISTERS = 4;

enum Flag {
    FLAG_CARRY     = 0x10,
    FLAG_HALFCARRY = 0x20,
    FLAG_SUB       = 0x40,
    FLAG_ZERO      = 0x80
};

enum Interrupt{
    INT_VBLANK = 0x01,
    INT_LCD    = 0x02,
    INT_TIMER  = 0x04,
    INT_SERIAL = 0x08,
    INT_JOYPAD = 0x10,
};

struct CPU{
    u8 opcode;
    bool do_first_fetch;
    i32 machine_cycle;
    float clock_speed;
    u32 machine_cycles_per_frame;
    float frame_time;
    u32 cycles_delta;
    float period;

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
    union{
        u16 SP; // Stack pointer
        struct{
            u8 SPL;
            u8 SPH;
        };
    };

    union{
        u16 PC; // Program counter
        struct{
            u8 PCL;
            u8 PCH;
        };
    };

    u16 internal_counter;

    Memory *memory;

    u16 *wide_register_map[NUM_WIDE_REGISTERS];
    u8  *register_map[NUM_REGISTERS];

    bool IME;
    bool scheduled_ei;
    bool is_extended;
    bool handling_interrupt;
    bool halt;
    bool fetched_next_instruction;
    bool was_extended;
    Interrupt interrupt;
    
    bool DMA_transfer_in_progress;
    u8 transferred_bytes;
    u16 DMA_source;
};

struct PPU;


void init_cpu(CPU *cpu, Memory *memory);
i32 run_cpu(CPU *cpu);
u8 fetch(CPU *cpu);
void go_to_next_instruction(CPU *cpu);

void set_flag(CPU *cpu, Flag flag);
void unset_flag(CPU *cpu, Flag flag);
u8 pop_stack(CPU *cpu);
u8 push_stack(CPU *cpu, u8 value);
u8 sum_and_set_flags(CPU *cpu, u8 summand_left, u8 summand_right, b32 check_carry, bool check_zero);

void handle_DMA_transfer(CPU *cpu);
void handle_interrupts(CPU *cpu, PPU *ppu);
void set_interrupt(CPU *cpu, Interrupt interrupt);
void unset_interrupt(CPU *cpu, Interrupt interrupt);
void enable_interrupt(CPU *cpu, Interrupt interrupt);
void disable_interrupt(CPU *cpu, Interrupt interrupt);

void update_timers(CPU *cpu);