        AORG >0000      ;ROM Start
        DATA >1000      ;Reset WP
        DATA INIT       ;Reset PC

        AORG >0100
INIT    B INIT          ;GOTO INIT

        AORG >1000      ;RAM Start
OSWP    BSS 32          ;Workspace
VAR1    BSS 2           ;A Variable
        END