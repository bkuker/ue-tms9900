       TITL 'TYPEWRIT'
********************************************************************************
*       Define start of RAM and allocate storage for registers and data        *
********************************************************************************
*
        RORG >1000       ; RAM start
RSTWP   BSS 32           ; Set aside 32 bytes/16 words for primary workspace
INTWP   BSS 32           ; Set the next 32 bytes for interrupt workspace
RXBUF   DATA 0           ; RX buffer (bit 15 = 'char avail' flag, bits 0-7 char)
*
********************************************************************************
*                Define the memory mapped I/O UART Addresses                   *
********************************************************************************
*
THRE    EQU >F000
THRL    EQU >F002
RRD     EQU >F004
DRR     EQU >F006
*
********************************************************************************
*   Define the workspace and program counter for Reset and Interrupt Level 8   *
********************************************************************************
*This is the Reset Level / Primary workspace
        RORG >0000
        DATA RSTWP              ; The workspace area we defined above
        DATA >0080              ; >0080 is the start of the program
*This is the Interrupt Level 8 and its workspace
        RORG >0020
        DATA INTWP              ; The workspace area we defined above
        DATA >0100              ; The interrupt code lives here in ROM
*
********************************************************************************
*                        The primary Typewriter Loop                           *
********************************************************************************
* This loop checks the top bit of the receive buffer, if that bit is not set,
* it loops around on repeat. If it is set, it drops down, takes the byte in
* in the buffer and shoves it back out the UART serial port.
* This loop affects R0 and R1.
        RORG >0080
        CLR @RXBUF              ; Clear out the buffer
        LIMI 15                 ; Enable all interrupts from level 0 to 15

* Typewriter loop waiting for a character in the buffer
CKBUF   MOV @RXBUF, R0          ; Move it into R0
        ANDI R0, >8000          ; And with >8000 to see if MSB is set
        JEQ CKBUF               ; If top bit isn't set, loop back, check again

* If top bit IS set, fall through to sending the byte back out
SEND    MOV @THRE, R1           ; Load R1 with THRE status
        ANDI 1, >0001           ; AND to eliminate everything except status
        JNE SEND                ; If equal to 0, loop back and test again
        MOVB @RXBUF, @THRL      ; Put value onto bus and toggle THRL address
        CLR @RXBUF              ; Clear out the buffer
        JMP CKBUF               ; Jump back and start again

********************************************************************************
*                          Interrupt Level 8 Code                              *
********************************************************************************
* If a byte is received by the UART, it should set the IRQ line, and make the
* interrupt level 8. The CPU will then jump to here, where it will receive the
* byte and stuff it into RXBUF then return back to main loop.
* Affects register R0
        RORG >0100
GETB    MOV @RRD, R0            ; Toggle RRD, and grab byte
        ORI R0, >8000           ; Set MSB to be a flag
        MOV R0, @RXBUF          ; Copy received byte and flag to buffer
        MOV R0, @DRR            ; Reset Data Received signal on UART
        RTWP

* This is the end of the program.
        END
