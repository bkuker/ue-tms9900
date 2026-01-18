# UE TMS-9900 Homebrew Emulator

A quick and dirty emulator for [Usagi Electric](https://www.youtube.com/@UsagiElectric)'s
homebrew computer using the TI-TMS9900 cpu.

The CPU emulation and a some ancillary files are taken from the [js99er](https://github.com/Rasmus-M/js99er-angular) project by [Rasmus Moustgaard](https://github.com/Rasmus-M).

## Usage

### Assembler

This just uses [Using Dave Pitts' TI-990 Tools](https://www.cozx.com/dpitts/ti990.html), defuckulates
some linefeed stuff for windows, and then strips the aout header and splits the file into roms. If you
are using Linux, just follow the directions in Usage.txt 

`npm run asm filename.asm` will generate:
 * filename.lst, A readable listing file
 * filename.obj, an object file
 * filename.aout, an aout binary
 * **filename.rom**, the rom to load into the emulator
 * filename.a.rom and filename.b.rom, the high and low rom images.

 `npm run emu filename.rom` will run the emulator. STDIN goes to Mux0, so type away!