/***********************************************************************
*
* asmpass1 - Pass 1 for the TI 990 assembler.
*
* Changes:
*   05/21/03   DGP   Original.
*   07/02/03   DGP   Change hanging byte methods.
*   07/10/03   DGP   Mark inital REF as absolute.
*   07/15/03   DGP   Adjust symbol value on an EVEN.
*   08/13/03   DGP   Redo the way ops are handled.
*                    Added /12 instructions.
*   05/20/04   DGP   Added literal support.
*   05/25/04   DGP   Added Long REF/DEF pseudo ops.
*   06/07/04   DGP   Added LONG, QUAD, FLOAT and DOUBLE pseudo ops.
*   05/06/05   DGP   Fixed EVEN relocation.
*   12/26/05   DGP   Added DSEG support.
*   12/30/05   DGP   Changes SymNode to use flags.
*   07/07/06   DGP   Added external back chain to literal symbol.
*   12/04/07   DGP   Allow DSEG to be relocatable.
*   12/12/07   DGP   Fold IDT to upper case.
*   03/16/09   DGP   Correct float/double literal
*   05/28/09   DGP   Added MACRO and CSEG support.
*   06/03/09   DGP   Cleaned up C/D/P SEGS.
*   06/29/09   DGP   Do not defer on DORG_T or ASMIF_T lookup.
*   04/22/10   DGP   Correct missing operand parsing.
*   12/07/10   DGP   Added checkexpr function.
*   12/09/10   DGP   Allow REFs at end of assembly.
*   12/14/10   DGP   Added line number to temporary file.
*   01/06/11   DGP   Fixed cross reference issues.
*   01/31/11   DGP   If symbol is REFd and then later DEFd, remove EXTERNAL
*   03/03/11   DGP   Correct literal length.
*   12/29/14   DGP   Allow for nested $IF/ASMIF statements.
*   02/10/15   DGP   Fixed REF/SREF in DATA statements.
*   02/11/15   DGP   Fixed segment end check.
*   11/18/15   DGP   Added inpseg flag.
*   01/24/17   DGP   Fixed segment flags on non-relo EQU.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "asmdef.h"

extern int pc;
extern int model;
extern int inpseg;
extern int indseg;
extern int incseg;
extern int csegcnt;
extern int dseglength;
extern int rightmargin;
extern int linenum;
extern int rorgpc;
extern int symbolcount;
extern int absolute;
extern int indorg;
extern int errcount;
extern int errnum;
extern int pgmlength;
extern int codelength;
extern int txmiramode;
extern int filenum;
extern char inbuf[MAXLINE];
extern char errline[10][120];
extern char ttlbuf[TTLSIZE+2];
extern char idtbuf[IDTSIZE+2];
extern char *pscanbuf;		/* Pointer for tokenizers */

extern SymNode *symbols[MAXSYMBOLS];
extern CSEGNode csegs[MAXCSEGS];

static FILE *tmpfd;
static char cursym[MAXSYMLEN+2];

static int bytecnt;
static int litlen;		/* Length of literals */
static int inmode;
static int curcseg;		/* Current CSEG */
static int inskip;		/* In an ASMIF Skip */

static int litcount;
static char littable[MAXSYMBOLS][MAXSYMLEN+2];

/***********************************************************************
* p1literal - Process literal.
***********************************************************************/

static char *
p1literal (char *bp, int dstflg)
{
   if (!txmiramode)
   {
      char *cp;
      int  i;
      char litsym[MAXSYMLEN+2];

      pc += 2;
      bp++;
      for (cp = bp; *cp; cp++)
      {
         if (isspace(*cp) || *cp == ',')
	    break;
      }
      strncpy (litsym, bp, cp-bp);
      litsym[cp-bp] = 0;
#if defined(DEBUGLITERAL)
      printf ("p1literal: pc = >%04X, litlen = >%04X, litsym = %s\n",
	    pc-2, litlen, litsym);
#endif
      for (i = 0; i < litcount; i++)
      {
          if (!strcmp(litsym, littable[i]))
	     break;
      }
      if (i == litcount)
      {
	 strcpy (littable[i], litsym);
	 if (*bp == 'F' || *bp == 'L')
	    litlen += 4;
	 else if (*bp == 'D' || *bp == 'Q')
	    litlen += 8;
	 else
	    litlen += 2;
	 litcount++;
      }
   }
   return (bp);
}

