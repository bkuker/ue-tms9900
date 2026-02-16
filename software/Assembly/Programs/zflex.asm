; A little test program that lives at >0080 in the ROM
; It copies a small program to memory that should set MA 13
; and toggle MA12 and MA11 back and forth repeatedly
; Essentially, it jumps from >2800 to >3000 and back over and over

          RORG >0000          ; Set origin to >0000
          DATA >1000          ; Place >1000 -> This is the WP
          DATA >0080          ; Place >0080 -> This is the start of the program

          RORG >0080          ; Set origin to >0080 -> Start of program
TOP       LI 1, >0460         ; Load the branch instruction into R1
          LI 2, >3000         ; Load the branch destination into R2
          LI 3, >0460         ; Load the next branch instruction into R3
          LI 4, >2800         ; Load the branch destination into R4

          LI 5, >2800         ; Load where the values will be stored into R5
          MOV 1,*5            ; Move R1 into memory specified by value in R5
          LI 5, >2802         ; Load where the values will be stored into R5
          MOV 2,*5            ; Move R1 into memory specified by value in R5
          LI 5, >3000         ; Load where the values will be stored into R5
          MOV 3,*5            ; Move R1 into memory specified by value in R5
          LI 5, >3002         ; Load where the values will be stored into R5
          MOV 4,*5            ; Move R1 into memory specified by value in R5
          
          END