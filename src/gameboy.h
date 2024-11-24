#pragma once
#include "common.h"
#include "CPU.h"

const i32 WINDOW_WIDTH  = 160;
const i32 WINDOW_HEIGHT = 144;

struct Gameboy{
    CPU cpu;
    // PPU ppu;
};

void init_gameboy(Gameboy *gmb);
void run_gameboy(Gameboy *gmb);