/***********************************************************************
* p112mop - One word opcode 2 word memory operand processor.
***********************************************************************/

static void
p112mop (OpCode *op, char *bp)
{
   char *cp;
   int type;

   while (isspace (*bp)) bp++;
   cp = checkexpr (bp, &type);
   if (*bp == MEMSYM || type == MEMEXPR)
      pc += 2;
   else if (*bp == LITERALSYM)
      bp = p1literal (bp, FALSE);

   while (*bp && *bp != ',') bp++;
   if (*bp == ',')
   {
      int cmpr;

      if (op->opvalue == 0x8000 || op->opvalue == 0x9000)
	 cmpr = TRUE;
      else
	 cmpr = FALSE;
      bp++;
      while (isspace (*bp)) bp++;
      cp = checkexpr (bp, &type);
      if (*bp == MEMSYM || type == MEMEXPR)
	 pc += 2;
      else if (cmpr && *bp == LITERALSYM)
	 bp = p1literal (bp, TRUE);
   }
   pc += 2;
}

/***********************************************************************
* p111mop - One word opcode 1 word memory operand processor.
***********************************************************************/

static void
p111mop (OpCode *op, char *bp)
{
   char *cp;
   int type;

   while (isspace (*bp)) bp++;
   cp = checkexpr (bp, &type);
   if (*bp == MEMSYM || type == MEMEXPR)
      pc += 2;
   else if (*bp == LITERALSYM)
      bp = p1literal (bp, FALSE);
   pc += 2;
}

/***********************************************************************
* p111iop - One word opcode 1 word immediate operand processor.
***********************************************************************/

static void
p111iop (OpCode *op, char *bp)
{
   if (op->opmod != 2) pc += 2;
   pc += 2;
}

/***********************************************************************
* p110mop - One word opcode 0 word memory operand processor.
***********************************************************************/

static void
p110mop (OpCode *op, char *bp)
{
   pc += 2;
}

/***********************************************************************
* p122mop - Two word opcode 2 word memory operand processor.
***********************************************************************/

static void
p122mop (OpCode *op, char *bp)
{
   char *cp;
   int type;

   while (isspace (*bp)) bp++;
   cp = checkexpr (bp, &type);
   if (*bp == MEMSYM || type == MEMEXPR)
      pc += 2;
   else if (*bp == LITERALSYM)
      bp = p1literal (bp, FALSE);

   while (*bp && *bp != ',') bp++;
   if (*bp == ',')
   {
      bp++;
      while (isspace (*bp)) bp++;
      cp = checkexpr (bp, &type);
      if (*bp == MEMSYM || type == MEMEXPR)
	 pc += 2;
   }
   pc += 4;
}

/***********************************************************************
* p122cop - Two word opcode 2 word memory operand processor with condition.
***********************************************************************/

static void
p122cop (OpCode *op, char *bp)
{
   char *cp;
   int type;

   while (isspace (*bp)) bp++;
   while (*bp && *bp != ',') bp++;
   if (*bp == ',')
   {
      bp++;
      while (isspace (*bp)) bp++;
      cp = checkexpr (bp, &type);
      if (*bp == MEMSYM || type == MEMEXPR)
	 pc += 2;
      else if (*bp == LITERALSYM)
	 bp = p1literal (bp, FALSE);

      while (*bp && *bp != ',') bp++;
      if (*bp == ',')
      {
	 bp++;
	 while (isspace (*bp)) bp++;
	 cp = checkexpr (bp, &type);
	 if (*bp == MEMSYM || type == MEMEXPR)
	    pc += 2;
      }
   }
   pc += 4;
}

