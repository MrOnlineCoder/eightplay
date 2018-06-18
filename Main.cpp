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
#include <iostream>

#include "Chip8.h"

int main(int argc, char* argv[]) {

	if (argc < 2) {
		std::cout << "eightplay CHIP-8 emulator by MrOnlineCoder" << std::endl << std::endl;
		std::cout << "Usage: eightplay <file>" << std::endl;
		std::cout << "- <file> - input CHIP-8 program to execute" << std::endl;
		return 0;
	}

	Chip8 chip8;

	if (!chip8.loadFromFile(std::string(argv[1]))) {
		std::cerr << "Error: failed to load file " << argv[1] << std::endl;
		return 1;
	}

	sf::RenderWindow window;
	window.create(sf::VideoMode(1024,768), "eightplay");
	window.setFramerateLimit(60);

	chip8.prepare(window);
	chip8.printMemory();

	sf::Font fnt;
	if (!fnt.loadFromFile("opensans.ttf")) {
		std::cerr << "Error: failed to load opensans font!" << std::endl;
		return 2;
	}

	chip8.errText.setFont(fnt);
	chip8.errText.setCharacterSize(21);
	chip8.errText.setString("Ready.");
	chip8.errText.setPosition(10, window.getSize().y - chip8.errText.getGlobalBounds().height - 15);

	chip8.debugText.setFont(fnt);
	chip8.debugText.setCharacterSize(18);

	chip8.updateDebugText();

	while (window.isOpen()) {
		sf::Event evt;
		while (window.pollEvent(evt)) {
			if (evt.type == sf::Event::Closed) window.close();

			if (evt.type == sf::Event::KeyReleased) {
				if (evt.key.code == sf::Keyboard::Space) {
					chip8.execute();
					chip8.updateDebugText();
				}
			}
		}

		window.clear();

		window.draw(chip8.debugText);
		window.display();
	}


	return 0;
}