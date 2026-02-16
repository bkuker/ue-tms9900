/***********************************************************************
*
* asmoptab.c - Opcode table for the TI 990 assembler.
*
* Changes:
*   05/21/03   DGP   Original.
*   08/13/03   DGP   Added /12 instructions and all directives.
*   05/25/04   DGP   Added Long REF/DEF pseudo ops.
*   06/07/04   DGP   Added LONG, QUAD, FLOAT and DOUBLE pseudo ops.
*   12/30/05   DGP   Changes SymNode to use flags.
*   06/28/06   DGP   Generate opcode not found errors only in pass 2.
*   05/09/09   DGP   Added opadd and opdel functions for DFOP.
*
***********************************************************************/

#include <string.h>
#include <memory.h>
#include <stdlib.h>

#include "asmdef.h"

extern int model;
extern int errcount;
extern int errnum;
extern int txmiramode;
extern SymNode *symbols[MAXSYMBOLS];
extern char errline[10][120];

OpCode optable[NUMOPS] =
{
   { "$ASG",   ASG_T,    0,      TYPE_P,   0 | SDSMAC },
   { "$CALL",  CALL_T,   0,      TYPE_P,   0 | SDSMAC },
   { "$ELSE",  ELSE_T,   0,      TYPE_P,   0 | SDSMAC },
   { "$END",   MEND_T,   0,      TYPE_P,   0 | SDSMAC },
   { "$ENDIF", ENDIF_T,  0,      TYPE_P,   0 | SDSMAC },
   { "$EXIT",  EXIT_T,   0,      TYPE_P,   0 | SDSMAC },
   { "$GOTO",  GOTO_T,   0,      TYPE_P,   0 | SDSMAC },
   { "$IF",    IF_T,     0,      TYPE_P,   0 | SDSMAC },
   { "$MACRO", MACRO_T,  0,      TYPE_P,   0 | SDSMAC },
   { "$NAME",  NAME_T,   0,      TYPE_P,   0 | SDSMAC },
   { "$SBSTG", SBSTG_T,  0,      TYPE_P,   0 | SDSMAC },
   { "$VAR",   VAR_T,    0,      TYPE_P,   0 | SDSMAC },
   { "A",      0xA000,   0,      TYPE_1,   4 },
   { "AB",     0xB000,   0,      TYPE_1,   4 },
   { "ABS",    0x0740,   0,      TYPE_6,   4 },
   { "AD",     0x0E40,   0,      TYPE_6,  12 | SDSMAC },
   { "AI",     0x0220,   0,      TYPE_8,   4 },
   { "AM",     0x002A,   0,      TYPE_11, 12 | SDSMAC },
   { "ANDI",   0x0240,   0,      TYPE_8,   4 },
   { "ANDM",   0x0028,   0,      TYPE_11, 12 | SDSMAC },
   { "AORG",   AORG_T,   0,      TYPE_P,   0 },
   { "AR",     0x0C40,   0,      TYPE_6,  12 | SDSMAC },
   { "ARJ",    0x0C0D,   0,      TYPE_17, 12 | SDSMAC },
   { "ASMELS", ASMELS_T, 0,      TYPE_P,   0 | SDSMAC },
   { "ASMEND", ASMEND_T, 0,      TYPE_P,   0 | SDSMAC },
   { "ASMIF",  ASMIF_T,  0,      TYPE_P,   0 | SDSMAC },
   { "B",      0x0440,   0,      TYPE_6,   4 },
   { "BDC",    0x0023,   0,      TYPE_11, 12 | SDSMAC },
   { "BES",    BES_T,    0,      TYPE_P,   0 },
   { "BIND",   0x0140,   0,      TYPE_6,  11 | SDSMAC },
   { "BL",     0x0680,   0,      TYPE_6,   4 },
   { "BLSK",   0x00B0,   0,      TYPE_8,  12 | SDSMAC },
   { "BLWP",   0x0400,   0,      TYPE_6,   4 },
   { "BSS",    BSS_T,    0,      TYPE_P,   0 },
   { "BYTE",   BYTE_T,   0,      TYPE_P,   0 },
   { "C",      0x8000,   0,      TYPE_1,   4 },
   { "CB",     0x9000,   0,      TYPE_1,   4 },
   { "CDE",    0x0C05,   0,      TYPE_7,  12 | SDSMAC },
   { "CDI",    0x0C01,   0,      TYPE_7,  12 | SDSMAC },
   { "CED",    0x0C07,   0,      TYPE_7,  12 | SDSMAC },
   { "CEND",   CEND_T,   0,      TYPE_P,   0 },
   { "CER",    0x0C06,   0,      TYPE_7,  12 | SDSMAC },
   { "CI",     0x0280,   0,      TYPE_8,   4 },
   { "CID",    0x0E80,   0,      TYPE_6,  12 | SDSMAC },
   { "CIR",    0x0C80,   0,      TYPE_6,  12 | SDSMAC },
   { "CKOF",   0x03C0,   0,      TYPE_7,   4 },
   { "CKON",   0x03A0,   0,      TYPE_7,   4 },
   { "CKPT",   CKPT_T,   0,      TYPE_P,   0 | SDSMAC },
   { "CLR",    0x04C0,   0,      TYPE_6,   4 },
   { "CNTO",   0x0020,   0,      TYPE_11, 12 | SDSMAC },
   { "COC",    0x2000,   0,      TYPE_3,   4 },
   { "COPY",   COPY_T,   0,      TYPE_P,   0 | SDSMAC },
   { "CRC",    0x0E20,   0,      TYPE_12, 12 | SDSMAC },
   { "CRE",    0x0C04,   0,      TYPE_7,  12 | SDSMAC },
   { "CRI",    0x0C00,   0,      TYPE_7,  12 | SDSMAC },
   { "CS",     0x0040,   0,      TYPE_12, 12 | SDSMAC },
   { "CSEG",   CSEG_T,   0,      TYPE_P,   0 },
   { "CZC",    0x2400,   0,      TYPE_3,   4 },
   { "DATA",   DATA_T,   0,      TYPE_P,   0 },
   { "DBC",    0x0024,   0,      TYPE_11, 12 | SDSMAC },
   { "DD",     0x0F40,   0,      TYPE_6,  12 | SDSMAC },
   { "DEC",    0x0600,   0,      TYPE_6,   4 },
   { "DECT",   0x0640,   0,      TYPE_6,   4 },
   { "DEF",    DEF_T,    0,      TYPE_P,   0 },
   { "DEND",   DEND_T,   0,      TYPE_P,   0 },
   { "DFOP",   DFOP_T,   0,      TYPE_P,   0 | SDSMAC },
   { "DINT",   0x002F,   0,      TYPE_7,  12 | SDSMAC },
   { "DIV",    0x3C00,   0,      TYPE_9,   4 },
   { "DIVS",   0x0180,   0,      TYPE_6,  11 | SDSMAC },
   { "DORG",   DORG_T,   0,      TYPE_P,   0 },
   { "DOUBLE", DOUBLE_T, 0,      TYPE_P,   0 | SDSMAC },
   { "DR",     0x0D40,   0,      TYPE_6,  12 | SDSMAC },
   { "DSEG",   DSEG_T,   0,      TYPE_P,   0 },
   { "DXOP",   DXOP_T,   0,      TYPE_P,   0 },
   { "EINT",   0x002E,   0,      TYPE_7,  12 | SDSMAC },
   { "EMD",    0x002D,   0,      TYPE_7,  12 | SDSMAC },
   { "END",    END_T,    0,      TYPE_P,   0 },
   { "EP",     0x03F0,   0,      TYPE_21, 12 | SDSMAC },
   { "EQU",    EQU_T,    0,      TYPE_P,   0 },
   { "EVEN",   EVEN_T,   0,      TYPE_P,   0 },
   { "FLOAT",  FLOAT_T,  0,      TYPE_P,   0 | SDSMAC },
   { "IDLE",   0x0340,   0,      TYPE_7,   4 },
   { "IDT",    IDT_T,    0,      TYPE_P,   0 },
   { "INC",    0x0580,   0,      TYPE_6,   4 },
   { "INCT",   0x05C0,   0,      TYPE_6,   4 },
   { "INSF",   0x0C10,   0,      TYPE_16, 12 | SDSMAC },
   { "INV",    0x0540,   0,      TYPE_6,   4 },
   { "IOF",    0x0E00,   0,      TYPE_15, 12 | SDSMAC },
   { "JEQ",    0x1300,   0,      TYPE_2,   4 },
   { "JGT",    0x1500,   0,      TYPE_2,   4 },
   { "JH",     0x1B00,   0,      TYPE_2,   4 },
   { "JHE",    0x1400,   0,      TYPE_2,   4 },
   { "JL",     0x1A00,   0,      TYPE_2,   4 },
   { "JLE",    0x1200,   0,      TYPE_2,   4 },
   { "JLT",    0x1100,   0,      TYPE_2,   4 },
   { "JMP",    0x1000,   0,      TYPE_2,   4 },
   { "JNC",    0x1700,   0,      TYPE_2,   4 },
   { "JNE",    0x1600,   0,      TYPE_2,   4 },
   { "JNO",    0x1900,   0,      TYPE_2,   4 },
   { "JOC",    0x1800,   0,      TYPE_2,   4 },
   { "JOP",    0x1C00,   0,      TYPE_2,   4 },
   { "LCS",    0x00A0,   0,      TYPE_18, 12 | SDSMAC },
   { "LD",     0x0F80,   0,      TYPE_6,  12 | SDSMAC },
   { "LDCR",   0x3000,   0,      TYPE_4,   4 },
   { "LDD",    0x07C0,   0,      TYPE_6,  10 },
   { "LDEF",   LDEF_T,   0,      TYPE_P,   0 | SDSMAC },
   { "LDS",    0x0780,   0,      TYPE_6,  10 },
   { "LI",     0x0200,   0,      TYPE_8,   4 },
   { "LIBIN",  LIBIN_T,  0,      TYPE_P,   0 | SDSMAC },
   { "LIBOUT", LIBOUT_T, 0,      TYPE_P,   0 | SDSMAC },
   { "LIM",    0x0070,   0,      TYPE_18, 12 | SDSMAC },
   { "LIMI",   0x0300,   1,      TYPE_8,   4 },
   { "LIST",   LIST_T,   0,      TYPE_P,   0 },
   { "LMF",    0x0320,   0,      TYPE_10, 10 },
   { "LOAD",   LOAD_T,   0,      TYPE_P,   0 },
   { "LONG",   LONG_T,   0,      TYPE_P,   0 | SDSMAC },
   { "LR",     0x0D80,   0,      TYPE_6,  12 | SDSMAC },
   { "LREF",   LREF_T,   0,      TYPE_P,   0 | SDSMAC },
   { "LREX",   0x03E0,   0,      TYPE_7,   4 },
   { "LST",    0x0080,   0,      TYPE_18, 11 | SDSMAC },
   { "LTO",    0x001F,   0,      TYPE_11, 12 | SDSMAC },
   { "LWP",    0x0090,   0,      TYPE_18, 11 | SDSMAC },
   { "LWPI",   0x02E0,   1,      TYPE_8,   4 },
   { "MD",     0x0F00,   0,      TYPE_6,  12 | SDSMAC },
   { "MOV",    0xC000,   0,      TYPE_1,   4 },
   { "MOVA",   0x002B,   0,      TYPE_19, 12 | SDSMAC },
   { "MOVB",   0xD000,   0,      TYPE_1,   4 },
   { "MOVS",   0x0060,   0,      TYPE_12, 12 | SDSMAC },
   { "MPY",    0x3800,   0,      TYPE_9,   4 },
   { "MPYS",   0x01C0,   0,      TYPE_6,  11 | SDSMAC },
   { "MR",     0x0D00,   0,      TYPE_6,  12 | SDSMAC },
   { "MVSK",   0x00D0,   0,      TYPE_12, 12 | SDSMAC },
   { "MVSR",   0x00C0,   0,      TYPE_12, 12 | SDSMAC },
   { "NEG",    0x0500,   0,      TYPE_6,   4 },
   { "NEGD",   0x0C03,   0,      TYPE_7,  12 | SDSMAC },
   { "NEGR",   0x0C02,   0,      TYPE_7,  12 | SDSMAC },
   { "NOP",    NOP_T,    0x1000, TYPE_P,   0 },
   { "NRM",    0x0C08,   0,      TYPE_11, 12 | SDSMAC },
   { "OPTION", OPTION_T, 0,      TYPE_P,   0 | SDSMAC },
   { "ORI",    0x0260,   0,      TYPE_8,   4 },
   { "ORM",    0x0027,   0,      TYPE_11, 12 | SDSMAC },
   { "PAGE",   PAGE_T,   0,      TYPE_P,   0 },
   { "PEND",   PEND_T,   0,      TYPE_P,   0 },
   { "POPS",   0x00E0,   0,      TYPE_12, 12 | SDSMAC },
   { "PSEG",   PSEG_T,   0,      TYPE_P,   0 },
   { "PSHS",   0x00F0,   0,      TYPE_12, 12 | SDSMAC },
   { "QUAD",   QUAD_T,   0,      TYPE_P,   0 | SDSMAC },
   { "REF",    REF_T,    0,      TYPE_P,   0 },
   { "RORG",   RORG_T,   0,      TYPE_P,   0 },
   { "RSET",   0x0360,   0,      TYPE_7,   4 },
   { "RT",     RT_T,     0x045B, TYPE_P,   0 },
   { "RTO",    0x001E,   0,      TYPE_11, 12 | SDSMAC },
   { "RTWP",   0x0380,   0,      TYPE_7,   4 },
   { "S",      0x6000,   0,      TYPE_1,   4 },
   { "SB",     0x7000,   0,      TYPE_1,   4 },
   { "SBO",    0x1D00,   1,      TYPE_2,   4 },
   { "SBZ",    0x1E00,   1,      TYPE_2,   4 },
   { "SD",     0x0EC0,   0,      TYPE_6,  12 | SDSMAC },
   { "SEQB",   0x0050,   0,      TYPE_12, 12 | SDSMAC },
   { "SETMNL", SETMNL_T, 0,      TYPE_P,   0 | SDSMAC },
   { "SETO",   0x0700,   0,      TYPE_6,   4 },
   { "SETRM",  SETRM_T,  0,      TYPE_P,   0 | SDSMAC },
   { "SLA",    0x0A00,   0,      TYPE_5,   4 },
   { "SLAM",   0x001D,   0,      TYPE_13, 12 | SDSMAC },
   { "SLSL",   0x0021,   0,      TYPE_20, 12 | SDSMAC },
   { "SLSP",   0x0022,   0,      TYPE_20, 12 | SDSMAC },
   { "SM",     0x0029,   0,      TYPE_11, 12 | SDSMAC },
   { "SNEB",   0x0E10,   0,      TYPE_12, 12 | SDSMAC },
   { "SOC",    0xE000,   0,      TYPE_1,   4 },
   { "SOCB",   0xF000,   0,      TYPE_1,   4 },
   { "SR",     0x0CC0,   0,      TYPE_6,  12 | SDSMAC },
   { "SRA",    0x0800,   0,      TYPE_5,   4 },
   { "SRAM",   0x001C,   0,      TYPE_13, 12 | SDSMAC },
   { "SRC",    0x0B00,   0,      TYPE_5,   4 },
   { "SREF",   SREF_T,   0,      TYPE_P,   0 },
   { "SRJ",    0x0C0C,   0,      TYPE_17, 12 | SDSMAC },
   { "SRL",    0x0900,   0,      TYPE_5,   4 },
   { "STCR",   0x3400,   0,      TYPE_4,   4 },
   { "STD",    0x0FC0,   0,      TYPE_6,  12 | SDSMAC },
   { "STPC",   0x0030,   0,      TYPE_18, 12 | SDSMAC },
   { "STR",    0x0DC0,   0,      TYPE_6,  12 | SDSMAC },
   { "STST",   0x02C0,   2,      TYPE_8,   4 },
   { "STWP",   0x02A0,   2,      TYPE_8,   4 },
   { "SWPB",   0x06C0,   0,      TYPE_6,   4 },
   { "SWPM",   0x0025,   0,      TYPE_11, 12 | SDSMAC },
   { "SZC",    0x4000,   0,      TYPE_1,   4 },
   { "SZCB",   0x5000,   0,      TYPE_1,   4 },
   { "TB",     0x1F00,   1,      TYPE_2,   4 },
   { "TCMB",   0x0C0A,   0,      TYPE_14, 12 | SDSMAC },
   { "TEXT",   TEXT_T,   0,      TYPE_P,   0 },
   { "TITL",   TITL_T,   0,      TYPE_P,   0 },
   { "TMB",    0x0C09,   0,      TYPE_14, 12 | SDSMAC },
   { "TS",     0x0E30,   0,      TYPE_12, 12 | SDSMAC },
   { "TSMB",   0x0C0B,   0,      TYPE_14, 12 | SDSMAC },
   { "UNL",    UNL_T,    0,      TYPE_P,   0 },
   { "WPNT",   WPNT_T,   0,      TYPE_P,   0 | SDSMAC },
   { "X",      0x0480,   0,      TYPE_6,   4 },
   { "XF",     0x0C30,   0,      TYPE_16, 12 | SDSMAC },
   { "XIT",    0x0C0E,   0,      TYPE_7,  12 | SDSMAC },
   { "XOP",    0x2C00,   0,      TYPE_9,   4 },
   { "XOR",    0x2800,   0,      TYPE_3,   4 },
   { "XORM",   0x0026,   0,      TYPE_11, 12 | SDSMAC },
   { "XV",     0x0C20,   0,      TYPE_16, 12 | SDSMAC },
   { "XVEC",   XVEC_T,   0,      TYPE_P,   0 | SDSMAC }
};

