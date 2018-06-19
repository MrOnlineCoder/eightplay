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
#include <bitset>

//Thanks @fallahn for this snippet! 
std::default_random_engine rndEngine(static_cast<unsigned long>(std::time(0)));
std::uniform_int_distribution<unsigned short> distribution(0, 0xFF);
sf::Uint16 randNext() { return distribution(rndEngine); }


Chip8::Chip8() {
	pc = CHIP8_PROGRAM_START;
	indexRegister = 0;

	delayTimer = 0;
	soundTimer = 0;

	error = false;

	clearScreen();

	kbdmap[0x1] = sf::Keyboard::Key::Num1;
	kbdmap[0x2] = sf::Keyboard::Key::Num2;
	kbdmap[0x3] = sf::Keyboard::Key::Num3;
	kbdmap[0xC] = sf::Keyboard::Key::Num4;

	kbdmap[0x4] = sf::Keyboard::Key::Q;
	kbdmap[0x5] = sf::Keyboard::Key::W;
	kbdmap[0x6] = sf::Keyboard::Key::E;
	kbdmap[0xD] = sf::Keyboard::Key::R;

	kbdmap[0x7] = sf::Keyboard::Key::A;
	kbdmap[0x8] = sf::Keyboard::Key::S;
	kbdmap[0x9] = sf::Keyboard::Key::D;
	kbdmap[0xE] = sf::Keyboard::Key::F;

	kbdmap[0xA] = sf::Keyboard::Key::Z;
	kbdmap[0x0] = sf::Keyboard::Key::X;
	kbdmap[0xB] = sf::Keyboard::Key::C;
	kbdmap[0xF] = sf::Keyboard::Key::V;

	running = true;

	cycles = CHIP8_DEFAULT_CYCLES;
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

	data.insert(data.begin(),
		std::istream_iterator<sf::Uint8>(file),
		std::istream_iterator<sf::Uint8>());

	return true;
}

void Chip8::loadFromMemory(const sf::Uint8 * mem, std::size_t sz) {
	data.clear();
	data.reserve(sz);
	for (std::size_t i = 0; i < sz; i++) {
		data.push_back(mem[i]);
	}
}

