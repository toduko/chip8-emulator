#ifndef CHIP8_H
#define CHIP8_H

#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <random>

const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x50;
const unsigned int START_ADDRESS = 0x200;
const unsigned int KEY_COUNT = 16;
const unsigned int MEMORY_SIZE = 4096;
const unsigned int REGISTER_COUNT = 16;
const unsigned int STACK_LEVELS = 16;
const unsigned int VIDEO_HEIGHT = 32;
const unsigned int VIDEO_WIDTH = 64;


uint8_t fontset[FONTSET_SIZE] = {
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

class Chip8 {
public:
	uint8_t keypad[KEY_COUNT]{};
	uint32_t video[VIDEO_WIDTH * VIDEO_HEIGHT]{};

	Chip8() : randGen(std::chrono::system_clock::now().time_since_epoch().count()) {
		// init program counter
		pc = START_ADDRESS;

		// load font into memory
		for (unsigned int i = 0; i < FONTSET_SIZE; ++i) {
			memory[FONTSET_START_ADDRESS + i] = fontset[i];
		}

		randByte = std::uniform_int_distribution<uint8_t>(0, 255U);

		// function pointer table
		table[0x0] = &Table0;
		table[0x1] = &OP_1NNN;
		table[0x2] = &OP_2NNN;
		table[0x3] = &OP_3XKK;
		table[0x4] = &OP_4XKK;
		table[0x5] = &OP_5XY0;
		table[0x6] = &OP_6XKK;
		table[0x7] = &OP_7XKK;
		table[0x8] = &Table8;
		table[0x9] = &OP_9XY0;
		table[0xA] = &OP_ANNN;
		table[0xB] = &OP_BNNN;
		table[0xC] = &OP_CXKK;
		table[0xD] = &OP_DXYN;
		table[0xE] = &TableE;
		table[0xF] = &TableF;

		table0[0x0] = &OP_00E0;
		table0[0xE] = &OP_00EE;

		table8[0x0] = &OP_8XY0;
		table8[0x1] = &OP_8XY1;
		table8[0x2] = &OP_8XY2;
		table8[0x3] = &OP_8XY3;
		table8[0x4] = &OP_8XY4;
		table8[0x5] = &OP_8XY5;
		table8[0x6] = &OP_8XY6;
		table8[0x7] = &OP_8XY7;
		table8[0xE] = &OP_8XYE;

		tableE[0x1] = &OP_EXA1;
		tableE[0xE] = &OP_EX9E;

		tableF[0x07] = &OP_FX07;
		tableF[0x0A] = &OP_FX0A;
		tableF[0x15] = &OP_FX15;
		tableF[0x18] = &OP_FX18;
		tableF[0x1E] = &OP_FX1E;
		tableF[0x29] = &OP_FX29;
		tableF[0x33] = &OP_FX33;
		tableF[0x55] = &OP_FX55;
		tableF[0x65] = &OP_FX65;
	}

    void LoadROM(char const* filename) {
		std::ifstream file(filename, std::ios::binary | std::ios::ate);

		if (file.is_open()) {
			std::streampos size = file.tellg();
            char* buffer = new char[size];
            file.seekg(0, std::ios::beg);
            file.read(buffer, size);
            file.close();

			for (long i = 0; i < size; i++) {
				memory[START_ADDRESS + i] = buffer[i];
			}

			// free buffer
			delete[] buffer;
		}
	}

	void Cycle() {
		// fetch
		opcode = (memory[pc] << 8u) | memory[pc + 1];

		// increment program counter before execution
		pc += 2;

		// decode and execute
		((*this).*(table[(opcode & 0xF000u) >> 12u]))();

		if (delayTimer > 0)
			--delayTimer;

		if (soundTimer > 0)
			--soundTimer;
	}
private:
    uint8_t memory[MEMORY_SIZE]{};
	uint8_t registers[REGISTER_COUNT]{};
	uint16_t index{};
	uint16_t pc{};
	uint8_t delayTimer{};
	uint8_t soundTimer{};
	uint16_t stack[STACK_LEVELS]{};
	uint8_t sp{};
	uint16_t opcode{};

	std::default_random_engine randGen;
	std::uniform_int_distribution<uint8_t> randByte;

	typedef void (Chip8::*Chip8Func)();
	Chip8Func table[0xF + 1]{&OP_NULL};
	Chip8Func table0[0xE + 1]{&OP_NULL};
	Chip8Func table8[0xE + 1]{&OP_NULL};
	Chip8Func tableE[0xE + 1]{&OP_NULL};
	Chip8Func tableF[0x65 + 1]{&OP_NULL};

	void Table0() {
		((*this).*(table0[opcode & 0x000Fu]))();
	}

	void Table8() {
		((*this).*(table8[opcode & 0x000Fu]))();
	}

	void TableE() {
		((*this).*(tableE[opcode & 0x000Fu]))();
	}

	void TableF() {
		((*this).*(tableF[opcode & 0x00FFu]))();
	}

	void OP_NULL() {}

	// 00E0 - clear the display
	void OP_00E0() {
		memset(video, 0, sizeof(video));
	}

	// 00EE - return from a subroutine
	void OP_00EE() {
		pc = stack[--sp];
	}

	// 1NNN - jump to location NNN
	void OP_1NNN() {
		uint16_t address = opcode & 0x0FFFu;
		pc = address;
	}

	// 2NNN - call subroutine at NNN
	void OP_2NNN() {
		uint16_t address = opcode & 0x0FFFu;
		stack[sp++] = pc;
		pc = address;
	}

	// 3XKK - skip next instruction if VX == KK
	void OP_3XKK() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t byte = opcode & 0x00FFu;

		if (registers[VX] == byte)
			pc += 2;
	}

	// 4XKK - skip next instruction if VX != KK
	void OP_4XKK() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t byte = opcode & 0x00FFu;

		if (registers[VX] != byte)
			pc += 2;
	}

	// 5XY0 - skip next instruction if VX == VY
	void OP_5XY0() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t VY = (opcode & 0x00F0u) >> 4u;

		if (registers[VX] == registers[VY])
			pc += 2;
	}

	// 6XKK - set VX to KK
	void OP_6XKK() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t byte = opcode & 0x00FFu;

		registers[VX] = byte;
	}

	// 7XKK - set VX to VX + KK
	void OP_7XKK() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t byte = opcode & 0x00FFu;

		registers[VX] += byte;
	}

	// 8XY0 - set VX to VY
	void OP_8XY0() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t VY = (opcode & 0x00F0u) >> 4u;

		registers[VX] = registers[VY];
	}

	// 8XY1 - set VX to VX OR VY
	void OP_8XY1() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t VY = (opcode & 0x00F0u) >> 4u;

		registers[VX] |= registers[VY];
	}

	// 8XY2 - set VX to VX AND VY
	void OP_8XY2() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t VY = (opcode & 0x00F0u) >> 4u;

		registers[VX] &= registers[VY];
	}

	// 8XY3 - set VX to VX XOR VY
	void OP_8XY3() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t VY = (opcode & 0x00F0u) >> 4u;

		registers[VX] ^= registers[VY];
	}

	// 8XY4 - set VX to VX + VY and set VF to carry
	void OP_8XY4() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t VY = (opcode & 0x00F0u) >> 4u;

		uint16_t sum = registers[VX] + registers[VY];

		if (sum > 255U)
			registers[0xF] = 1;
		else
			registers[0xF] = 0;

		registers[VX] = sum & 0xFFu;
	}

	// 8XY5 - set VX to VX - VY and VF to NOT borrow
	void OP_8XY5() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t VY = (opcode & 0x00F0u) >> 4u;

		if (registers[VX] > registers[VY])
			registers[0xF] = 1;
		else
			registers[0xF] = 0;

		registers[VX] -= registers[VY];
	}

	// 8XY6 - store the least significant bit of VX in VF and shift VX to the right by 1
	void OP_8XY6() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;

		registers[0xF] = (registers[VX] & 0x1u);

		registers[VX] >>= 1;
	}

	// 8XY7 - set VX to VX - VY and VF to NOT borrow
	void OP_8XY7() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t VY = (opcode & 0x00F0u) >> 4u;

		if (registers[VY] > registers[VX])
			registers[0xF] = 1;
		else
			registers[0xF] = 0;

		registers[VX] = registers[VY] - registers[VX];
	}

	// 8XYE - store the most significant bit of VX in VF and shift VX to the left by 1
	void OP_8XYE() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;

		registers[0xF] = (registers[VX] & 0x80u) >> 7u;

		registers[VX] <<= 1;
	}

	// 9XY0 - skip next instruction if VX != VY
	void OP_9XY0() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t VY = (opcode & 0x00F0u) >> 4u;

		if (registers[VX] != registers[VY])
			pc += 2;
	}

	// ANNN - set the index register to NNN
	void OP_ANNN() {
		uint16_t address = opcode & 0x0FFFu;

		index = address;
	}

	// BNNN - jump to location NNN + V0
	void OP_BNNN() {
		uint16_t address = opcode & 0x0FFFu;

		pc = registers[0] + address;
	}

	// CXKK - set Vx to random byte AND KK
	void OP_CXKK() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t byte = opcode & 0x00FFu;

		registers[VX] = randByte(randGen) & byte;
	}

	// DXYN - draw sprite at coordinates (VX, VY) with a width of 8 pixels and height of N pixels
	void OP_DXYN() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t VY = (opcode & 0x00F0u) >> 4u;
		uint8_t height = opcode & 0x000Fu;

		uint8_t xPos = registers[VX] % VIDEO_WIDTH;
		uint8_t yPos = registers[VY] % VIDEO_HEIGHT;

		registers[0xF] = 0;

        for (unsigned int row = 0; row < height; ++row) {
            uint8_t spriteByte = memory[index + row];

            for (unsigned int col = 0; col < 8; ++col) {
                uint8_t spritePixel = spriteByte & (0x80u >> col);
                uint32_t* screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];

                if (spritePixel) {
                    if (*screenPixel == 0xFFFFFFFF)
                        registers[0xF] = 1;
                    *screenPixel ^= 0xFFFFFFFF;
                }
            }
        }
	}
	// EX9E - skip next instruction if key with the value of VX is pressed
	void OP_EX9E() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t key = registers[VX];

		if (keypad[key])
			pc += 2;
	}

	// EXA1 - skip next instruction if key with the value of VX is not pressed
	void OP_EXA1() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t key = registers[VX];

		if (!keypad[key])
			pc += 2;
	}

	// FX07 - set VX to delay timer value
	void OP_FX07() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;

		registers[VX] = delayTimer;
	}

	// FX0A - await key press and store it in VX
	void OP_FX0A() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;

		if (keypad[0])
			registers[VX] = 0;
		else if (keypad[1])
			registers[VX] = 1;
		else if (keypad[2])
			registers[VX] = 2;
		else if (keypad[3])
			registers[VX] = 3;
		else if (keypad[4])
			registers[VX] = 4;
		else if (keypad[5])
			registers[VX] = 5;
		else if (keypad[6])
			registers[VX] = 6;
		else if (keypad[7])
			registers[VX] = 7;
		else if (keypad[8])
			registers[VX] = 8;
		else if (keypad[9])
			registers[VX] = 9;
		else if (keypad[10])
			registers[VX] = 10;
		else if (keypad[11])
			registers[VX] = 11;
		else if (keypad[12])
			registers[VX] = 12;
		else if (keypad[13])
			registers[VX] = 13;
		else if (keypad[14])
			registers[VX] = 14;
		else if (keypad[15])
			registers[VX] = 15;
		else
			pc -= 2;
	}

	// FX15 - set delay timer to VX
	void OP_FX15() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;

		delayTimer = registers[VX];
	}

	// FX18 - set sound timer to VX
	void OP_FX18() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;

		soundTimer = registers[VX];
	}

	// FX1E - set index register to index + VX
	void OP_FX1E() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;

		index += registers[VX];
	}

	// FX29 - set index register to location of sprite for digit VX
	void OP_FX29() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t digit = registers[VX];

		// fontset characters start at 0x50 and are 5 bytes each
		index = FONTSET_START_ADDRESS + (5 * digit);
	}

	// FX33 - store VX binary in the memory locations of index, index + 1, index + 2
	void OP_FX33() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;
		uint8_t value = registers[VX];

		// ones' place
        memory[index + 2] = value % 10;
        value /= 10;

        // tens' place
        memory[index + 1] = value % 10;
        value /= 10;

        // hundreds' place
        memory[index] = value % 10;
	}

	// FX55 - store registers V0 through VX in memory starting from index
	void OP_FX55() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;

		for (uint8_t i = 0; i <= VX; ++i) {
			memory[index + i] = registers[i];
		}
	}

	// FX65 - store registers V0 through VX in memory starting from index
	void OP_FX65() {
		uint8_t VX = (opcode & 0x0F00u) >> 8u;

		for (uint8_t i = 0; i <= VX; ++i) {
			registers[i] = memory[index + i];
		}
	}
};

#endif // CHIP8_H