/*
** Added opcodes
*/

extern int opdefcount;		/* Number of user defined opcodes */
extern OpDefCode *opdefcode[MAXDEFOPS];/* The user defined opcode table */

static char retopcode[MAXSYMLEN]; /* Return opcode symbol buffer */
static OpCode retop;		/* Return opcode buffer */
static int erremit;

/***********************************************************************
* addlookup - Lookup added opcode. Uses binary search.
***********************************************************************/

static OpCode *
addlookup (char *op)
{
   OpCode *ret = NULL;
   int done = FALSE;
   int mid;
   int last = 0;
   int lo;
   int up;
   int r;

#ifdef DEBUGOP
   fprintf (stderr, "addlookup: Entered: op = %s\n", op);
#endif

   lo = 0;
   up = opdefcount;
   
   while (opdefcount && !done)
   {
      mid = (up - lo) / 2 + lo;
#ifdef DEBUGOP
      fprintf (stderr, " mid = %d, last = %d\n", mid, last);
#endif
      if (opdefcount == 1) done = TRUE;
      else if (last == mid) break;
         
      r = strcmp (opdefcode[mid]->opcode, op);
      if (r == 0)
      {
	 strcpy (retopcode, op);
	 retop.opcode = retopcode;
	 retop.opvalue = opdefcode[mid]->opvalue;
	 retop.opmod = opdefcode[mid]->opmod;
	 retop.optype = opdefcode[mid]->optype;
	 retop.cputype = opdefcode[mid]->cputype;
	 ret = &retop;
	 done = TRUE;
      }
      else if (r < 0)
      {
	 lo = mid;
      }
      else 
      {
	 up = mid;
      }
      last = mid;
   }

#ifdef DEBUGOP
   fprintf (stderr, " ret = %8x\n", ret);
#endif
   return (ret);
}

