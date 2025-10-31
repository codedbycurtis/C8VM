#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"

// Default font used by the virtual machine.
static const uint8_t DEFAULT_FONT[] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// Clears the display.
static void C8_00E0(C8_Instance *instance)
{
	memset(instance->framebuffer, 0, sizeof(instance->framebuffer));
}

// Returns from the current subroutine.
static void C8_00EE(C8_Instance *instance)
{
	instance->pc = instance->stack[--instance->sp];
	instance->stack[instance->sp] = 0;
}

// Jumps to the specified address.
static void C8_1NNN(C8_Instance *instance)
{
	instance->pc = instance->instruction.nnn;
}

// Calls the subroutine at (nnn).
static void C8_2NNN(C8_Instance *instance)
{
	instance->stack[instance->sp++] = instance->pc;
	instance->pc = instance->instruction.nnn;
}

// Skips the next instruction if the value in the V(x) register is equal to (nn).
static void C8_3XNN(C8_Instance *instance)
{
	if (instance->v[instance->instruction.x] == instance->instruction.nn)
		instance->pc += INSTRUCTION_WIDTH;
}

// Skips the next instruction if the value in the V(x) register is not equal to (nn).
static void C8_4XNN(C8_Instance *instance)
{
	if (instance->v[instance->instruction.x] != instance->instruction.nn)
		instance->pc += INSTRUCTION_WIDTH;
}

// Skips the next instruction if the value in the V(x) register is equal to the value in the V(y) register.
static void C8_5XY0(C8_Instance *instance)
{
	if (instance->v[instance->instruction.x] == instance->v[instance->instruction.y])
		instance->pc += INSTRUCTION_WIDTH;
}

// Loads the immediate value (nn) into the V(x) register.
static void C8_6XNN(C8_Instance *instance)
{
	instance->v[instance->instruction.x] = instance->instruction.nn;
}

// Adds the immediate value (nn) to the value in the V(x) register.
static void C8_7XNN(C8_Instance *instance)
{
	instance->v[instance->instruction.x] += instance->instruction.nn;
}

// Loads the value in the V(y) register into the V(x) register.
static void C8_8XY0(C8_Instance *instance)
{
	instance->v[instance->instruction.x] = instance->v[instance->instruction.y];
}

// Loads the result of V(x) OR V(y) into the V(x) register.
static void C8_8XY1(C8_Instance *instance)
{
	instance->v[instance->instruction.x] |= instance->v[instance->instruction.y];
}

// Loads the result of V(x) AND V(y) into the V(x) register.
static void C8_8XY2(C8_Instance *instance)
{
	instance->v[instance->instruction.x] &= instance->v[instance->instruction.y];
}

// Loads the result of V(x) XOR V(y) into the V(x) register.
static void C8_8XY3(C8_Instance *instance)
{
	instance->v[instance->instruction.x] ^= instance->v[instance->instruction.y];
}

// Loads the result of V(x) + V(y) into the V(x) register.
// Sets V(F) to 1 if an overflow occurs; otherwise, 0.
static void C8_8XY4(C8_Instance *instance)
{
	const uint16_t result = instance->v[instance->instruction.x] + instance->v[instance->instruction.y];
	instance->v[instance->instruction.x] = result;
	instance->v[0xF] = result > UINT8_MAX ? 1 : 0;
}

// Loads the result of V(x) - V(y) into the V(x) register.
// Sets V(F) to 0 if an underflow occurs; otherwise, 1.
static void C8_8XY5(C8_Instance *instance)
{
	const int result = instance->v[instance->instruction.x] - instance->v[instance->instruction.y];
	instance->v[instance->instruction.x] = result;
	instance->v[0xF] = result < 0 ? 0 : 1;
}

// Logically shifts the value in the V(x) register to the right by 1 bit.
// The bit that was shifted out is stored in the V(F) register.
static void C8_8XY6(C8_Instance *instance)
{
	const uint8_t flag = instance->v[instance->instruction.x] & 0x01;
	if (!instance->config.useParameterisedShift)
	{
		instance->v[instance->instruction.x] = instance->v[instance->instruction.y];
	}
	instance->v[instance->instruction.x] >>= 1;
	instance->v[0xF] = flag;
}