void Chip8::prepare(sf::RenderWindow & target) {
	this->window = &target;

	std::memcpy(memory.data(), fontset.data(), fontset.size());

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
		clearScreen();
		advance(2);
		return;
	}

	/*
	00EE - RET
	Return from a subroutine.
	*/
	if (opcode == Chip8Opcodes::Return) {
		pc = pop();
		advance(2);
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

	/*
	Dxyn - DRW Vx, Vy, nibble
	Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.

	The interpreter reads n bytes from memory, starting at the address stored in I. 
	These bytes are then displayed as sprites on screen at coordinates (Vx, Vy). 
	Sprites are XORed onto the existing screen. 
	If this causes any pixels to be erased, VF is set to 1, otherwise it is set to 0. 
	If the sprite is positioned so part of it is outside the coordinates of the display, 
	it wraps around to the opposite side of the screen. 
	*/

	if (optype == Chip8Opcodes::DrawSprite) {
		auto n = opcode & 0x000F;
		auto regx = (opcode & 0x0F00) >> 8;
		auto regy = (opcode & 0x00F0) >> 4;

		auto x = registers[regx];
		auto y = registers[regy];

		for (int i = 0; i < n; ++i) {
			sf::Uint8 a = memory[indexRegister + i];

			for (int col = 0; col < 8; ++col) {
				bool bitValue = a & (0x80 >> col);

				int sx = (x + col) % CHIP8_SCREEN_WIDTH;
				int sy = (y + i) % CHIP8_SCREEN_HEIGHT;

				if (screen[sx][sy] == 1) registers[CARRY_REGISTER] = 1;

				if (bitValue) {
					screen[sx][sy] = screen[sx][sy] ^ 1;
				}
			}
		}
		advance(2);
		return;
	}

	/*
	Ex9E - SKP Vx
	Skip next instruction if key with the value of Vx is pressed.

	Checks the keyboard, and if the key corresponding to the value of Vx is currently in the down position, 
	PC is increased by 2.
	*/
	if ((opcode & 0xF0FF) == Chip8Opcodes::SkipIfKeyIsPressed) {
		auto reg = (opcode & 0x0F00) >> 8;

		if ((inputMask & (1 << registers[reg])) != 0) {
			advance(4);
		} else {
			advance(2);
		}
		return;
	}

	/*
	ExA1 - SKNP Vx
	Skip next instruction if key with the value of Vx is not pressed.

	Checks the keyboard, and if the key corresponding to the value of Vx is currently in the up position, PC is increased by 2.
	*/

	if ((opcode & 0xF0FF) == Chip8Opcodes::SkipIfKeyIsNotPressed) {
		auto reg = (opcode & 0x0F00) >> 8;

		if ((inputMask & (1 << registers[reg])) == 0) {
			advance(4);
		}
		else {
			advance(2);
		}
		return;
	}

	/*
	Fx07 - LD Vx, DT
	Set Vx = delay timer value.

	The value of DT is placed into Vx.
	*/

	if ((opcode & 0xF0FF) == Chip8Opcodes::GetDelayTimerValue) {
		auto reg = (opcode & 0x0F00) >> 8;

		registers[reg] = delayTimer;
		advance(2);
		return;
	}

	/*
	Fx0A - LD Vx, K
	Wait for a key press, store the value of the key in Vx.

	All execution stops until a key is pressed, then the value of that key is stored in Vx.
	*/

	if ((opcode & 0xF0FF) == Chip8Opcodes::WaitKeyPress) {
		auto reg = (opcode & 0x0F00) >> 8;

		for (auto i = 0; i < CHIP8_KBD_SIZE; ++i) {
			if (inputMask & (1 << i)) {
				registers[reg] = i;
				advance(2);
				return;
			}
		}

		return;
	}

	/*
	Fx15 - LD DT, Vx
	Set delay timer = Vx.

	DT is set equal to the value of Vx.
	*/
	if ((opcode & 0xF0FF) == Chip8Opcodes::SetDelayTimer) {
		auto reg = (opcode & 0x0F00) >> 8;

		delayTimer = registers[reg];
		advance(2);

		return;
	}

	/*
	Fx18 - LD ST, Vx
	Set sound timer = Vx.

	ST is set equal to the value of Vx.
	*/
	if ((opcode & 0xF0FF) == Chip8Opcodes::SetSoundTimer) {
		auto reg = (opcode & 0x0F00) >> 8;

		soundTimer = registers[reg];
		advance(2);

		return;
	}

	/*
	Fx1E - ADD I, Vx
	Set I = I + Vx.

	The values of I and Vx are added, and the results are stored in I.
	*/
	if ((opcode & 0xF0FF) == Chip8Opcodes::IndexAdd) {
		auto reg = (opcode & 0x0F00) >> 8;

		indexRegister = indexRegister + registers[reg];
		advance(2);

		return;
	}

	/*
	Fx29 - LD F, Vx
	Set I = location of sprite for digit Vx.

	The value of I is set to the location for the hexadecimal sprite corresponding to the value of Vx.
	*/
	if ((opcode & 0xF0FF) == Chip8Opcodes::IndexSetFont) {
		auto reg = (opcode & 0x0F00) >> 8;

		indexRegister = registers[reg] * 0x5;
		advance(2);

		return;
	}

	/*
	Fx33 - LD B, Vx
	Store BCD representation of Vx in memory locations I, I+1, and I+2.

	The interpreter takes the decimal value of Vx, 
	and places the hundreds digit in memory at location in I, 
	the tens digit at location I+1, 
	and the ones digit at location I+2.
	*/
	if ((opcode & 0xF0FF) == Chip8Opcodes::IndexBCD) {
		auto reg = (opcode & 0x0F00) >> 8;

		auto val = registers[reg];

		auto hunderds = val / 100;
		auto tens = (val / 10) % 10;
		auto ones = (val % 100) % 10;

		memory[indexRegister] = hunderds;
		memory[indexRegister + 1] = tens;
		memory[indexRegister + 2] = ones;

		advance(2);
		return;
	}

	/*
	Fx55 - LD [I], Vx
	Store registers V0 through Vx in memory starting at location I.

	The interpreter copies the values of registers V0 through Vx into memory, starting at the address in I.
	*/
	if ((opcode & 0xF0FF) == Chip8Opcodes::RegistersToMemory) {
		auto x = (opcode & 0x0F00) >> 8;

		for (int i = 0; i <= x; i++) {
			memory[indexRegister + i] = registers[i];
		}

		advance(2);
		return;
	}

	/*
	Fx65 - LD Vx, [I]
	Read registers V0 through Vx from memory starting at location I.

	The interpreter reads values from memory starting at location I into registers V0 through Vx.
	*/
	if ((opcode & 0xF0FF) == Chip8Opcodes::MemoryToRegisters) {
		auto x = (opcode & 0x0F00) >> 8;

		for (int i = 0; i <= x; i++) {
			registers[i] = memory[indexRegister + i];
		}

		advance(2);
		return;
	}

	unknownOpcode(opcode);
	return;
}

