        RORG >0000
        DATA >1000          ; >1000 is the workspace pointer
        DATA >0080          ; >0080 is the start of the program

        RORG >0080
TOP     LI 2,>0200          ; Store the text address in R3
        LI 3,>F000          ; Store the MMIO THRE address in R3
        LI 4,>F002          ; Store the MMIO THRL address in R4

PR1     MOV *3, 1           ; Load R1 with the value pointed at by R3
        ANDI 1, >0001       ; Do an AND to eliminate everything except status
        JNE PR1             ; If equal to 0, loop back and test again
        MOV *2, 1           ; Load R1 with value pointed at by R2
        MOV 1, *4           ; Move the value into THRL

TST     INCT 2              ; Increment R2 by a word
        MOV *2, 1           ; Load R1 with value pointed at by R2
        JEQ TOP             ; If zero, restart the whole loop
        JMP PR1             ; Else, run the print loop again

        RORG >0200
        DATA >0048          ; ASCII "H"
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

        END