static void C8_8XY7(C8_Instance *instance)
{
	const int result = instance->v[instance->instruction.y] - instance->v[instance->instruction.x];
	instance->v[instance->instruction.x] = result;
	instance->v[0xF] = result < 0 ? 0 : 1;
}

static void C8_8XYE(C8_Instance *instance)
{
	const uint8_t flag = instance->v[instance->instruction.x] >> 7;
	if (!instance->config.useParameterisedShift)
	{
		instance->v[instance->instruction.x] = instance->v[instance->instruction.y];
	}
	instance->v[instance->instruction.x] <<= 1;
	instance->v[0xF] = flag;
}

// Skips the next instruction if the value in V(x) is not equal to the value in V(y).
static void C8_9XY0(C8_Instance *instance)
{
	if (instance->v[instance->instruction.x] != instance->v[instance->instruction.y])
	{
		instance->pc += INSTRUCTION_WIDTH;
	}
}

// Loads the specified value into the index register.
static void C8_ANNN(C8_Instance *instance)
{
	instance->i = instance->instruction.nnn;
}

// If 'use parameterised jump' is disabled, adds the value in the V0 register to (nnn) and jumps to the address;
// otherwise, adds the value in the V(x) register to (xnn) and jumps to the address.
static void C8_BNNN(C8_Instance *instance)
{
	instance->pc = instance->config.useParameterisedJump ? instance->instruction.nnn + instance->v[instance->instruction.x] : instance->instruction.nnn + instance->v[0];
}

// Generates a random number the range 0..255, ANDs it with (nn) and stores the result in the V(x) register.
static void C8_CXNN(C8_Instance *instance)
{
	instance->v[instance->instruction.x] = rand() % (CHIP_8_RAND_MAX + 1) & instance->instruction.nn;
}

// Draws an (n)-pixels tall sprite at the co-ordinates in the V(x) and V(y) registers.
static void C8_DXYN(C8_Instance *instance)
{
	const uint8_t x = instance->v[instance->instruction.x];
	const uint8_t y = instance->v[instance->instruction.y];
	const uint16_t sprite = instance->i;

	instance->v[0xF] = 0;

	for (uint8_t i = 0; i < instance->instruction.n; ++i)
	{
		const uint8_t spriteRow = instance->heap[sprite + i];
		for (uint8_t j = 0; j < 8; ++j)
		{
			if (spriteRow << j & 0x80)
			{
				const uint8_t coordX = (x + j) % CHIP_8_DISPLAY_WIDTH;
				const uint8_t coordY = (y + i) % CHIP_8_DISPLAY_HEIGHT;
				const uint16_t index = coordY * 64 + coordX;

				if (instance->framebuffer[index])
					instance->v[0xF] = 1;

				instance->framebuffer[index] = !instance->framebuffer[index];
			}
		}
	}
}

// Skips the next instruction if the key corresponding to the value in the V(x) register is pressed.
static void C8_EX9E(C8_Instance *instance)
{
	if (instance->keysPressed[instance->v[instance->instruction.x]])
	{
		instance->pc += INSTRUCTION_WIDTH;
	}
}

// Skips the next instruction if the key corresponding to the value in the V(x) register is not pressed.
static void C8_EXA1(C8_Instance *instance)
{
	if (!instance->keysPressed[instance->v[instance->instruction.x]])
	{
		instance->pc += INSTRUCTION_WIDTH;
	}
}

// Loads the value of the delay timer into the V(x) register.
static void C8_FX07(C8_Instance *instance)
{
	instance->v[instance->instruction.x] = instance->dt;
}

// Halts execution until a key is pressed and stores the key's value in the V(x) register.
static void C8_FX0A(C8_Instance *instance)
{
	instance->awaitKeyPressRegister = instance->instruction.x;
	instance->pc -= INSTRUCTION_WIDTH;
}