/***********************************************************************
* stdlookup - Lookup standard opcode. Uses binary search.
***********************************************************************/

static OpCode *
stdlookup (char *op, int pass)
{
   OpCode *ret = NULL;
   int done = FALSE;
   int mid;
   int last = 0;
   int lo;
   int up;
   int r;
   char errtmp[MAXLINE];

#ifdef DEBUGOP
   printf ("stdlookup: Entered: op = %s\n", op);
#endif

   lo = 0;
   up = NUMOPS;
   
   while (!done)
   {
      mid = (up - lo) / 2 + lo;
#ifdef DEBUGOP
      printf (" mid = %d, last = %d\n", mid, last);
#endif
      if (last == mid) break;
      r = strcmp (optable[mid].opcode, op);
      if (r == 0)
      {
	 if (model >= (optable[mid].cputype & CPUMASK))
	 {
	    if (txmiramode && (optable[mid].cputype & SDSMAC))
	    {
	       if (pass == 2)
	       {
		  sprintf (errtmp, "Opcode %s not supported in TXMIRA mode",
		  	   op);
		  logerror (errtmp);
		  erremit = TRUE;
	       }
	    }
	    else
	    {
	       ret = &optable[mid];
	    }
	 }
	 else if (pass == 2)
	 {
	    if (txmiramode && (optable[mid].cputype & SDSMAC))
	    {
	       sprintf (errtmp, "Opcode %s not supported in TXMIRA mode", op);
	       logerror (errtmp);
	    }
	    sprintf (errtmp, "Opcode %s not supported on 990/%d CPU",
		     op, model);
	    logerror (errtmp);
	    erremit = TRUE;
	 }
	 return (ret);
      }
      else if (r < 0)
      {
         lo = mid;
      }
      else 
      {
         up = mid;
      }
      last = mid;
   }

   return (NULL);
}

