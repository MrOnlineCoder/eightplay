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

#include "Chip8.h"
#include <iostream>
#include <fstream>
#include <iterator>
#include <random>
#include <sstream>

//Thanks @fallahn for this short snippet! 
std::default_random_engine rndEngine(static_cast<unsigned long>(std::time(0)));
std::uniform_int_distribution<unsigned short> distribution(0, 0xFF);
sf::Uint16 randNext() { return distribution(rndEngine); }

Chip8::Chip8() {
	pc = CHIP8_PROGRAM_START;
	indexRegister = 0;
}

bool Chip8::loadFromFile(const std::string & filename) {
	std::ifstream file(filename, std::ios::binary);

	if (!file) {
		return false;
	}

	file.unsetf(std::ios::skipws);

	std::streampos fileSize;

	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	data.reserve(fileSize);

	//uhh, I do not really like how c++ reads binary files, fread is better imho
	data.insert(data.begin(),
		std::istream_iterator<sf::Uint8>(file),
		std::istream_iterator<sf::Uint8>());

	return true;
}

void Chip8::prepare(sf::RenderWindow & target) {
	this->window = &target;

	/*for (std::size_t i = 0; i < data.size(); i+=2) {
		//code.push_back((data[i] << 8) | data[i+1]);
	}*/

	std::memcpy(&memory[CHIP8_PROGRAM_START], data.data(), data.size());
	std::memset(registers.data(), 0u, CHIP8_REGISTERS);
	std::memset(stack.data(), 0u, CHIP8_STACK_SIZE);
}