void Chip8::update() {
	if (!running) return;

	execute();
	updateDebugText();

	if (delayTimer > 0) {
		if (delayClock.getElapsedTime().asMilliseconds() > 1000 / CHIP8_CLOCK_SPEED) {
			delayTimer--;
			delayClock.restart();
		}
	}
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

void Chip8::setRunning(bool arg) {
	running = arg;
}

bool Chip8::isRunning() {
	return running;
}

void Chip8::updateDebugText() {
	Opcode oc = memory[pc] << 8 | memory[pc + 1];

	std::stringstream sstream;

	sstream << "Program counter: " << pc << std::hex << " (0x" << pc << ")" << std::dec << " Stack pointer: " << sp << " Index register: " << indexRegister 
		<< " Input mask: " << std::bitset<16>(inputMask) << " Next opcode: " << std::hex << oc << "\n";
	sstream << "Registers: ";

	for (int r = 0; r < CHIP8_REGISTERS; r++) {
		sstream << "V" << r << "=" << std::hex << registers[r] << " ";

		if (r == 8) sstream << "\n";
	}

	sstream << "\nStack:\n";
	for (int i = 0; i < stack.size(); i++) {
		if (stack[i] != 0x0) sstream << std::hex << "0x" << stack[i] << "\n";
	}

	debugText.setString(sstream.str());
	debugText.setPosition(10, window->getSize().y - debugText.getGlobalBounds().height - 15);
}

void Chip8::processKeyPress(sf::Event & ev) {
	for (int i = 0; i < CHIP8_KBD_SIZE; i++) {
		if (ev.key.code == kbdmap[i]) {
			inputMask |= (1 << i);
			return;
		}
	}
}

void Chip8::processKeyRelease(sf::Event & ev) {
	for (int i = 0; i < CHIP8_KBD_SIZE; i++) {
		if (ev.key.code == kbdmap[i]) {
			inputMask &= ~(1 << i);
			return;
		}
	}
}

void Chip8::setCycles(int perSecond) {
	if (perSecond <= 0) {
		running = false;
		error = true;
		errText.setString("Manual mode. Press F2 for next opcode");
		return;
	}

	cycles = perSecond;
}

int Chip8::getCycles() {
	return cycles;
}

void Chip8::advance(int a) {
	pc += a;

	if (pc >= memory.size()) {
		errText.setString("Out of memory.");
		running = false;
		return;
	}
}

void Chip8::push(sf::Uint16 value) {
	stack[sp++] = value;
}

sf::Uint16 Chip8::pop() {
	return stack[--sp];
}

void Chip8::unknownOpcode(sf::Uint16 opcode) {
	setRunning(false);

	error = true;

	std::stringstream ss;

	ss << "Unknown opcode: 0x" << std::hex << std::uppercase << opcode << "\n";

	errText.setString(ss.str());
}

void Chip8::clearScreen() {
	for (int x = 0; x < CHIP8_SCREEN_WIDTH; x++) {
		for (int y = 0; y < CHIP8_SCREEN_HEIGHT; y++) {
			screen[x][y] = 0;
		}
	}
}
