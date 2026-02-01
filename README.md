# UE TMS-9900 Homebrew Emulator

A quick and dirty emulator for [Usagi Electric](https://www.youtube.com/@UsagiElectric)'s
homebrew computer using the TI-TMS9900 cpu.

The CPU emulation and a some ancillary files are taken from the [js99er](https://github.com/Rasmus-M/js99er-angular) project by [Rasmus Moustgaard](https://github.com/Rasmus-M).

# Lessons Learned

## Dave Pitts' TI-990 Assembler and Linker

Assembler and linker. Compile and run fine under linux. Some small changed needed to compile under modern windows, find them in this repo's history.

Assembler and linker disagree on object files being binary or text with respect to linefeeds. Changes applied in this repo.

After these changes Linux and Windows results are identical.

Linker only outputs a.out format... Simple programs can just have the 16 bytes of header ripped off, but this seems to fail for more complex programs.

We suspect there is a bug in the a.out format when using AORG statements, they are not in the right place, but I wonder if it is something more complex about a.out format.

[Pitts web page](https://www.cozx.com/dpitts/ti990.html)

## Ralph Benzinger's xdt99 Assembler

These tools are written in python. The `-b` option generates binary images suitable for direct use as roms. (They still must be split into low and high byte roms for Usagi's homebrew).

RORG, AORG and DORG all seem to work properly.

**I think this is the better solution for assembly programs**

[xdt99 Github page](https://github.com/endlos99/xdt99)

## Insomnia's tms9900-gcc Compiler

I took [The docker image from Chris Cureau](https://github.com/ccureau/tms9900-gcc) and slightly modernized it. You can find it [here](https://github.com/bkuker/tms9900-gcc).

His repo was missing the actual patches from the AtariAge forum, and was using a base Centos image so old the yum steps faild when building. Make was also segfaulting sometimes for no reasona at all.

If you are not using Docker or Podman or something it's time to learn that skill. The entire gcc & binutils compile happens in a container (kind of like a VM) and none of the mess leaks out into your real computer. The same is true when you run it, source goes in, results come out, nothing gross happens.

### Bare Metal C

So you think the main function is where your program starts?

#### The Linker Script link.ld

The example I have now is suitable for writing a C program that generates a full ROM image that can be directly loaded into Usagi's homebrew.

It describes the memory layout so that the linker can put everything in the right place. It gives names to various sections, like the familiar `.text`, `.data` and `.bss`, but also some special ones like `.vectors` and `.workspace`.

It does something I don't understand the syntax for, that seems to work... Your global variable are things that live in RAM, but should be initialized before the program starts. The `.data` section is somehow defined in RAM but with a copy stored in ROM so that it can be copied from ROM to RAM at startup. The copy is not magic, explained in the **crt0.s** section.

It also defines the top of the stack, at the top of actual physical ram (no virtual memory here!). Also mentioned in the next section.

#### crt0.s

This file is a small assembly program. In the `.vectors` section it sets up your intial on-reset workspace and PC values.

Then comes the `.text` section. Forget about main, `_start` is where your code actually starts!

The current version of this code:
 * Turns off interrupts
 * Sets the stack pointer (r10) to the address defined in link.ld
 * Copies the initial values of globals from ROM to `.data` in RAM
 * Zeros out the `.bss` secion in ram
 * Calls the `main()` funciton
 * Loops forever when main returns

 #### The compiler & C code

 It's just C, hooray! And by just C I mean **just C**. You weren't expecting any libraries were you?

 ##### Problems & notes so far

  * If you do not `-fomit-frame-pointers` it generates bad code where the stack gets clobbered during function calls.
  * I am not sure why it chooses R10 for the stack pointer.
  * Optimizations are pretty basic. -O1 and -Os generate slightly different code.

  #### Makefile

  Pretty normal.