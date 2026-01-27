        DORG >1000      ; RAM start
OSWP    BSS 32          ; Set aside 32 bytes/16 words for primary workspace

TCBS
TCB1
        BSS 2           ;Workspace
        BSS 2           ;Program Counter
        BSS 2           ;Status
TCB2    BSS 2
        BSS 2
        BSS 2

TCUR    BSS 2
 
        AORG >0000
        DATA OSWP         
        DATA INIT           

        AORG >003C
        DATA OSWP          
        DATA TIMER        

        AORG >0100

INIT    LI 0,TCB1       ;Set up Task 1 TCB
        LI 1,T1RAM
        MOV 1, *0+
        LI 1,TOP1
        MOV 1, *0+
        LI 1,>00FF
        MOV 1,*0+

        LI 0,TCB2       ;Set up Task 1 TCB
        LI 1,T2RAM      
        MOV 1, *0+
        LI 1,TOP2
        MOV 1, *0+
        LI 1,>00FF
        MOV 1, *0+
        JMP T1

;        LIMI 15        ;Cant do this because assumes T1 is already running
;IDLE    JMP IDLE


TIMER   LIMI 0          ;INT OFF
        CLR @>F0F0      ;Clear timer int
        MOV @TCUR, 1    ;Load current task into R1
        JNE S2

S1      LI 0,TCB1       ;Store away Task 1
        MOV 13,*0+
        MOV 14,*0+
        MOV 15,*0+
T2      LI 0,TCB2       ;Load Task 2
        MOV *0+,13
        MOV *0+,14
        MOV *0+,15
        LI 0,>1
        MOV 0,@TCUR
        RTWP

S2      LI 0,TCB2       ;Store away Task 2
        MOV 13,*0+
        MOV 14,*0+
        MOV 15,*0+
        JMP T1
T1      LI 0,TCB1       ;Load Task 2
        MOV *0+,13
        MOV *0+,14
        MOV *0+,15
        LI 0,>0
        MOV 0,@TCUR
        RTWP

        DORG >2000
T1RAM   BSS 2           ;Workspace

        AORG >0200
TOP1    LI 2,STR1           ; Store the text address in R3
        LI 3,>F000          ; Store the MMIO THRE address in R3
        LI 4,>F002          ; Store the MMIO THRL address in R4

PR1     MOV *3, 1           ; Load R1 with the value pointed at by R3
        ANDI 1, >0001       ; Do an AND to eliminate everything except status
        JNE PR1             ; If equal to 0, loop back and test again
        MOV *2, 1           ; Load R1 with value pointed at by R2
        MOV 1, *4           ; Move the value into THRL

TST1    INCT 2              ; Increment R2 by a word
        MOV *2, 1           ; Load R1 with value pointed at by R2
        JEQ TOP1             ; If zero, restart the whole loop
        JMP PR1             ; Else, run the print loop again

STR1    DATA >0048          ; ASCII "H"
        DATA >0045          ; ASCII "E"
        DATA >004C          ; ASCII "L"
        DATA >004C          ; ASCII "L"
        DATA >004F          ; ASCII "O"
        DATA >0052          ; ASCII "R"
        DATA >004C          ; ASCII "L"
        DATA >0044          ; ASCII "D"
        DATA >0021          ; ASCII "!"
        DATA >000A          ; LF
        DATA >000D          ; CR
        DATA >0000          ; Null terminator

        DORG >3000
T2RAM   BSS 2           ;Workspace
        AORG >0300
TOP2    LI 2,STR2           ; Store the text address in R3
        LI 3,>F00A          ; Store the MMIO THRE address in R3
        LI 4,>F00C          ; Store the MMIO THRL address in R4

PR2     MOV *3, 1           ; Load R1 with the value pointed at by R3
        ANDI 1, >0001       ; Do an AND to eliminate everything except status
        JNE PR2             ; If equal to 0, loop back and test again
        MOV *2, 1           ; Load R1 with value pointed at by R2
        MOV 1, *4           ; Move the value into THRL

TST2    INCT 2              ; Increment R2 by a word
        MOV *2, 1           ; Load R1 with value pointed at by R2
        JEQ TOP2             ; If zero, restart the whole loop
        JMP PR2             ; Else, run the print loop again

STR2    DATA >0054          ; ASCII "T"
        DATA >004D          ; ASCII "M"
        DATA >0053          ; ASCII "S"
        DATA >002D          ; ASCII "-"
        DATA >0039          ; ASCII "9"
        DATA >0039          ; ASCII "9"
        DATA >0030          ; ASCII "0"
        DATA >0030          ; ASCII "0"
        DATA >0021          ; ASCII "!"
        DATA >000A          ; LF
        DATA >000D          ; CR
        DATA >0000          ; Null terminator

        END