/***********************************************************************
* oplookup - Lookup opcode.
***********************************************************************/

OpCode *
oplookup (char *op, int pass)
{
   OpCode *ret = NULL;
   char temp[20];
   char errtmp[MAXLINE];

#ifdef DEBUGOP
   printf ("oplookup: Entered: op = %s, pass = %d\n", op, pass);
#endif

   /*
   ** Check added opcodes first, incase of override
   */

   erremit = FALSE;
   if (!(ret = addlookup (op)))
   {
      /*
      ** Check standard opcode table
      */

      if (!(ret = stdlookup (op, pass)))
      {
	 SymNode *s;

	 /*
	 ** If not found, check if DXOP defined
	 */

	 sprintf (temp, "!!%s", op);
	 if ((s = symlookup (temp, FALSE, TRUE)) != NULL)
	 {
	    if (s->flags & XOPSYM)
	    {
	       strcpy (retopcode, op);
	       retop.opcode = retopcode;
	       retop.opvalue = 0x2C00 | (s->value << 6); 
	       retop.opmod = 0;
	       retop.optype = TYPE_6;
	       retop.cputype = 0;
	       ret = &retop;
	    }
	 }
      }
   }

   if (ret == NULL && pass == 2 && !erremit)
   {
      sprintf (errtmp, "Undefined opcode: %s", op);
      logerror (errtmp);
   }
#ifdef DEBUGOP
   fprintf (stderr, " ret = %8x\n", ret);
#endif
   return (ret);
}