/***********************************************************************
* p121mop - Two word opcode 1 word memory operand processor.
***********************************************************************/

static void
p121mop (OpCode *op, char *bp)
{
   char *cp;
   int type;

   while (isspace (*bp)) bp++;
   cp = checkexpr (bp, &type);
   if (*bp == MEMSYM || type == MEMEXPR)
      pc += 2;
   else if (*bp == LITERALSYM)
      bp = p1literal (bp, FALSE);

   pc += 4;
}

/***********************************************************************
* p120mop - Two word opcode 0 word memory operand processor.
***********************************************************************/

static void
p120mop (OpCode *op, char *bp)
{
   pc += 4;
}

/***********************************************************************
* p1pop - Process Pseudo ops.
***********************************************************************/


static void
p1pop (OpCode *op, char *bp)
{
   SymNode *s;
   char *cp;
   char *token;
   int tokentype;
   int val;
   int i, j;
   int relocatable = TRUE;
   char term;
   char errortext[256];

   switch (op->opvalue)
   {

   case AORG_T:
      if (incseg)
      {
         csegs[curcseg].length = pc;
	 incseg = FALSE;
      }
      if (!absolute && !indorg) rorgpc = pc;
      bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      if (val < 0 || val > 65535)
      {
	 val = 0;
      }
      absolute = TRUE;
      relocatable = FALSE;
      indorg = FALSE;
      pc = val & 0xFFFE;
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = val;
	 s->flags &= ~RELOCATABLE;
      }
      break;

   case ASMIF_T:
      bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      inskip = if_start (val);
      break;

   case ASMELS_T:
      inskip = if_else();
      break;

   case ASMEND_T:
      inskip = if_end();
      break;

   case BES_T:
      bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      if (val < 0 || val > 65535)
      {
	 val = 0;
      }
      pc += val;
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = pc;
	 if (relocatable)
	    s->flags |= RELOCATABLE;
	 else
	    s->flags &= ~RELOCATABLE;
      }
      break;

   case BSS_T:
      bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      if (val < 0 || val > 65535)
      {
	 val = 0;
      }
      pc += val;
      break;

   case BYTE_T:
      do {
	 bp = exprscan (bp, &val, &term, &relocatable, 2, TRUE, 0);
	 bytecnt++;
	 pc++;
      } while (term == ',');
      break;

   case COPY_T:
      break;

   case DATA_T:
      if (pc & 0x0001) pc++;
#ifdef DEBUGPC
      printf ("DATA pc = %04X\n", pc);
#endif
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = pc;
      }
      do {
	 while (*bp && isspace(*bp)) bp++;
	 if (*bp == MEMSYM && !txmiramode) bp++;
	 cp = bp;
	 while (*bp != ',' && !isspace(*bp)) bp++;
	 term = *bp;
	 *bp++ = '\0';
	 pc += 2;
#ifdef DEBUGPC
	 printf ("    pc = %04X\n", pc);
#endif
      } while (term == ',');
      break;

   case DORG_T:
      if (incseg)
      {
         csegs[curcseg].length = pc;
	 incseg = FALSE;
      }
      if (!absolute && !indorg) rorgpc = pc;
      bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      if (val < 0 || val > 65535)
      {
	 val = 0;
      }
      absolute = TRUE;
      indorg = TRUE;
      pc = val & 0xFFFE;
#ifdef DEBUGDORG
      printf ("p1: DORG: pc = %04X\n", pc);
#endif
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = val;
	 s->flags &= ~RELOCATABLE;
      }
      break;

   case DOUBLE_T:
   case QUAD_T:
      if (pc & 0x0001) pc++;
#ifdef DEBUGPC
      printf ("DOUBLE/QUAD pc = %04X\n", pc);
