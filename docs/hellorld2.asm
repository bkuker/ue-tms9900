        RORG >1000       ; RAM start
OSWP    BSS 32           ; Set aside 32 bytes/16 words for primary workspace

T1WP    DATA 0
T1PC    DATA 0
T1ST    DATA 0

T2WP    DATA 0
T2PC    DATA 0
T2ST    DATA 0 

TCUR    DATA 0
 
        RORG >0000
        DATA >1000         
        DATA INIT           

        RORG >003C
        DATA >1000          
        DATA TIMER        

        RORG >0100
INIT    LI 0,>2000              ;Set up Task 1 TCB
        MOV 0, @T1WP
        LI 0,TOP1
        MOV 0, @T1PC
        LI 0,>00FF
        MOV 0,@T1ST

        LI 0,>3000              ;Set up Task 2 TCB
        MOV 0, @T2WP
        LI 0,TOP2
        MOV 0, @T2PC
        LI 0,>00FF
        MOV 0,@T2ST

        LIMI 0

T1      MOV @T1WP,13
        MOV @T1PC,14
        MOV @T1ST,15
        LI 0,>0
        MOV 0,@TCUR
        RTWP

T2      MOV @T2WP,13
        MOV @T2PC,14
        MOV @T2ST,15
        LI 0,>1
        MOV 0,@TCUR
        RTWP

TIMER   LIMI 0                  ;INT OFF
        CLR >F0F0            ;Clear timer int
        MOV @TCUR, 1             ;Load current task into R1
        JNE S2

S1      MOV 13,@T1WP
        MOV 14,@T1PC
        MOV 15,@T1ST
        B T2

S2      MOV 13,@T2WP
        MOV 14,@T2PC
        MOV 15,@T2ST
        B T1


        RORG >0200
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

        RORG >0300
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
