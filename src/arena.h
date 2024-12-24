#pragma once

#include "common.h"

struct Arena{
    char* start;
    char* current;
    unsigned int size;
    unsigned int occupied;
    bool initialized;
};

extern Arena global_arena;

void init_global_arena(unsigned int size);
void* alloc(unsigned int size);

void init_arena(Arena *arena, unsigned int size);
void* alloc(Arena *arena, unsigned int size);
void free_arena(Arena *arena);