#endif
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = pc;
      }
      do {
	 while (*bp && isspace(*bp)) bp++;
	 cp = bp;
	 while (*bp != ',' && !isspace(*bp)) bp++;
	 term = *bp;
	 *bp++ = '\0';
	 pc += 8;
#ifdef DEBUGPC
	 printf ("    pc = %04X\n", pc);
#endif
      } while (term == ',');
      break;

   case DXOP_T:
      bp = tokscan (bp, &token, &tokentype, &val, &term);
      if (tokentype == SYM)
      {
	 char temp[20];

	 sprintf (temp, "!!%s", token);
	 s = symlookup (temp, TRUE, FALSE);
	 if (term == ',')
	 {
	    bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
	    if (val < 0 || val > 15)
	    {
	       sprintf (errortext, "Invalid DXOP value: %d", val);
	       logerror (errortext);
	       val = 0;
	    }
	    s->value = val;
	    s->flags &= ~RELOCATABLE;
	    s->flags |= XOPSYM;
	 }
      }
      else
      {
	 sprintf (errortext, "Invalid DXOP symbol: %s", token);
	 logerror (errortext);
      }
      break;

   case EQU_T:
      bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
#if defined(DEBUGPC) || defined(DEBUGDORG)
      printf ("p1: EQU pc = %04X, val = %04X\n", pc, val);
#endif
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = val;
	 if (relocatable)
	    s->flags |= RELOCATABLE;
	 else
	    s->flags &= ~(RELOCATABLE | DSEGSYM | CSEGSYM);
      }
      break;

   case EVEN_T:
      if (pc & 0x0001) pc++;
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = pc;
	 if (absolute)
	    s->flags &= ~RELOCATABLE;
	 else
	    s->flags |= RELOCATABLE;
      }
      break;

   case FLOAT_T:
   case LONG_T:
      if (pc & 0x0001) pc++;
#ifdef DEBUGPC
      printf ("FLOAT/LONG pc = %04X\n", pc);