// Sets the delay timer to the value in the V(x) register.
static void C8_FX15(C8_Instance *instance)
{
	instance->dt = instance->v[instance->instruction.x];
}

// Sets the sound timer to the value in the V(x) register.
static void C8_FX18(C8_Instance *instance)
{
	instance->st = instance->v[instance->instruction.x];
}

// Adds the value in the V(x) register to the index register.
static void C8_FX1E(C8_Instance *instance)
{
	instance->i += instance->v[instance->instruction.x];
}

// Sets the index register to the memory address of the sprite for the value in the V(x) register.
static void C8_FX29(C8_Instance *instance)
{
	instance->i = FONT_SPRITE_OFFSET + instance->v[instance->instruction.x] * FONT_SPRITE_WIDTH;
}

// Loads the binary-coded decimal representation of the value in
// the V(x) register into memory at the location in the index register.
static void C8_FX33(C8_Instance *instance)
{
	instance->heap[instance->i] = instance->v[instance->instruction.x] / 100;
	instance->heap[instance->i + 1] = instance->v[instance->instruction.x] / 10 % 10;
	instance->heap[instance->i + 2] = instance->v[instance->instruction.x] % 10;
}

// Stores the values in registers V0 to V(x) in successive memory addresses, starting at the address in the index register.
// If 'use temporary index' is enabled, a temporary variable will be used and the value in the index register will remain unchanged.
static void C8_FX55(C8_Instance *instance)
{
	if (instance->config.useTemporaryIndex)
	{
		for (uint8_t i = 0; i <= instance->instruction.x; ++i)
		{
			instance->heap[instance->i + i] = instance->v[i];
		}
	}
	else
	{
		for (uint8_t i = 0; i <= instance->instruction.x; ++i)
		{
			instance->heap[instance->i++] = instance->v[i];
		}
	}
}

// Stores values in memory in the registers V0 to V(x), starting at the address in the index register.
// If 'use temporary index' is enabled, a temporary variable will be used and the value in the index register will remain unchanged.
static void C8_FX65(C8_Instance *instance)
{
	if (instance->config.useTemporaryIndex)
	{
		for (uint8_t i = 0; i <= instance->instruction.x; ++i)
		{
			instance->v[i] = instance->heap[instance->i + i];
		}
	}
	else
	{
		for (uint8_t i = 0; i <= instance->instruction.x; ++i)
		{
			instance->v[i] = instance->heap[instance->i++];
		}
	}
}

