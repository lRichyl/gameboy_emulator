#include "gameboy.h"

void init_gameboy(Gameboy *gmb, SDL_Renderer *renderer, const char *rom_path){
    assert(renderer);
    gmb->frame_time = 1000.0f / 59.7f; // Frame time in milliseconds.
    init_memory(&gmb->memory, rom_path);
    init_cpu(&gmb->cpu, &gmb->memory);
    init_ppu(&gmb->ppu, &gmb->memory, renderer);
}

void run_gameboy(Gameboy *gmb, LARGE_INTEGER starting_time, i64 perf_count_frequency){
    CPU *cpu = &gmb->cpu;
    PPU *ppu = &gmb->ppu;
    run_cpu(cpu);
    
    ppu_tick(ppu);
    ppu_tick(ppu);


    // Handle interrupts

    // Handle emulator timing


    while(1){// Busy wait.
        LARGE_INTEGER end_counter;
        QueryPerformanceCounter(&end_counter);

        i64 counter_elapsed = end_counter.QuadPart - starting_time.QuadPart; 
        f32 ms_elapsed     = (f32)((1000.0f*(f32)counter_elapsed) / (f32)perf_count_frequency);
        if(ms_elapsed >= gmb->frame_time){
            printf("Milliseconds elapsed: \t%f\n", ms_elapsed);
            // printf("Cycles ran last frame: \t%d\n", cycles_delta);
            break;  
        } 
    }
    ppu_render(ppu);
}