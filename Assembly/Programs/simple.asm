; Reset Vector (Interrupt LLLL)
        RORG >0000
        DATA >1000      ; >1000 is the workspace pointer
        DATA >0080      ; >0080 is the start of the program

; Interrupt LVL 8 (Interrupt HLLL)
        RORG >0020
        DATA >1020      ; >1020 is the workspace pointer
        DATA >0100      ; Where the typewriter program lives

; UART I/O Addresses
THRE    EQU >F000
THRL    EQU >F002
RRD     EQU >F004
DRR     EQU >F006

; A little something to burn time while waiting for IRQ
        RORG >0080
        LIMI 10         ; Enable all interrupts from level 0 to 10
LOOP    JMP LOOP        ; Jump to self

; Typewriter program starts here
        RORG >0100
GETB    MOV @RRD, R1    ; Enable receiver register and copy data bus into R1
        MOV R0, @DRR    ; Toggle Data Received Reset to clear interrupt
SEND    MOV @THRE, R2   ; Load R2 Transmitter Holding Register Empty status
        ANDI R2, >0001  ; Do an AND to eliminate everything except status
        JNE SEND        ; If equal to 0, loop back and test again
        MOV R1, @THRL   ; Put value onto data bus and toggle THRL address
        RTWP
        END