void C8_FetchExecute(C8_Instance *instance)
{
	// Fetch
	const uint16_t addr = instance->pc;
	const uint16_t inst = (instance->heap[addr] << 8) | instance->heap[addr + 1]; // Combine two adjacent bytes into a 16-bit instruction

	// Decode
	instance->instruction = (const C8_Instruction){
		.opcode = (inst & 0xF000) >> 12,
		.x = (inst & 0x0F00) >> 8,
		.y = (inst & 0x00F0) >> 4,
		.n = inst & 0x000F,
		.nn = inst & 0x00FF,
		.nnn = inst & 0x0FFF
	};

	// Execute
	instance->pc += INSTRUCTION_WIDTH;

	switch (instance->instruction.opcode)
	{
		case 0x0:
		{
			switch (instance->instruction.nnn)
			{
				case 0x0E0:
					C8_00E0(instance);
					break;
				case 0x0EE:
					C8_00EE(instance);
					break;
				default:
					break;
			}
			break;
		}
		case 0x1:
			C8_1NNN(instance);
			break;
		case 0x2:
			C8_2NNN(instance);
			break;
		case 0x3:
			C8_3XNN(instance);
			break;
		case 0x4:
			C8_4XNN(instance);
			break;
		case 0x5:
			C8_5XY0(instance);
			break;
		case 0x6:
			C8_6XNN(instance);
			break;
		case 0x7:
			C8_7XNN(instance);
			break;
		case 0x8:
		{
			switch (instance->instruction.n)
			{
				case 0x0:
					C8_8XY0(instance);
					break;
				case 0x1:
					C8_8XY1(instance);
					break;
				case 0x2:
					C8_8XY2(instance);
					break;
				case 0x3:
					C8_8XY3(instance);
					break;
				case 0x4:
					C8_8XY4(instance);
					break;
				case 0x5:
					C8_8XY5(instance);
					break;
				case 0x6:
					C8_8XY6(instance);
					break;
				case 0x7:
					C8_8XY7(instance);
					break;
				case 0xE:
					C8_8XYE(instance);
					break;
				default:
					break;
			}
			break;
		}
		case 0x9:
			C8_9XY0(instance);
			break;
		case 0xA:
			C8_ANNN(instance);
			break;
		case 0xB:
			C8_BNNN(instance);
			break;
		case 0xC:
			C8_CXNN(instance);
			break;
		case 0xD:
			C8_DXYN(instance);
			break;
		case 0xE:
		{
			switch (instance->instruction.nn)
			{
				case 0x9E:
					C8_EX9E(instance);
					break;
				case 0xA1:
					C8_EXA1(instance);
					break;
				default:
					break;
			}
			break;
		}
		case 0xF:
		{
			switch (instance->instruction.nn)
			{
				case 0x07:
					C8_FX07(instance);
					break;
				case 0x0A:
					C8_FX0A(instance);
					break;
				case 0x15:
					C8_FX15(instance);
					break;
				case 0x18:
					C8_FX18(instance);
					break;
				case 0x1E:
					C8_FX1E(instance);
					break;
				case 0x29:
					C8_FX29(instance);
					break;
				case 0x33:
					C8_FX33(instance);
					break;
				case 0x55:
					C8_FX55(instance);
					break;
				case 0x65:
					C8_FX65(instance);
					break;
				default:
					break;
			}
			break;
		}
		default:
			break;
	}
}

void C8_UpdateTimers(C8_Instance *instance)
{
	if (instance->dt > 0)
		--instance->dt;

	if (instance->st > 0)
		--instance->st;
}

void C8_NotifyKeyEvent(C8_Instance *instance, const uint8_t key, const bool isKeyPressed)
{
	if (instance->awaitKeyPressRegister == NOT_AWAITING || !isKeyPressed)
	{
		instance->keysPressed[key] = isKeyPressed;
		return;
	}

	instance->v[instance->awaitKeyPressRegister] = key;
	instance->awaitKeyPressRegister = NOT_AWAITING;
	instance->pc += INSTRUCTION_WIDTH;
}

bool C8_LoadProgram(C8_Instance *instance, const char *filePath, char **error)
{
	FILE *file = fopen(filePath, "rb");
	if (!file)
	{
		*error = "Failed to open file at the specified path.";
		return false;
	}

	// 4KiB (heap size) - 512 bytes reserved = 3.5KiB (maximum program size)
	constexpr uint16_t bufferSize = 4096 - 0x200;
	uint8_t *buf = malloc(sizeof(uint8_t) * bufferSize);
	if (!buf)
	{
		*error = "Failed to allocate memory.";
		fclose(file);
		return false;
	}

	int byte;
	uint16_t count = 0;
	while ((byte = fgetc(file)) != EOF)
	{
		if (count > bufferSize)
		{
			*error = "Failed to load program - exceeded 3.5KiB limit.";
			fclose(file);
			free(buf);
			return false;
		}
		buf[count++] = byte;
	}

	memcpy(&instance->heap[PROGRAM_OFFSET], buf, count);

	memcpy(&instance->heap[FONT_SPRITE_OFFSET], DEFAULT_FONT, sizeof(DEFAULT_FONT));

	instance->pc = PROGRAM_OFFSET;

	fclose(file);
	free(buf);

	return true;
}

void C8_Reset(C8_Instance *instance)
{
	const C8_Config prevConfig = instance->config;
	*instance = (C8_Instance){ 0 };
	instance->config = prevConfig;
}