Version: 2.4.6
Date: 2022/11/17

I. Introduction

asm990 is a cross assembler for the TI 990 computer. It is compatible with the
TXMIRA assembler. Many features that are in SDSMAC are also supported.
Including: MACROS, conditional assembly, all 990/12 opcodes, COPY in source,
logical expressions, and improved arithmetic expressions with parenthesis.


II. Build asm990

Lunix/Unix:

$ make

The make attempts to figure out which system to make for using uname.

WinBlows:

$ nmake nt


III. To run asm990

$ asm990 [-options] -o file.obj file.asm

Where options are:

   -c		- "Cassette" mode. Object can be loaded using the ASR733 
                  cassette ROM loader.
   -g           - Generate symbol table in object.
   -Iincludedir - Include directory for COPY
   -l file.lst  - Generate listing to file.lst
   -m model     - Assemble for CPU model = 4, 5, 9, 10 and 12, default = 12 
   -o file.obj  - Generate object to file.obj
   -t           - TXMIRA mode, default SDSMAC mode
   -w           - Wide listing mode
   -x           - Generate cross reference


IV. Supported Pseudo Ops

IV.1 TXMIRA Mode Supported Pseudo Ops

   AORG   - Absolute ORiGin (absolute assembly).
   BES    - Block Ended by Symbol (reserve storage).
   BSS    - Block Started by Symbol (reserve storage).
   BYTE   - BYTE data initialization.
   CEND   - CSEG end.
   CSEG   - Named COMMON SEGment definition.
   DATA   - Word DATA initialization.
   DEF    - DEFine global.
   DEND   - DSEG End.
   DORG   - Dummy ORiGin (templates).
   DSEG   - Data SEGment definition.
   DXOP   - Define XOP (extended instructions).
   END    - END of assembler input.
   EQU    - EQUate symbols to values.
   EVEN   - Make pc EVEN (set to word boundary).
   IDT    - Program IDenTifier.
   LIST   - Turn LISTing on.
   LOAD   - Force LOAD a module at link time.
   NOP    - No OPeration (generates JMP $+1).
   PAGE   - Start linsting line on next PAGE.
   PEND   - PSEG end.
   PSEG   - Program SEGment definition.
   REF    - Define external REFerence.
   RORG   - Relocatable ORiGin.
   RT     - ReTurn (generates B *11).
   SREF   - Secondary REFerence.
   TEXT   - TEXT data initialization.
   TITL   - TITLe of the assembly, placed in listing header.
   UNL    - UNList, turns listing off.


IV.2 SDSMAC Mode Supported Pseudo Ops

In SDSMAC mode, the default mode, asm990 supports all TXMIRA mode pseudo ops
and the following additional pseudo ops:

   $ASG   - Assign variable in macro
   $CALL  - Call a macro
   $ELSE  - Else clause in macro if statement
   $END   - End macro definition
   $ENDIF - End of macro if statement
   $EXIT  - Exit from a macro
   $GOTO  - Goto a statement in a macro
   $IF    - If statement in macro
   $MACRO - Define macro
   $NAME  - Name a goto label in macro
   $SBSTG - Substring assignment in macro
   $VAR   - Allocate variable in macro

   ASMELS - Conditional ASSmebly ELSe
   ASMEND - Conditional ASSmebly END
   ASMIF  - Conditional ASSembly IF
   CKPT   - ChecK PoinT register, used for /12 string instructions.
   COPY   - COPY source from a file into the assembly.
   DFOP   - DeFine OPeration.
   LIBIN  - Specify Macro library.
   LIBOUT - Specify Macro library outout.
   OPTION - Listing OPTIONs.
            10     - Process 990/10 instructions.
            12     - Process 990/12 instructions.
	    BUNLST - Byte unlist.
	    DUNLST - Data unlist.
	    MUNLST - Macro unlist.
	    NOLIST - Do not generate a listing.
	    SYMT   - Generate symbol table in object.
	    TUNLST - Text unlist.
	    XREF   - Generate cross reference.
   SETMNL - SET Macro Nesting Level, default = 16.
   SETRM  - SET Right Margin.
   XVEC   - Transfer Vector.


V. Extensions

asm990 adds several extensions. These extensions are not supported in TXMIRA
mode.

V.1 Added Pseudo Ops

   DOUBLE - Four word (64 bit) DOUBLE precision floating point initialization.
   FLOAT  - Two word (32 bit) single precision FLOATing point initialization.
   LDEF   - Long DEFine allows 32 char globals.
   LONG   - Two word (32 bit) integer initialization.
   LREF   - Long REFerence allows 32 char externals.
   QUAD   - Four word (64 bit) integer initialization.


V.2 Literal support

Literals have been added for the source operands of all instructions. There are
literals for the modes as follows:

   =@     - Address of symbol literal.
   =B     - decimal Byte literal (right justified in memory word).
   =D     - Double precision floating point literal.
   =F     - single precision Floating point literal.
   =L     - decimal Long (32 bit) literal.
   =O     - Octal word literal.
   =Q     - decimal Quad (64 bit) literal.
   =W     - decimal Word literal.
   =X     - heX word literal.

Examples of use:

   MOV   =W54,R5         Get the value of 54 in to R5

   LD    =D3.1415926     Get the value of PI into the FPA
   
   AM    =Q245643,R4,8   Add 64 bit (8 byte) value to R4,R5,R6,R7

   MOV   =@ENTRY,R5      Get address of ENTRY in R5

   LI	R5,=F3.14156     Generates the literal and loads its address.

The literals are output after the END statement in an assembly.


V.3 Optional quotes on COPY directive

Added support of optional quotes on the COPY filename. For example the
following COPY directive:

   COPY   'myfile'

Is the same as:

   COPY   myfile


V.4 White space following commas

Added support to ignore white space following a comma. For example:

   MOV	@D1, R5

Is the same as:

   MOV	@D1,R5

In TXMIRA mode the appearance of a space will generate an error.


V.5 Literal symbol in Macros

Added the support of literal symbols in Macros. This is done through the
addition of the attribute $PLIT than can be tested during expansion.
For example:

   FADD	=F3.14,*4

And the source argument is refered to in the macro as AU and the destination is
AD. The paramter entries would be:

   Symbol   String   Value   Length   Attributes

   AU       =F3.14   0       6        $PCALL,$PLIT
   AD       *4       4       2        $PCALL,$PIND


VI. References

2270509-9701 990/99000 Assembly Language Reference Manual
