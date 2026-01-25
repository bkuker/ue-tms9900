        AORG >1000      ;RAM Start
OSWP    BSS 32          ;Workspace
VAR1    DATA 0          ;A Variable
        EORG

        AORG >0000      ;ROM Start
        DATA >1000      ;Reset WP
        DATA INIT       ;Reset PC

        AORG >0100
INIT    B INIT          ;GOTO INIT
        END