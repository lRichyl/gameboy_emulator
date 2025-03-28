#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "arena.h"



Arena global_arena;

void init_global_arena(unsigned int size){
    init_arena(&global_arena, size);
}

void* alloc(unsigned int size){
    return alloc(&global_arena,  size);
}

void free_global_arena(){
    assert(global_arena.initialized);
    global_arena.start = global_arena.current = NULL;
    global_arena.size = 0;
    global_arena.occupied = 0;
    free(global_arena.start);
}

// Initialize the arena
void init_arena(Arena *arena, unsigned int size) {
    arena->initialized = true;
    arena->start = (char*)malloc(size);
    memset(arena->start, 0, size);
    arena->current = arena->start;
    arena->size = size;
    arena->occupied = 0;
}

// Allocate memory from the arena
void* alloc(Arena *arena, unsigned int size) {
    assert(arena->initialized);

    if (arena->current + size <= arena->start + arena->size) {
        void* result = arena->current;
        arena->current += size;
        arena->occupied += size;
        return result;
    } else {
        fprintf(stderr, "Arena overflow\n");
        assert(false);
    }
   
    return NULL;
}

// Free the entire arena
void free_arena(Arena *arena) {
    assert(arena->initialized);
    free(arena->start);
    arena->start = arena->current = NULL;
    arena->size = 0;
    arena->occupied = 0;
}
