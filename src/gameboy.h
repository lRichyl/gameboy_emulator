#pragma once
#include <windows.h>
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
    float frame_time;
};

void init_gameboy(Gameboy *gmb, SDL_Renderer *renderer, const char *rom_path);
void run_gameboy(Gameboy *gmb, LARGE_INTEGER starting_time, i64 perf_count_frequency, const bool *input);