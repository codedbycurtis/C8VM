#ifndef C8_VM_H
#define C8_VM_H

#include <stdint.h>

// The horizontal resolution of the virtual display.
#define CHIP_8_DISPLAY_WIDTH  64

// The vertical resolution of the virtual display.
#define CHIP_8_DISPLAY_HEIGHT 32

// The width (in bytes) of each instruction.
#define INSTRUCTION_WIDTH sizeof(uint16_t)

// The location in virtual memory of the default font.
#define FONT_SPRITE_OFFSET 0x50

// The width (in pixels) of each font sprite.
#define FONT_SPRITE_WIDTH 5

// The location in virtual memory of the loaded program's first instruction.
#define PROGRAM_OFFSET 0x200

// The maximum value that should be returned from the random number generator.
#define CHIP_8_RAND_MAX 0xFF

// The virtual machine is not currently awaiting any key presses.
#define NOT_AWAITING (-1)

// Configures the behaviour of some CHIP-8 instructions to enable compatability with modern interpreters.
typedef struct
{
	// If true (CHIP-48, SUPER-CHIP), shifts the value in the V(x) register in place and ignores the V(y) register.
	// If false (COSMAC VIP), the value in the V(y) register is first copied into the V(x) register and then shifted.
	// Affects the behaviour of both the 0x8XY6 (shift right) and 0x8XYE (shift left) instructions.
	bool useParameterisedShift;

	// If true (CHIP-48, SUPER-CHIP), adds the value in the V(x) register to the address before the jump.
	// If false (COSMAC VIP), adds the value in the V0 register to the address before the jump.
	// Affects the behaviour of the 0xBXNN (jump with offset) instruction.
	bool useParameterisedJump;

	// If true (CHIP-48, SUPER-CHIP), uses a temporary variable for the address offset, leaving the value in the index register unchanged.
	// If false, (COSMAC VIP), increments the index register when calculating the address offset.
	// Affects the behaviour of both the 0xFX55 (store registers to memory) and 0xFX65 (store memory to registers) instructions.
	bool useTemporaryIndex;
} C8_Config;

// Represents a decoded CHIP-8 instruction.
typedef struct
{
	uint8_t  opcode;
	uint8_t  x;
	uint8_t  y;
	uint8_t  n;
	uint8_t  nn;
	uint16_t nnn;
} C8_Instruction;

// Represents the internal state of the virtual machine.
typedef struct
{
	// The virtual machine's configuration.
	C8_Config config;

	// The current instruction being executed by the virtual machine.
	C8_Instruction instruction;

	// General-purpose registers (V0-VF)
	uint8_t v[16];

	// Index Register
	uint16_t i;

	// Program Counter
	uint16_t pc;

	// Stack Pointer
	uint16_t sp;

	// Delay Timer
	uint8_t dt;

	// Sound Timer
	uint8_t st;

	// Heap memory containing program instructions and data.
	uint8_t heap[4096];

	// Function call stack.
	uint16_t stack[16];

	// If a key press is being awaited, this is the register it will be stored in.
	int8_t awaitKeyPressRegister;

	// The pixels composing the current frame.
	bool framebuffer[CHIP_8_DISPLAY_WIDTH * CHIP_8_DISPLAY_HEIGHT];

	// The state of the virtual machine's hexadecimal (0-F) keypad.
	bool keysPressed[16];
} C8_Instance;

// Performs a fetch-execute cycle for the provided virtual machine.
void C8_FetchExecute(C8_Instance *vm);

// Updates the delay and sound timers.
// This function should be called at a rate of 60Hz.
void C8_UpdateTimers(C8_Instance *vm);

// Notifies the virtual machine that the specified key has been pressed or released.
void C8_NotifyKeyEvent(C8_Instance *vm, uint8_t key, bool isKeyPressed);

// Loads a CHIP-8 program and initialises the virtual machine.
// If this function returns false, error will be populated with a string describing the reason.
// Returns true if the program was loaded successfully; otherwise, false.
bool C8_LoadProgram(C8_Instance *instance, const char *filePath, char **error);

// Resets the state of the virtual machine.
void C8_Reset(C8_Instance *vm);

#endif // C8_VM_H