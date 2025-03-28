#include <stdio.h>
#include <windows.h>

#include "common.h"
#include "gameboy.h"
#include "arena.h"

#include "SDL3/SDL.h"

int main(int argc, const char **argv){
    if(argc < 1){
        printf("No ROM path provided\n");
        return -1;
    }

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) == 0) {
        printf("SDL2 Initialization failed: %s", SDL_GetError());
        return 0;
    }

    // SDL_Window *window;
    SDL_Window *window = SDL_CreateWindow("Space Invaders", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    if (!window) {
        printf("Error creating the window: %s", SDL_GetError());
        return 0;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        printf("Error creating the renderer: %s", SDL_GetError());
        return 0;
    }

    const bool *input = SDL_GetKeyboardState(NULL);

	LARGE_INTEGER perf_count_frequency_result;
    QueryPerformanceFrequency(&perf_count_frequency_result);
    i64 perf_count_frequency = perf_count_frequency_result.QuadPart;

    LARGE_INTEGER last_counter;
    QueryPerformanceCounter(&last_counter);

    init_global_arena(megabytes(5));
	Gameboy *gmb = (Gameboy*)alloc(sizeof(Gameboy));
	init_gameboy(gmb, renderer, argv[1]); // First argument is the rom path.

	b32 is_running = true;
    while (is_running) { // Main loop
        // Input gathering
        
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                is_running = false;
            }
        }

        run_gameboy(gmb, last_counter, perf_count_frequency, input);

        SDL_RenderPresent(renderer);

        LARGE_INTEGER end_counter;

        QueryPerformanceCounter(&end_counter);

        last_counter = end_counter;

        
    }

    fclose(gmb->cpu.fp);
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();

    free_global_arena();

	return 0;
}