/***********************************************************************
* opadd - Add opcode.
***********************************************************************/

void
opadd (OpCode *op, char *newopcode)
{
   OpDefCode *new;
   int lo, up;

#ifdef DEBUGOP
   fprintf (stderr,
	    "opadd: Entered: newopcode = %s\n", newopcode);
#endif

   /*
   ** Allocate storage for opcode and fill it in
   */

   if (opdefcount+1 > MAXDEFOPS)
   {
      fprintf (stderr, "asm990: DFOP Code table exceeded\n");
      exit (ABORT);
   }

   if ((new = (OpDefCode *)malloc (sizeof (OpDefCode))) == NULL)
   {
      fprintf (stderr, "asm990: Unable to allocate memory\n");
      exit (ABORT);
   }

   memset (new, '\0', sizeof (OpDefCode));
   strcpy (new->opcode, newopcode);
   new->opvalue = op->opvalue;
   new->opmod = op->opmod;
   new->optype = op->optype;
   new->cputype = op->cputype;

   if (opdefcount == 0)
   {
      opdefcode[0] = new;
      opdefcount++;
      return;
   }

   /*
   ** Insert pointer in sort order.
   */

   for (lo = 0; lo < opdefcount; lo++)
   {
      if (strcmp (opdefcode[lo]->opcode, newopcode) > 0)
      {
	 for (up = opdefcount + 1; up > lo; up--)
	 {
	    opdefcode[up] = opdefcode[up-1];
	 }
	 opdefcode[lo] = new;
	 opdefcount++;
	 return;
      }
   }
   opdefcode[opdefcount] = new;
   opdefcount++;
}

/***********************************************************************
* opdel - Delete an op code from the table.
***********************************************************************/

void
opdel (char *op)
{
   int i;

#ifdef DEBUGOP
   fprintf (stderr, "opdel: Entered: op = %s\n", op);
#endif
   for (i = 0; i < opdefcount; i++)
   {
      if (!strcmp (opdefcode[i]->opcode, op))
      {
         free (opdefcode[i]);
	 for (; i < opdefcount; i++)
	 {
	    opdefcode[i] = opdefcode[i+1];
	 }
	 opdefcount --;
         return;
      }
   }
}
