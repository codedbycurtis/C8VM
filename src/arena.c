#include <stdlib.h>

#include "arena.h"

Arena CreateArena(const size_t size)
{
    unsigned char *memory = malloc(size);
    return (Arena){
        .size = size,
        .memory = memory,
        .offset = 0
    };
}

void *RequestAllocationFromArena(Arena *arena, const size_t size)
{
    const size_t newOffset = arena->offset + size;
    if (newOffset > arena->size)
        return nullptr;
    void *allocation = arena->memory + arena->offset;
    arena->offset = newOffset;
    return allocation;
}

void ResetArena(Arena *arena)
{
    arena->offset = 0;
}

void FreeArena(Arena *arena)
{
    free(arena->memory);
    arena->memory = nullptr;
    arena->size = 0;
}