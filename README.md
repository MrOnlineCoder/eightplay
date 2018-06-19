# eightplay
Simple CHIP-8 interpreter in C++

## Building
eightplay uses SFML for rendering display and showing debug info. Download latest SFML binaries and libs from [here](https://www.sfml-dev.org/download.php), and put content of SFML's lib folder into **thirdparty/SFML/lib** folder.

Then just open the solution file and you are ready to build. If build fails, setup [SFML manually](https://www.sfml-dev.org/tutorials/2.5/start-vc.php).

## Usage
```bash
eightplay <file> [speed]
```

where `file` is path to CHIP-8 ROM.
`speed` is the speed of emulator (instructions / second). **Optional**. If not specified, default value of 300 is used.
Set to 0 to enable **manual mode** - you have to run each next instruction by pressing F2.

## Thanks to:
[fallahn](https://github.com/fallahn/)

Cowgod for writing nice CHIP-8 reference: [link](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM)
