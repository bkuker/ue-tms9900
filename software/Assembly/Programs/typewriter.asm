; Interrupt is HLLL, which should be Memory Address >0020

; UART I/O Addresses
THRE EQU >F000          ; Transmitter Holding Register Empty (From UART)
THRL EQU >F002          ; Transmitter Holding Register Load (To UART)
RRD EQU >F004           ; Receiver Register Disconnect (To UART)
DRR EQU >F006           ; Data Received Reset (To UART)

; Initial Setup stuff
        LIMI 1                  ; Enable interrupts

; Reset Vector (Interrupt LLLL)
        AORG >0000
        DATA >1000              ; >1000 is the workspace pointer
        DATA >0080              ; >0080 is the start of the program

; Interrupt LVL 8 (Interrupt HLLL)
        AORG >0020
        DATA >1020              ; >1020 is the workspace pointer
        DATA >0100              ; New program counter

; Typewriter loop waiting for a character in the buffer
        AORG >0080
RXBUF   DATA >0000              ; This is the receive register buffer, 1 word
CKBUF   MOV @RXBUF, R0          ; Move it into R0
        ANDI R0, >8000          ; And with >8000 to see if MSB is set
        JNE CKBUF               ; If top bit isn't set, loop back, check again

; If set, fall through to sending the byte back out
SEND    MOV @THRE, R1           ; Load R1 with THRE status
        ANDI 1, >0001           ; AND to eliminate everything except status
        JNE SEND                ; If equal to 0, loop back and test again
        MOVB @RXBUF, @THRL      ; Put value onto bus and toggle THRL address
        CLR @RXBUF              ; Clear out the buffer
        JMP CKBUF               ; Jump back and start again

; This is the interrupt code that receives the byte and stuffs it in RXBUF
        AORG >0100
GETB    MOV RRD, R0             ; Toggle RRD, and grab byte
        ORI R0, >8000           ; Set MSB to be a flag
        MOV R0, @RXBUF          ; Copy received byte and flag to buffer
        MOV R0, @DRR            ; Reset Data Received signal on UART
        RTWP
        END
