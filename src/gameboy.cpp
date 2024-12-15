#include "gameboy.h"

void init_gameboy(Gameboy *gmb, SDL_Renderer *renderer, const char *rom_path){
    assert(renderer);
    gmb->frame_time = 1000.0f / 59.7f; // Frame time in milliseconds.
    init_memory(&gmb->memory, rom_path);
    init_cpu(&gmb->cpu, &gmb->memory);
    init_ppu(&gmb->ppu, &gmb->memory, renderer);
}

void increment_div(Memory *memory){
    u8 div = read_memory(memory, 0xFF04);
    div++;
    write_memory(memory, 0xFF04, div);
}

void increment_tima(Memory *memory){
    u8 tima = read_memory(memory, 0xFF05);
    u8 previous_tima = tima;
    write_memory(memory, 0xFF05, tima);
    tima++;
    if(tima == 0x00 && previous_tima == 0xFF){
        set_interrupt(memory, INT_TIMER);
        tima = read_memory(memory, 0xFF06);
    }
}

void run_gameboy(Gameboy *gmb, LARGE_INTEGER starting_time, i64 perf_count_frequency){
    CPU *cpu = &gmb->cpu;
    PPU *ppu = &gmb->ppu;
    while(!ppu->frame_ready){
        // Handling interrupts
        if(cpu->fetched_next_instruction){
            handle_interrupts(cpu);
        }
        if(!cpu->handling_interrupt && !cpu->halt){
            cpu->cycles_delta += run_cpu(cpu);
        }
        
        write_memory(cpu->memory, 0xFF04, (cpu->internal_counter & 0xFF00) >> 8);
        cpu->internal_counter += 4;
        // if(cpu->cycles_delta % 256 == 0){
        //     increment_div(cpu->memory);
        // }

        u8 TAC = read_memory(cpu->memory, 0xFF07);
        if(TAC & 0x04){
            u8 clock = TAC & 0x03;
            u32 every = 0;
            switch (clock){
                case 0x00:{every = 256; break;}
                case 0x01:{every = 4; break;}
                case 0x02:{every = 16; break;}
                case 0x03:{every = 64; break;}
            }

            if(cpu->cycles_delta % (every * 4) == 0){
                increment_tima(cpu->memory);
            }
            
        }
        //printf("Cycles %d \n", cpu->cycles_delta);
       
        ppu_tick(ppu, cpu);
        ppu_tick(ppu, cpu);

        //printf("LX: \t%X\n", ppu->pixel_count);
        //printf("PPU cycles: \t%d, Tile x %d\n", ppu->cycles, ppu->tile_x);

        

    }

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
    
    
    ppu->frame_ready = false;
    cpu->cycles_delta -= cpu->machine_cycles_per_frame;
    
}