void Chip8::execute() {
	Opcode opcode = memory[pc] << 8 | memory[pc + 1];
	
	sf::Uint8 byte1 = opcode & 0xFF00;
	sf::Uint8 byte2 = opcode & 0x00FF;

	sf::Uint16 optype = opcode & 0xF000;

	/*
	00E0 - CLS
	Clear the display.	
	*/
	if (opcode == Chip8Opcodes::ClearScreen) {
		errText.setString("Cleared the screen.");
		advance(2);
		return;
	}

	/*
	00EE - RET
	Return from a subroutine.
	*/
	if (opcode == Chip8Opcodes::Return) {
		pc = pop();
		return;
	}
	
	/*
	1nnn - JP addr
	Jump to location nnn.
	*/
	if (optype == Chip8Opcodes::Jump) {
		pc = opcode & 0x0FFF;
		return;
	}

	/*
	2nnn - CALL addr
	Call subroutine at nnn.
	*/
	if (optype == Chip8Opcodes::SubroutineCall) {
		push(pc);

		pc = opcode & 0x0FFF;
		return;
	}

	/*
	3xkk - SE Vx, byte
	Skip next instruction if Vx = kk.
	*/
	if (optype == Chip8Opcodes::SkipIfEqual) {
		if (registers[(opcode & 0x0F00) >> 8] == byte2) {
			advance(4);
		} else {
			advance(2);
		}
		return;
	}

	/*
	4xkk - SNE Vx, byte
	Skip next instruction if Vx != kk.
	*/
	if (optype == Chip8Opcodes::SkipIfNotEqual) {
		if (registers[(opcode & 0x0F00) >> 8] != byte2) {
			advance(4);
		}
		else {
			advance(2);
		}
		return;
	}

	/*
	5xy0 - SE Vx, Vy
	Skip next instruction if Vx = Vy.
	*/
	if (optype == Chip8Opcodes::SkipIfRegistersEqual) {
		auto reg1 = (opcode & 0x0F00) >> 8;
		auto reg2 = (opcode & 0x00F0) >> 4;

		if (registers[reg1] == registers[reg2]) {
			advance(4);
		} else {
			advance(2);
		}

		return;
	}

	/*
	6xkk - LD Vx, byte
	Set Vx = kk.
	*/
	if (optype == Chip8Opcodes::SetRegister) {
		int reg = (opcode & 0x0F00) >> 8;

		registers[reg] = byte2;

		advance(2);
		return;
	}

	/*
	7xkk - ADD Vx, byte
	Set Vx = Vx + kk.
	*/
	if (optype == Chip8Opcodes::RegisterAdd) {
		int reg = (opcode & 0x0F00) >> 8;

		registers[reg] += byte2;

		advance(2);
		return;
	}

	//Registers opcodes
	if (optype == 0x8000) {
		sf::Uint16 last = opcode & 0xF00F;

		auto regx = (opcode & 0x0F00) >> 8;
		auto regy = (opcode & 0x00F0) >> 4;

		/*
		8xy0 - LD Vx, Vy
		Set Vx = Vy.
		*/
		if (last == Chip8Opcodes::AssignRegisters) {
			registers[regx] = registers[regy];
			advance(2);
			return;
		}

		/*
		8xy1 - OR Vx, Vy
		Set Vx = Vx OR Vy.
		*/
		if (last == Chip8Opcodes::BitwiseOr) {
			registers[regx] = registers[regx] | registers[regy];
			advance(2);
			return;
		}

		/*
		8xy2 - AND Vx, Vy
		Set Vx = Vx AND Vy.
		*/
		if (last == Chip8Opcodes::BitwiseAnd) {
			registers[regx] = registers[regx] & registers[regy];
			advance(2);
			return;
		}


		/*
		8xy3 - XOR Vx, Vy
		Set Vx = Vx XOR Vy.
		*/
		if (last == Chip8Opcodes::BitwiseXor) {
			registers[regx] = registers[regx] ^ registers[regy];
			advance(2);
			return;
		}

		/*
		8xy4 - ADD Vx, Vy
		Set Vx = Vx + Vy, set VF = carry.
		*/
		if (last == Chip8Opcodes::AddRegisterAndSetCarry) {
			if (registers[regx] + registers[regy] > 255) {
				registers[CARRY_REGISTER] = 1;
			} else {
				registers[CARRY_REGISTER] = 0;
			}

			registers[regx] += registers[regy];

			advance(2);
			return;
		}

		/*
		8xy5 - SUB Vx, Vy
		Set Vx = Vx - Vy, set VF = NOT borrow.

		If Vx > Vy, then VF is set to 1, otherwise 0. Then Vy is subtracted from Vx, and the results stored in Vx.
		*/
		if (last == Chip8Opcodes::SubtractRegisterAndSetCarry) {
			if (registers[regx] > registers[regy]) {
				registers[CARRY_REGISTER] = 1;
			}
			else {
				registers[CARRY_REGISTER] = 0;
			}

			registers[regx] -= registers[regy];

			advance(2);
			return;
		}

		/*
		8xy6 - SHR Vx {, Vy}
		Set Vx = Vx SHR 1.

		If the least-significant bit of Vx is 1, then VF is set to 1, otherwise 0. Then Vx is divided by 2.
		*/
		if (last == Chip8Opcodes::DivideLSB) {
			registers[CARRY_REGISTER] = registers[regx] & 0x1;
			registers[regx] = registers[regx] >> 1;
			
			advance(2);
			return;
		}

		/*
		8xy7 - SUBN Vx, Vy
		Set Vx = Vy - Vx, set VF = NOT borrow.

		If Vy > Vx, then VF is set to 1, otherwise 0. Then Vx is subtracted from Vy, and the results stored in Vx.
		*/
		if (last == Chip8Opcodes::SubtractRegisterAndSetCarryYX) {
			registers[regx] = registers[regy] - registers[regx];
			
			if (registers[regy] > registers[regx]) {
				registers[CARRY_REGISTER] = 1;
			} else {
				registers[CARRY_REGISTER] = 0;
			}

			advance(2);
			return;
		}

		/*
		8xyE - SHL Vx {, Vy}
		Set Vx = Vx SHL 1.

		If the most-significant bit of Vx is 1, then VF is set to 1, otherwise to 0. Then Vx is multiplied by 2.
		*/
		if (last == Chip8Opcodes::MultiplyMSB) {
			registers[CARRY_REGISTER] = registers[regx] >> 7;
			registers[regx] <<= 1;

			advance(2);
			return;
		}
		return;
	}

	/*
	9xy0 - SNE Vx, Vy
	Skip next instruction if Vx != Vy.

	The values of Vx and Vy are compared, and if they are not equal, the program counter is increased by 2.
	*/
	if (optype == Chip8Opcodes::SkipIfRegistersNotEqual) {
		auto regx = (opcode & 0x0F00) >> 8;
		auto regy = (opcode & 0x00F0) >> 4;

		if (registers[regx] != registers[regy]) {
			advance(4);
		} else {
			advance(2);
		}

		return;
	}

	/*
	Annn - LD I, addr
	Set I = nnn.

	The value of register I is set to nnn.

	*/
	if (optype == Chip8Opcodes::SetIndexRegister) {
		indexRegister = opcode & 0x0FFF;
		advance(2);
		return;
	}


	/*
	Bnnn - JP V0, addr
	Jump to location nnn + V0.

	The program counter is set to nnn plus the value of V0.
	*/
	if (optype == Chip8Opcodes::SetProgramCounterPlusV0) {
		pc = (opcode & 0x0FFF) + registers[0];
		return;
	}


	/*
	Cxkk - RND Vx, byte
	Set Vx = random byte AND kk.

	The interpreter generates a random number from 0 to 255, which is then ANDed with the value kk.
	The results are stored in Vx. See instruction 8xy2 for more information on AND.
	*/
	if (optype == Chip8Opcodes::GenRandom) {
		
		int kk = opcode & 0x00FF;

		int regx = (opcode & 0x0F00) >> 8;

		registers[regx] = randNext() & kk;
		advance(2);

		return;
	}

	std::cout << "Unknown opcode: " << std::hex << opcode << std::endl;
	advance(2);
}

void Chip8::printData() {
	std::cout << std::hex;
	for (std::size_t i = 0; i < data.size(); i++) {
		std::cout << (int) data[i] << " ";
	}
}


void Chip8::printMemory() {
	std::cout << std::hex;
	for (std::size_t i = 0; i < memory.size(); i += 2) {
		std::cout << (int) memory[i] << " ";
	}
}

void Chip8::updateDebugText() {
	std::stringstream sstream;

	sstream << "Program counter: " << pc << " Stack pointer: " << sp << "\n";
	sstream << "Registers: ";

	for (int r = 0; r < CHIP8_REGISTERS; r++) {
		sstream << "V" << r << "=" << std::hex << registers[r] << "\n ";
	}

	debugText.setString(sstream.str());
	debugText.setPosition(10, window->getSize().y - debugText.getGlobalBounds().height - 15);
}

void Chip8::advance(int a) {
	pc += a;
}

void Chip8::push(sf::Uint16 value) {
	stack[sp++] = value;
}

sf::Uint16 Chip8::pop() {
	return stack[sp--];
}