#endif
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = pc;
      }
      do {
	 while (*bp && isspace(*bp)) bp++;
	 cp = bp;
	 while (*bp != ',' && !isspace(*bp)) bp++;
	 term = *bp;
	 *bp++ = '\0';
	 pc += 4;
#ifdef DEBUGPC
	 printf ("    pc = %04X\n", pc);
#endif
      } while (term == ',');
      break;

   case IDT_T:
      if (!idtbuf[0])
      {
	 while (isspace(*bp)) bp++;
	 cp = bp;
	 if (*bp == '\'')
	 {
	    cp = ++bp;
	    while (*bp != '\'' && *bp != '\n')
	    {
	       if (islower (*bp)) *bp = toupper(*bp);
	       bp++;
	    }
	 }
	 *bp = '\0';
	 if (strlen(cp) > IDTSIZE)
	 {
	    *(cp+IDTSIZE) = '\0';
	 }
	 strcpy (idtbuf, cp);
      }
      break;

   case DEF_T:
   case LDEF_T:
      do {
	 bp = tokscan (bp, &token, &tokentype, &val, &term);
	 if (tokentype != SYM)
	 {
	    return;
	 }
	 if (strlen(token) > MAXSYMLEN)
	 {
	    token[MAXSYMLEN] = '\0';
	 }
	 if ((s = symlookup (token, FALSE, FALSE)) != NULL)
	 {
	    if (s->flags & EXTERNAL)
	       s->flags &= ~EXTERNAL;
	 }
      } while (term == ',');
      break;

   case LOAD_T:
      do {
	 bp = tokscan (bp, &token, &tokentype, &val, &term);
	 if (tokentype != SYM)
	 {
	    sprintf (errortext, "LOAD requires symbol: %s", token);
	    logerror (errortext);
	    return;
	 }
	 if (strlen(token) > MAXSYMLEN)
	 {
	    token[MAXSYMLEN] = '\0';
	 }
	 if ((s = symlookup (token, FALSE, FALSE)) == NULL)
	 {
	    s = symlookup (token, TRUE, FALSE);
	 }
	 s->value = 0;
	 s->flags &= ~RELOCATABLE;
	 s->flags |= LOADSYM;
      } while (term == ',');
      break;

   case LREF_T:
      do {
	 bp = tokscan (bp, &token, &tokentype, &val, &term);
	 if (tokentype != SYM)
	 {
	    sprintf (errortext, "LREF requires symbol: %s", token);
	    logerror (errortext);
	    return;
	 }
	 if (strlen(token) > MAXSYMLEN)
	 {
	    token[MAXSYMLEN] = '\0';
	 }
	 if ((s = symlookup (token, FALSE, TRUE)) == NULL)
	 {
	    s = symlookup (token, TRUE, TRUE);
	 }
	 s->value = 0;
	 s->flags |= EXTERNAL | LONGSYM;
      } while (term == ',');
      break;

   case NOP_T:
   case RT_T:
      if (pc & 0x0001) pc++;
      pc += 2;
      break;

   case PAGE_T:
      break;

   case REF_T:
      do {
	 bp = tokscan (bp, &token, &tokentype, &val, &term);
	 if (tokentype != SYM)
	 {
	    sprintf (errortext, "REF requires symbol: %s", token);
	    logerror (errortext);
	    return;
	 }
	 if (strlen(token) > MAXSYMLEN)
	 {
	    token[MAXSYMLEN] = '\0';
	 }
	 if ((s = symlookup (token, FALSE, TRUE)) == NULL)
	 {
	    s = symlookup (token, TRUE, TRUE);
	 }
	 s->value = 0;
	 s->flags |= EXTERNAL;
      } while (term == ',');
      break;

   case RORG_T:
      if (incseg)
      {
         csegs[curcseg].length = pc;
	 incseg = FALSE;
      }
      bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      indorg = FALSE;
      absolute = FALSE;
      relocatable = TRUE;
      if (val == 0)
	 pc = rorgpc;
      else
         pc = val & 0xFFFE;
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = pc;
	 s->flags |= RELOCATABLE;
      }
      break;

   case SREF_T:
      do {
	 bp = tokscan (bp, &token, &tokentype, &val, &term);
	 if (tokentype != SYM)
	 {
	    sprintf (errortext, "SREF requires symbol: %s", token);
	    logerror (errortext);
	    return;
	 }
	 if (strlen(token) > MAXSYMLEN)
	 {
	    token[MAXSYMLEN] = '\0';
	 }
	 if ((s = symlookup (token, FALSE, FALSE)) == NULL)
	 {
	    s = symlookup (token, TRUE, TRUE);
	 }
	 s->value = 0;
	 s->flags &= ~RELOCATABLE;
	 s->flags |= SREFSYM;
      } while (term == ',');
      break;

   case TEXT_T:
      bp = tokscan (bp, &token, &tokentype, &val, &term);
      if (term == '-')
	 bp = tokscan (bp, &token, &tokentype, &val, &term);
      if (tokentype == STRING)
      {
	 bytecnt = strlen(token);
	 pc += bytecnt;
      }
      break;

   case TITL_T:
      if (!ttlbuf[0])
      {
	 while (isspace(*bp)) bp++;
	 if (*bp == '\'')
	 {
	    cp = ++bp;
	    while (*bp != '\'' && *bp != '\n') bp++;
	 }
	 *bp = '\0';
	 strcpy (ttlbuf, cp);
      }
      break;

   case UNL_T:
      break;

   case XVEC_T:
      pc += 4;
      break;

   case CEND_T:
      if (!incseg)
      {
         logerror ("CEND not in CSEG");
	 break;
      }
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = pc;
	 s->flags |= RELOCATABLE;
	 s->flags |= CSEGSYM;
      }
      csegs[curcseg].length = pc;
      incseg = FALSE;
      absolute = FALSE;
      relocatable = TRUE;
      inpseg = TRUE;
      pc = rorgpc;
      break;

   case DEND_T:
      if (!indseg)
      {
         logerror ("DEND not in DSEG");
	 break;
      }
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = pc;
	 s->flags |= RELOCATABLE;
	 s->flags |= DSEGSYM;
      }
      dseglength = pc;
      indseg = FALSE;
      absolute = FALSE;
      relocatable = TRUE;
      inpseg = TRUE;
      pc = rorgpc;
      break;

   case PEND_T:
      if (incseg || indseg)
      {
         logerror ("PEND not in PSEG");
	 break;
      }
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = pc;
	 s->flags |= RELOCATABLE;
      }
      absolute = FALSE;
      relocatable = TRUE;
      inpseg = TRUE;
      break;

   case END_T:
      if (indseg)
      {
	 dseglength = pc;
	 indseg = FALSE;
	 pc = rorgpc;
      }
      if (incseg)
      {
         csegs[curcseg].length = pc;
	 incseg = FALSE;
	 pc = rorgpc;
      }
      indorg = FALSE;
      codelength = pc;
      pgmlength = pc + litlen;
      inpseg = TRUE;
      break;

   case CSEG_T:
      if (!absolute && !incseg && !indseg && !indorg) rorgpc = pc;
      if (indseg)
      {
	 dseglength = pc;
	 indseg = FALSE;
      }
      if (incseg)
      {
         csegs[curcseg].length = pc;
      }
      absolute = FALSE;
      indorg = FALSE;
      inpseg = FALSE;
      relocatable = TRUE;
      incseg = TRUE;
      while (*bp && isspace(*bp) && (bp - pscanbuf < rightmargin)) bp++;
      cp = bp;
      if (*bp == '\'')
      {
	 cp = ++bp;
	 while (*bp != '\'' && *bp != '\n')
	 {
	    if (islower (*bp)) *bp = toupper(*bp);
	    bp++;
	 }
      }
      else
      {
         strcpy (errortext, "$BLANK");
	 cp = errortext;
      }
      *bp = '\0';
      if (strlen(cp) > MAXSHORTSYMLEN)
      {
	 *(cp+MAXSHORTSYMLEN) = '\0';
      }
      j = FALSE;
      for (i = 0; i < csegcnt; i++)
      {
         if (!strcmp (csegs[i].name, cp))
	 {
	    j = TRUE;
	    curcseg = i;
	    break;
	 }
      }
      pc = csegs[curcseg].length;
      break;

   case DSEG_T:
      if (!absolute && !incseg && !indseg && !indorg) rorgpc = pc;
      if (incseg)
      {
         csegs[curcseg].length = pc;
	 incseg = FALSE;
      }
      absolute = FALSE;
      indorg = FALSE;
      inpseg = FALSE;
      relocatable = TRUE;
      indseg = TRUE;
      if (dseglength < 0)
         dseglength = 0;
      pc = dseglength;
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = pc;
	 s->flags |= RELOCATABLE;
	 s->flags |= DSEGSYM;
      }
      break;

   case PSEG_T:
      if (incseg)
      {
         csegs[curcseg].length = pc;
      }
      if (indseg)
      {
	 dseglength = pc;
      }
      incseg = FALSE;
      indseg = FALSE;
      indorg = FALSE;
      absolute = FALSE;
      relocatable = TRUE;
      if (!inpseg)
	 pc = rorgpc;
      inpseg = TRUE;
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = pc;
	 s->flags |= RELOCATABLE;
      }
      break;

   default: ;
   }
}

