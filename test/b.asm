        AORG >0000      ;ROM Start
        DATA >1111      ;Reset WP
        DATA VAR1

        AORG >0100
        DATA >2222
        DATA OOPS

        AORG >0200
        DATA >3333

        DORG >1000
VAR1    BSS 32
        DORG >1004
OOPS    BSS 2
        END