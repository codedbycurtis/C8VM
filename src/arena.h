#ifndef C8VM_ARENA_H
#define C8VM_ARENA_H

#include <stdint.h>

// A block of pre-allocated memory that smaller blocks can be requested from.
typedef struct
{
    size_t size;
    size_t offset;
    unsigned char *memory;
} Arena;

// Pre-allocates a new [Arena] of [size] bytes.
Arena CreateArena(size_t size);

// Requests [size] bytes from the [arena].
// Returns a pointer to the requested block of memory if enough is available; otherwise, a null pointer.
void *RequestAllocationFromArena(Arena *arena, size_t size);

// Resets the internal memory offset of the [arena], but does not free or zero the memory.
// After this function is called, the [arena] may still be used, but will overwrite any previously allocated memory.
void ResetArena(Arena *arena);

// Frees the pre-allocated block of memory held by the [arena], and zeroes all fields.
// After this function is called, the [arena] can no longer be used.
void FreeArena(Arena *arena);

#endif // C8VM_ARENA_H