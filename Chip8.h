/*
	eightplay CHIP-8 emulator

	github.com/MrOnlineCoder/eightplay

	MIT License

	Copyright (c) 2018 Nikita Kogut (MrOnlineCoder)

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#ifndef CHIP8_H
#define CHIP8_H

#include <vector>
#include <SFML/Graphics.hpp>
#include <array>

const unsigned int CHIP8_MEMORY_SIZE = 4096u;
const unsigned int CHIP8_PROGRAM_START = 0x200;
const unsigned int CHIP8_STACK_SIZE = 16;
const unsigned int CHIP8_REGISTERS = 16;
const unsigned int CARRY_REGISTER = CHIP8_REGISTERS - 1;

const int CHIP8_SCREEN_WIDTH = 64;
const int CHIP8_SCREEN_HEIGHT = 32;


typedef sf::Uint16 Opcode;

namespace Chip8Opcodes {
	const Opcode ClearScreen = 0x00E0;
	const Opcode Return = 0x00EE;

	const Opcode Jump = 0x1000;
	const Opcode SubroutineCall = 0x2000;
	
	const Opcode SkipIfEqual = 0x3000;
	const Opcode SkipIfNotEqual = 0x4000;
	const Opcode SkipIfRegistersEqual = 0x5000;

	const Opcode SetRegister = 0x6000;
	const Opcode RegisterAdd = 0x7000;

	const Opcode AssignRegisters = 0x8000;

	const Opcode BitwiseOr = 0x8001;
	const Opcode BitwiseAnd = 0x8002;
	const Opcode BitwiseXor = 0x8003;

	const Opcode AddRegisterAndSetCarry = 0x8004;
	const Opcode SubtractRegisterAndSetCarry = 0x8005;

	const Opcode DivideLSB = 0x8006;

	const Opcode SubtractRegisterAndSetCarryYX = 0x8007;

	const Opcode MultiplyMSB = 0x800E;

	const Opcode SkipIfRegistersNotEqual = 0x9000;

	const Opcode SetIndexRegister = 0xA000;
	
	const Opcode SetProgramCounterPlusV0 = 0xB000;

	const Opcode GenRandom = 0xC000;
};

class Chip8 {
public:
	Chip8();
	bool loadFromFile(const std::string& filename);

	void prepare(sf::RenderWindow& target);
	void execute();

	void printData();
	void printMemory();

	void updateDebugText();

	sf::Text errText;
	sf::Text debugText;
private:
	sf::RenderWindow* window;

	unsigned int pc; //program counter, or instruction pointer
	sf::Uint16 sp; //stack pointer

	void advance(int a);

	void push(sf::Uint16 value);
	sf::Uint16 pop();

	std::array<sf::Uint8, CHIP8_MEMORY_SIZE> memory;
	
	std::array<sf::Uint16, CHIP8_STACK_SIZE> stack;

	std::array<sf::Uint16, CHIP8_REGISTERS> registers;
	sf::Uint16 indexRegister;

	std::vector<sf::Uint8> data; //raw data loaded from ROM file
};

#endif