#pragma once
#include "common.h"
#include "CPU.h"
#include "ppu.h"

const i32 WINDOW_WIDTH  = 160;
const i32 WINDOW_HEIGHT = 144;

struct Gameboy{
    CPU cpu;
    Memory memory;
    PPU ppu;
    i32 cycle_count;
};

void init_gameboy(Gameboy *gmb, const char *rom_path);
void run_gameboy(Gameboy *gmb);