/***********************************************************************
* asmpass1 - Pass 1
***********************************************************************/

int
asmpass1 (FILE *infd)
{
   char *token;
   int status = 0;
   int val;
   int tokentype;
   int done;
   char term;

#ifdef DEBUG
   printf ("asmpass1: Entered\n");
#endif

   /*
   ** Rewind the input.
   */

   tmpfd = infd;
   if (fseek (tmpfd, 0, SEEK_SET) < 0)
   {
      perror ("asm990: Can't rewind temp file");
      return (-1);
   }

   /*
   ** Process the source.
   */

   pc = 0;
   rorgpc = 0;
   litlen = 0;
   litcount = 0;

   absolute = FALSE;
   indorg = FALSE;
   incseg = FALSE;
   done = FALSE;
   inskip = FALSE;

   linenum = 0;
   ttlbuf[0] = '\0';
   idtbuf[0] = '\0';

   dseglength = -1;

   pscanbuf = inbuf;

   while (!done)
   {
      char *bp;

      /*
      ** Read source line mode, number and text.
      */

      if (readsource (tmpfd, &inmode, &linenum, &filenum, inbuf) < 0)
      {
         done = TRUE;
	 break;
      }

      errnum = 0;
      errline[0][0] = '\0';
      bp = inbuf;

      /*
      ** If not a comment, then process.
      */

      if (!(inmode & MACDEF) && (*bp != COMMENTSYM))
      {
	 SymNode *s;
	 OpCode *op;

	 /*
	 ** If label present, add to symbol table.
	 */

         if (isalpha(*bp) || *bp == '$' || *bp == '_')
	 {
	    bp = tokscan (bp, &token, &tokentype, &val, &term);
	    strcpy (cursym, token);
	    if (strlen(token) > MAXSYMLEN)
	    {
	       cursym[MAXSYMLEN] = '\0';
	       token[MAXSYMLEN] = '\0';
	    }
	    if (!inskip)
	    {
	       if ((s = symlookup (token, FALSE, TRUE)) == NULL)
	       {
		  s = symlookup (token, TRUE, TRUE);
	       }
	       if (s)
	       {
	          if (indseg) s->flags |= DSEGSYM;
	          if (incseg) s->flags |= CSEGSYM;
		  s->value = pc;
#if defined(DEBUGPC) || defined(DEBUGDORG)
		  printf ("p1: cursym = %s, pc = %04X\n", cursym, pc);
#endif
	       }
	    }
	    if (term == '\n') continue;
	 }
	 else 
	 {
	    cursym[0] = '\0';
	    while (isspace (*bp)) bp++;
	 }

	 /*
	 ** Scan off opcode.
	 */

	 bp = tokscan (bp, &token, &tokentype, &val, &term);
	 if (!token[0]) continue;
	 if ((inmode & MACCALL) || (inmode & SKPINST)) continue;
#ifdef DEBUGPC
	 printf ("INST = %s, pc = %04X\n", token, pc);
#endif

	 op = oplookup (token, 1);
	 if (inskip)
	 {
	    if (op != NULL && op->optype == TYPE_P)
	       switch (op->opvalue)
	       {
	       case ASMIF_T:
	       case ASMELS_T:
	       case ASMEND_T:
	          p1pop (op, bp);
		  break;

	       default: ;
	       }
	 }
	 else if (op != NULL)
	 {
	    while (*bp && isspace(*bp)) bp++;

	    bytecnt = 0;
	    if ((op->optype != TYPE_P) && (pc & 0x0001)) pc++;
	    switch (op->optype)
	    {

	       case TYPE_1:
	          p112mop (op, bp);
		  break; 

	       case TYPE_2:
	       case TYPE_5:
	       case TYPE_7:
	       case TYPE_10:
	       case TYPE_18:
	          p110mop (op, bp);
		  break; 

	       case TYPE_3:
	       case TYPE_6:
	       case TYPE_4:
	       case TYPE_9:
	          p111mop (op, bp);
		  break; 

	       case TYPE_8:
	          p111iop (op, bp);
		  break; 

	       case TYPE_11:
	       case TYPE_12:
	       case TYPE_16:
	       case TYPE_19:
	       case TYPE_21:
	          p122mop (op, bp);
		  break; 

	       case TYPE_20:
	          p122cop (op, bp);
		  break; 

	       case TYPE_13:
	       case TYPE_14:
	       case TYPE_15:
	          p121mop (op, bp);
		  break; 

	       case TYPE_17:
	          p120mop (op, bp);
		  break; 

	       case TYPE_P:
	          p1pop (op, bp);
	          break;

	       default: ;
	    }
	 }
	 else
	 {
	    /* print errors in pass2 */
	    status = -1;
	 }
	 
      }

   }

   return (status);
}
