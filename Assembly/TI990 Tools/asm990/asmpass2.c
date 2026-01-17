/***********************************************************************
*
* asmpass2.c - Pass 2 for the TI 990 assembler.
*
* Changes:
*   05/21/03   DGP   Original.
*   06/15/03   DGP   Added registerarg function.
*   06/23/03   DGP   Added date/time and creator to EOF record.
*   07/01/03   DGP   Fixed TEXT/BYTE processing.
*                    Fixed negative REF/DEF equ'd values.
*   07/02/03   DGP   Change hanging byte methods.
*                    Fixed register expressions.
*   07/08/03   DGP   Moved get date/time to always get it.
*   07/10/03   DGP   Fixed EVEN to use absolute for new org.
*   07/17/03   DGP   Fixed BSS & BES hanging byte.
*   08/12/03   DGP   Revert to one word per line listing.
*                    Add type 10 opcode (LMF).
*   08/13/03   DGP   Added /12 instructions.
*   12/01/03   DGP   Insure pgmlength is 16 bits in IDT.
*   04/08/04   DGP   Added "cassette" mode and orgout.
*   04/09/04   DGP   Added object checksum.
*   05/20/04   DGP   Added literal support.
*   05/25/04   DGP   Added Long REF/DEF pseudo ops.
*   06/07/04   DGP   Added LONG, QUAD, FLOAT and DOUBLE pseudo ops.
*   04/27/05   DGP   Put IDT in record sequence column 72.
*   12/26/05   DGP   Added DSEG support.
*   12/30/05   DGP   Changes SymNode to use flags.
*   01/03/06   DGP   Correct RELORG output on TEXT/BYTE psops.
*   01/09/07   DGP   Changed to use function branch table.
*   01/17/07   DGP   Correct literal external reference chain.
*   01/31/07   DGP   Allow destination litteral for compare insts.
*   06/20/07   DGP   Adjust for long symbols in list format.
*   12/04/07   DGP   Allow DSEG to be relocatable.
*   12/15/08   DGP   Added widelist option.
*   03/16/09   DGP   Correct float/double literal
*   03/17/09   DGP   Correct PC offset with BES/BSS and odd PC.
*   05/28/09   DGP   Added MACRO and CSEG support.
*   06/03/09   DGP   Cleaned up C/D/P SEGS.
*   06/29/09   DGP   In asmskip mode only check for ASMELS or ASMEND.
*   04/22/10   DGP   Generate error in TXMIRA mode if space follows a
*                    comma in a destination operand.
*   11/30/10   DGP   Fixed origin output.
*   12/01/10   DGP   Added External Index support.
*   12/06/10   DGP   Fixed TEXT -'string' for last character.
*   12/07/10   DGP   Added checkexpr function.
*   12/10/10   DGP   Fixed literal for new external refcnt.
*   12/14/10   DGP   Allow '@' in immediate expression in SDSMAC mode.
*                    Added line number to temporary file.
*                    Added file number to error processing.
*   12/23/10   DGP   Correct OPTION for *LST and process options in a
*                    function.
*   12/24/10   DGP   Correct page count to eject new page.
*   01/05/11   DGP   Correct extname processing with multiple operands.
*   01/06/11   DGP   Fixed printdata to not print if data is in MUNLST.
*                    Fixed cross reference issues.
*   02/09/11   DGP   Unify segments in segvaltype.
*   06/14/12   DGP   Correct checksum for EBCDIC.
*   08/22/13   DGP   Fixed missing memory reference.
*   08/23/13   DGP   Fixed emit of zero length DSEG.
*                    Fixed CSEG REF/DEF.
*   02/03/14   DGP   Added SYMT support.
*   02/21/14   DGP   Allow expression in address literal.
*   10/06/14   DGP   Fixed CSEG Data in instructions.
*   10/09/14   DGP   Refactor operand processing.
*   10/15/14   DGP   Fixed punchrecord overrun.
*   12/29/14   DGP   Allow for nested $IF/ASMIF statements.
*   02/20/15   DGP   When skipping allow for undefined opcode in block.
*   10/09/15   DGP   Added Long symbol common tags.
*   10/15/15   DGP   Fixed external symbol expression in literal.
*   10/19/15   DGP   Fixed print relocation marker when ext+offset.
*   11/18/15   DGP   Added inpseg flag.
*                    Fixed dseg and common symbols in literal.
*   11/17/22   DGP   Fixed IDT and TITL test.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "asmdef.h"

extern int pc;
extern int model;
extern int rorgpc;
extern int csegcnt;
extern int inpseg;
extern int indseg;
extern int incseg;
extern int segvaltype;
extern int csegval;
extern int genxref;
extern int gensymt;
extern int symbolcount;
extern int errcount;
extern int errnum;
extern int absolute;
extern int cassette;
extern int linenum;
extern int widelist;
extern int indorg;
extern int p1errcnt;
extern int pgmlength;
extern int codelength;
extern int dseglength;
extern int libincnt;		/* Number of LIBIN's active */
extern int libout;		/* LIBOUT active */
extern int rightmargin;		/* Right margin for scanner */
extern int txmiramode;
extern int extvalue;
extern int filenum;
extern int bunlist;
extern int dunlist;
extern int munlist;
extern int tunlist;
extern char extname[MAXSYMLEN+2];
extern char errline[10][120];
extern char inbuf[MAXLINE];
extern char *pscanbuf;		/* Pointer for tokenizers */

extern SymNode *symbols[MAXSYMBOLS];
extern ErrTable p1error[MAXERRORS];
extern CSEGNode csegs[MAXCSEGS];
extern LIBfile libinfiles[MAXASMFILES];/* LIBIN file descriptors */
extern LIBfile liboutfile;	/* LIBOUT file descriptor */

char ttlbuf[TTLSIZE+2];
char idtbuf[IDTSIZE+2];

static FILE *tmpfd;
static int linecnt = MAXLINE;
static int curp1err = 0;
static int objrecnum = 0;
static int objcnt = 0;
static int pagenum = 0;
static int printed = FALSE;
static int listing = TRUE;
static int listmode;
static int inmode;
static char relochar;
static char datebuf[48];
static char pcbuf[6];
static char objbuf[LINESIZE+2];
static char objrec[LINESIZE+2];
static char opbuf[32];
static char cursym[MAXSYMLEN+1];
static char lstbuf[MAXLINE];
static char errtmp[256];	/* Error print string */
static struct tm *timeblk;

static uint16 byteacc;
static int bytecnt;
static int ckpt = -1;
static int curcseg;
static int extndx;
static int inskip;		/* In an ASMIF Skip */

static int orgout = FALSE;

static void p201op (OpCode *, char *, FILE *, FILE *);
static void p202op (OpCode *, char *, FILE *, FILE *);
static void p203op (OpCode *, char *, FILE *, FILE *);
static void p204op (OpCode *, char *, FILE *, FILE *);
static void p205op (OpCode *, char *, FILE *, FILE *);
static void p206op (OpCode *, char *, FILE *, FILE *);
static void p207op (OpCode *, char *, FILE *, FILE *);
static void p208op (OpCode *, char *, FILE *, FILE *);
static void p209op (OpCode *, char *, FILE *, FILE *);
static void p210op (OpCode *, char *, FILE *, FILE *);
static void p211op (OpCode *, char *, FILE *, FILE *);
static void p212op (OpCode *, char *, FILE *, FILE *);
static void p213op (OpCode *, char *, FILE *, FILE *);
static void p214op (OpCode *, char *, FILE *, FILE *);
static void p215op (OpCode *, char *, FILE *, FILE *);
static void p216op (OpCode *, char *, FILE *, FILE *);
static void p217op (OpCode *, char *, FILE *, FILE *);
static void p218op (OpCode *, char *, FILE *, FILE *);
static void p219op (OpCode *, char *, FILE *, FILE *);
static void p220op (OpCode *, char *, FILE *, FILE *);
static void p221op (OpCode *, char *, FILE *, FILE *);

typedef struct {
   void (*proc) (OpCode *, char *, FILE *, FILE *);
} Inst_Proc;

Inst_Proc inst_proc[MAX_INST_TYPES] = {
   p201op, p202op, p203op, p204op, p205op, p206op, p207op,
   p208op, p209op, p210op, p211op, p212op, p213op, p214op,
   p215op, p216op, p217op, p218op, p219op, p220op, p221op
};

/***********************************************************************
* printheader - Print header on listing.
***********************************************************************/

static void
printheader (FILE *lstfd)
{
   if (listmode && listing)
   {
      if (linecnt >= LINESPAGE)
      {
	 linecnt = 0;
	 if (pagenum) fputc ('\f', lstfd);
	 if (widelist)
	    fprintf (lstfd, H1WFORMAT, VERSION, idtbuf, datebuf, ++pagenum);
	 else
	    fprintf (lstfd, H1FORMAT, VERSION, idtbuf, datebuf, ++pagenum);
	 fprintf (lstfd, H2FORMAT, ttlbuf); 
      }
   }
}

/***********************************************************************
* printline - Print line on listing.
*
* 0        1         2         3         4
* 1234567890123456789012345678901234567890
*
* oooo FFFFr LLLLx TTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT
* oooo FFFF  
*
***********************************************************************/

static void
printline (FILE *lstfd)
{
   char *xp;
   char mch = ' ';
   char linebuf[MAXLINE];

   if (munlist && (inmode & MACEXP))
      return;

   if (listmode && listing)
   {
      printheader (lstfd);
      if (widelist)
      {
         if ((inmode & INSERT) || (inmode & MACEXP))
	 {
	    if ((xp = strchr (lstbuf, '\n')) != NULL) *xp = '\0';
	    sprintf (linebuf, "%-80.80s  %4d\n",
		     lstbuf, (inmode & NESTMASK) >> NESTSHFT);
	 }
	 else
	    strcpy (linebuf, lstbuf);
      }
      else
      {
	 strcpy (linebuf, lstbuf);
	 if (strlen (linebuf) > PRINTMARGIN)
	 {
	    linebuf[PRINTMARGIN] = '\n';
	    linebuf[PRINTMARGIN+1] = '\0';
	 }
      }

      if (inskip || inmode & SKPINST)
         mch = 'X';
      else if (inmode & MACEXP)
         mch = '+';
      else if (inmode & INSERT)
         mch = 'C';

      fprintf (lstfd, L1FORMAT, pcbuf, opbuf,
	       linenum, mch, linebuf);
      linecnt++;
   }
}

/***********************************************************************
* printdata - Print a data line.
***********************************************************************/

static void
printdata (FILE *lstfd, int opc)
{
   if (munlist && (inmode & MACEXP))
      return;

   if (listmode && listing)
   {
      printheader (lstfd);
      sprintf (pcbuf, PCFORMAT, opc);
      fprintf (lstfd, L2FORMAT, pcbuf, opbuf);
      linecnt++;
   }
}

/***********************************************************************
* printlit - Print a literal data line.
***********************************************************************/

static void
printlit (FILE *lstfd, int opc, char *lit)
{
   if (listmode && listing)
   {
      printheader (lstfd);
      sprintf (pcbuf, PCFORMAT, opc);
      fprintf (lstfd, LITFORMAT, pcbuf, opbuf, lit);
      linecnt++;
   }
}

/***********************************************************************
* printsymbols - Print the symbol table.
***********************************************************************/

static void
printsymbols (FILE *lstfd)
{
   int i, j;
   int longformat;
   int maxsymlen;
   int symperline;
   int xrefperline;
   char type;
   char format[40];
   char xrefpad[LINESIZE];
   char xrefheader[LINESIZE+2];

   if (!listmode) return;

   linecnt = MAXLINE;
   longformat = FALSE;
   maxsymlen = 6;

   for (i = 0; i < symbolcount; i++)
   {
      if (symbols[i]->flags & LONGSYM)
      {
	 longformat = TRUE;
	 if ((j = strlen (symbols[i]->symbol)) > maxsymlen)
	    maxsymlen = j;
      }
   }

   maxsymlen += 2;
   if (widelist)
      symperline = WLINESIZE / (8 + maxsymlen);
   else
      symperline = LINESIZE / (8 + maxsymlen);
   if (longformat)
   {
      char temp[40];

      for (i = 0; i < maxsymlen-7; i++) temp[i] = ' ';
      temp[i] = '\0';
      sprintf (format, LONGSYMFORMAT, maxsymlen, maxsymlen);
      sprintf (xrefheader, LONGXREFHEADER, temp);
      for (i = 0; i < maxsymlen; i++) temp[i] = ' ';
      temp[i] = '\0';
      sprintf (xrefpad, "%s              ", temp);
   }
   else
   {
      strcpy (format, SYMFORMAT);
      strcpy (xrefheader, XREFHEADER);
      strcpy (xrefpad, "                      ");
   }
   if (widelist)
      xrefperline = (WLINESIZE - maxsymlen - 14) / 6;
   else
      xrefperline = (LINESIZE - maxsymlen - 14) / 6;

   j = 0;
   for (i = 0; i < symbolcount; i++)
   {
      SymNode *sym = symbols[i];

      if (sym->flags & P0SYM)
         continue;

      printheader (lstfd);

      if (genxref && !linecnt)
      {
	 fputs (xrefheader, lstfd);
	 linecnt = 3;
      }

      if (!(sym->flags & XOPSYM) && (sym->symbol[0] != LITERALSYM))
      {

	 if      (sym->flags & GLOBAL)       type = 'D';
	 else if (sym->flags & EXTERNAL)     type = 'R';
	 else if (sym->flags & LOADSYM)      type = 'L';
	 else if (sym->flags & SREFSYM)      type = 'S';
	 else if (sym->flags & RELOCATABLE)
	 {
	    if (sym->flags & CSEGSYM)        type = '+';
	    else if (sym->flags & DSEGSYM)   type = '"';
	    else                             type = '\'';
	 }
	 else                                type = ' ';

	 fprintf (lstfd, format, sym->symbol, sym->value & 0xFFFF, type);
	 j++;

	 if (genxref)
	 {
	    XrefNode *xref = sym->xref_head;

	    j = 0;
	    fprintf (lstfd, " %4d ", symbols[i]->line);
	    while (xref)
	    {
	       if (j >= xrefperline)
	       {
		  fprintf (lstfd, "\n");
		  printheader (lstfd);
	          if (!linecnt)
		  {
		     fputs (xrefheader, lstfd);
		     linecnt = 2;
		  }
		  fputs (xrefpad, lstfd);
		  linecnt++;
		  j = 0;
	       }
	       fprintf (lstfd, " %4d ", xref->line);
	       xref = xref->next;
	       j++;
	    }
	    j = symperline+1;
	 }

	 if (j >= symperline)
	 {
	    fprintf (lstfd, "\n");
	    linecnt++;
	    j = 0;
	 }
      }
   }
   fprintf (lstfd, "\n");
}

/***********************************************************************
* printerrors - Print any error message for this line.
***********************************************************************/

static int
printerrors (FILE *lstfd, int listmode)
{
   int i;
   int ret;

   ret = 0;
   if (errnum)
   {
      for (i = 0; i < errnum; i++)
      {
#ifdef DEBUGERROR
	 printf ("printerrors: p2: i = %d, linenum = %d, filenum = %d\n",
		 i, linenum, filenum);
	 printf ("   error = %s\n", p1error[i].errortext);
#endif
	 if (listmode)
	 {
	    fprintf (lstfd, "ERROR: %s\n", errline[i]);
	    linecnt++;
	 }
	 else
	 {
	    fprintf (stderr, "asm990: %d-%d: %s\n",
		     filenum + 1, linenum, errline[i]);
	 }
	 errline[i][0] = '\0';
      }
      errnum = 0;
      ret = -1;
   }

   if (p1errcnt)
   {
      for (i = 0; i < p1errcnt; i++)
      {
	 if ((p1error[i].errorline == linenum) &&
	     (p1error[i].errorfile == filenum))
	 {
#ifdef DEBUGERROR
	    printf ("printerrors: p1: i = %d, linenum = %d, filenum = %d\n",
		    i, linenum, filenum);
	    printf ("   errorline = %d, errorfile = %d\n",
		    p1error[i].errorline, p1error[i].errorfile);
	    printf ("   error = %s\n", p1error[i].errortext);
#endif
	    if (listmode)
	    {
	       fprintf (lstfd, "ERROR: %s\n", p1error[i].errortext);
	       linecnt++;
	    }
	    else
	    {
	       fprintf (stderr, "asm990: %d-%d: %s\n",
			filenum + 1, linenum, p1error[i].errortext);
	    }
	    p1error[i].errorline = -1;
	    ret = -1;
	 }
      }
   }

   return (ret);
}

/***********************************************************************
* punchfinish - Punch a record with sequence numbers.
***********************************************************************/

static void
punchfinish (FILE *outfd)
{
   if (objcnt)
   {
      int i;
      short int cksum;
      char temp[10];

      cksum = 0;
      for (i = 0; i < objcnt; i++) cksum += TOASCII (objrec[i]);
      cksum += TOASCII (CKSUM_TAG);
      cksum = -cksum;
      sprintf (temp, "%c%04X%c", CKSUM_TAG, cksum & 0xFFFF, EOR_TAG);
      strncpy (&objrec[objcnt], temp, 6);
      objcnt += 6;

      if (cassette)
      {
	 strcpy (&objrec[objcnt], "\n");
      }
      else
      {
	 if (idtbuf[0])
	    strncpy (&objrec[IDTCOLUMN], idtbuf, strlen(idtbuf));
	 sprintf (&objrec[SEQUENCENUM], SEQFORMAT, ++objrecnum);
      }
      fputs (objrec, outfd);
      memset (objrec, ' ', sizeof(objrec));
      objcnt = 0;
   }
}

/***********************************************************************
* punchrecord - Punch an object value into record.
***********************************************************************/

static void 
punchrecord (FILE *outfd)
{
   int len = strlen (objbuf);

   if (!indorg)
   {
      if (!orgout && !absolute)
      {
	 char temp[20];
	 int objlen;

	 orgout = TRUE;
	 if (!(objbuf[0] == ABSORG_TAG || objbuf[0] == RELORG_TAG ||
	       objbuf[0] == CMNORG_TAG || objbuf[0] == DSEGORG_TAG))
	 {
	    if (incseg)
	    {
	       sprintf (temp, CMNOBJFORMAT, CMNORG_TAG,
			pc & 0xFFFE, csegs[curcseg].number);
	       objlen = 9;
	    }
	    else
	    {
	       sprintf (temp, OBJFORMAT, RELORG_TAG, pc & 0xFFFE);
	       objlen = 5;
	    }
	    if (objcnt+objlen >= (cassette ? CHARSPERCASREC : CHARSPERREC))
	    {
	       punchfinish (outfd);
	    }
	    strncpy (&objrec[objcnt], temp, objlen);
	    objcnt += objlen;
	 }
      }
      if (objcnt+len >= (cassette ? CHARSPERCASREC : CHARSPERREC))
      {
	 punchfinish (outfd);
      }
      strncpy (&objrec[objcnt], objbuf, len);
      objcnt += len;
      objbuf[0] = '\0';
   }
}

/***********************************************************************
* punchsymbols - Punch REF and DEF symbols.
***********************************************************************/

static void
punchsymbols (FILE *outfd)
{
   SymNode *sym;
   int i;
   int emitsym;

#ifdef DEBUGPUNCHSYM
   printf ("punchsymbols:\n");
#endif
   punchfinish (outfd);
   orgout = TRUE;
   for (i = 0; i < symbolcount; i++)
   {
      sym = symbols[i];

      if ((sym->flags & P0SYM) || (sym->flags & XOPSYM) ||
          (sym->flags & PREDEFINE))
         continue;

#ifdef DEBUGPUNCHSYM
      printf ("   sym = %-32.32s, flags = >%04X, value = >%04X\n",
	      sym->symbol, sym->flags, sym->value);
#endif

      emitsym = TRUE;
      if (sym->flags & EXTERNAL)
      {
#ifdef DEBUGEXTNDX
	printf ("PUNCH: symbol = %s, value = %04X, flags = %X\n", 
		sym->symbol, sym->value, sym->flags);
	printf ("   refcnt = %d, extcnt = %d\n", sym->refcnt, sym->extcnt);
#endif
	 emitsym = FALSE;
	 if (sym->refcnt == 0)
	    sym->flags &= ~RELOCATABLE;

	 if (sym->flags & DSEGEXTERN)
	 {
	    if (sym->flags & LONGSYM)
	       sprintf (objbuf, LCMNFORMAT, LCMNEXTERN_TAG,
			sym->value & 0xFFFF, sym->symbol, 0);
	    else
	       sprintf (objbuf, CMNFORMAT, CMNEXTERN_TAG,
			sym->value & 0xFFFF, sym->symbol, 0);
	 }
	 else if (sym->flags & CSEGEXTERN)
	 {
	    if (sym->flags & LONGSYM)
	       sprintf (objbuf, LCMNFORMAT, LCMNEXTERN_TAG, sym->value & 0xFFFF,
			sym->symbol, sym->csegndx);
	    else
	       sprintf (objbuf, CMNFORMAT, CMNEXTERN_TAG, sym->value & 0xFFFF,
			sym->symbol, sym->csegndx);
	 }
	 else
	 {
	    if (sym->flags & LONGSYM)
	       sprintf (objbuf, LREFFORMAT,
		     (sym->flags & RELOCATABLE) ? LRELEXTRN_TAG : LABSEXTRN_TAG,
		     sym->value & 0xFFFF, sym->symbol);
	    else
	       sprintf (objbuf, REFFORMAT,
		    (sym->flags & RELOCATABLE) ? RELEXTRN_TAG : ABSEXTRN_TAG,
		     sym->value & 0xFFFF, sym->symbol);
	 }
         punchrecord (outfd);
      }
      else if (sym->flags & GLOBAL)
      {
	 if (sym->flags & DSEGSYM)
	 {
	    if (sym->flags & LONGSYM)
	       sprintf (objbuf, LCMNFORMAT, LCMNGLOBAL_TAG,
			sym->value & 0xFFFF, sym->symbol, 0);
	    else
	       sprintf (objbuf, CMNFORMAT,
			CMNGLOBAL_TAG, sym->value & 0xFFFF, sym->symbol, 0);
	 }
	 else if (sym->flags & CSEGSYM)
	 {
	    if (sym->flags & LONGSYM)
	       sprintf (objbuf, LCMNFORMAT, LCMNGLOBAL_TAG, sym->value & 0xFFFF,
			sym->symbol, sym->csegndx);
	    else
	       sprintf (objbuf, CMNFORMAT, CMNGLOBAL_TAG, sym->value & 0xFFFF,
			sym->symbol, sym->csegndx);
	 }
	 else
	 {
	    if (sym->flags & LONGSYM)
	       sprintf (objbuf, LDEFFORMAT,
		   (sym->flags & RELOCATABLE) ? LRELGLOBAL_TAG : LABSGLOBAL_TAG,
			sym->value & 0xFFFF, sym->symbol);
	    else
	       sprintf (objbuf, DEFFORMAT,
		   (sym->flags & RELOCATABLE) ? RELGLOBAL_TAG : ABSGLOBAL_TAG,
			sym->value & 0xFFFF, sym->symbol);
	 }
         punchrecord (outfd);
      }
      else if (sym->flags & LOADSYM)
      {
	 if (sym->flags & LONGSYM)
	 {
	    sprintf (objbuf, LREFFORMAT, LLOAD_TAG, 0, sym->symbol);
	 }
	 else
	 {
	    sprintf (objbuf, REFFORMAT, LOAD_TAG, 0, sym->symbol);
	 }
         punchrecord (outfd);
      }
      else if (sym->flags & SREFSYM)
      {
	 if (sym->refcnt == 0)
	    sym->flags &= ~RELOCATABLE;
	 if (sym->flags & LONGSYM)
	 {
	    sprintf (objbuf, LREFFORMAT,
		     (sym->flags & RELOCATABLE) ? LRELSREF_TAG : LABSSREF_TAG,
		     sym->value & 0xFFFF, sym->symbol);
	 }
	 else
	 {
	    sprintf (objbuf, REFFORMAT,
		     (sym->flags & RELOCATABLE) ? RELSREF_TAG : ABSSREF_TAG,
		     sym->value & 0xFFFF, sym->symbol);
	 }
         punchrecord (outfd);
      }
      if (gensymt && emitsym)
      {
	 if (sym->flags & LONGSYM)
	 {
	    sprintf (objbuf, LDEFFORMAT,
		   (sym->flags & RELOCATABLE) ? LRELSYMBOL_TAG : LABSSYMBOL_TAG,
		     sym->value & 0xFFFF, sym->symbol);
	 }
	 else
	 {
	    sprintf (objbuf, DEFFORMAT,
		     (sym->flags & RELOCATABLE) ? RELSYMBOL_TAG : ABSSYMBOL_TAG,
		     sym->value & 0xFFFF, sym->symbol);
	 }
         punchrecord (outfd);
      }
   }
}

/***********************************************************************
* puncheof - Punch EOF mark.
***********************************************************************/

static void
puncheof (FILE *outfd)
{
   char temp[LINESIZE+2];

   punchfinish (outfd);
   objrec[0] = EOFSYM;
   if (cassette)
   {
      strcpy (&objrec[1], "\n");
   }
   else
   {
      sprintf (temp, "%-8.8s  %02d/%02d/%02d  %02d:%02d:%02d    ASM990 %s",
	       idtbuf,
	       timeblk->tm_mon+1, timeblk->tm_mday, timeblk->tm_year - 100,
	       timeblk->tm_hour, timeblk->tm_min, timeblk->tm_sec,
	       VERSION);
      strncpy (&objrec[7], temp, strlen(temp));
      if (idtbuf[0])
	 strncpy (&objrec[IDTCOLUMN], idtbuf, strlen(idtbuf));
      sprintf (&objrec[SEQUENCENUM], SEQFORMAT, ++objrecnum);
   }
   fputs (objrec, outfd);
}

/***********************************************************************
* outputdataword - Output a word of data.
***********************************************************************/

static int
outputdataword (FILE *outfd, FILE *lstfd, int opc, int val, int first)
{
   sprintf (opbuf, DATA1FORMAT, val, ' ');
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);
   pc += 2;
   printed = TRUE;
   if (first)
   {
      printline (lstfd);
   }
   else
   {
      if (!dunlist)
	 printdata (lstfd, opc);
   }
   strcpy (opbuf, OPBLANKS);
   return (pc);
}

/***********************************************************************
* outputlitword - Ouput a literal word value.
***********************************************************************/

static void 
outputlitword (FILE *outfd, FILE *lstfd, int d, int first, char *sym)
{
   printed = TRUE;
   if (listmode)
   {
      sprintf (opbuf, OP1FORMAT, d, ' ');
      if (first)
	 printlit (lstfd, pc, sym);
      else if (!dunlist)
	 printdata (lstfd, pc);
   }
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, d);
   punchrecord (outfd);
   pc += 2;
}

/***********************************************************************
* processextndx - Process External ref. index.
***********************************************************************/

static void
processextndx (char *outbuf, int val, char *name, FILE *outfd)
{
   SymNode *s;

   if ((s = symlookup (name, FALSE, FALSE)) == NULL)
   {
      logerror ("Internal Error");
      return;
   }

   if (!(s->flags & EXTERNOUT))
   {
      s->flags |= EXTERNOUT;
      s->extndx = extndx++;
      if (s->flags & LONGSYM)
	 sprintf (objbuf, LREFFORMAT, LABSEXTRN_TAG, 0, name);
      else
	 sprintf (objbuf, REFFORMAT, ABSEXTRN_TAG, 0, name);
      punchrecord (outfd);
   }

   sprintf (objbuf, CMNOBJFORMAT, EXTNDX_TAG, s->extndx, val);

}

/***********************************************************************
* processliterals - Process literals.
***********************************************************************/

static int
processliterals (FILE *outfd, FILE *lstfd, int listmode)
{
   char *cp;
   int i;
   int lrelo;
   int literalcount = 0;
   int escapecount = 0;
   int status = 0;
   int datapunched;
   int csegsym;
   int dsegsym;
   int csegnum;
   char lrelochar;

#if defined(DEBUGLITERAL) || defined(DEBUGEXTNDX)
   printf ("processliterals: entered: listmode = %s\n",
	   listmode ? "TRUE" : "FALSE");
#endif

   for (i = 0; i < symbolcount; i++)
      if (symbols[i]->symbol[0] == LITERALSYM) literalcount++;

   escapecount = literalcount * 2; /* just in case */

#if defined(DEBUGLITERAL) || defined(DEBUGEXTNDX)
   printf ("   literalcount = %d\n", literalcount);
#endif

   while (literalcount)
   {
      if (--escapecount == 0) break;
      for (i = 0; i < symbolcount; i++)
      {
	 if (symbols[i]->symbol[0] == LITERALSYM && symbols[i]->value == pc)
	 {
	    t_uint64 qd;
	    unsigned long ld;
	    int d;

#if defined(DEBUGLITERAL) || defined(DEBUGEXTNDX)
	    printf ("   literal = %s, value = %04X, pc = %04X, flags = %x\n",
		    symbols[i]->symbol, symbols[i]->value,
		    pc, symbols[i]->flags);
	    printf ("   refcnt = %d, extcnt = %d\n",
		    symbols[i]->refcnt, symbols[i]->extcnt);
#endif
	    lrelo = FALSE;
	    cp = &symbols[i]->symbol[1];
	    printed = FALSE;
	    datapunched = FALSE;
	    csegsym = FALSE;
	    dsegsym = FALSE;
	    lrelochar = relochar;
	    csegnum = 0;

	    switch (*cp)
	    {
	    case '@': /* Address literal */
	       {
		  SymNode *s;

		  cp++;
		  if (strpbrk (cp, "+-/*") != NULL)
		  {
		     char lterm;

		     cp = exprscan (cp, &d, &lterm, &lrelo, 2, FALSE, 0);
		     lterm = 0;
		     if (lrelo < 0)
		     {
			processextndx (objbuf, d, extname, outfd);
			lterm = ' ';
		     }
		     else if (segvaltype == CSEGSYM)
		     {
			sprintf (objbuf, CMNOBJFORMAT, CMNDATA_TAG, d, csegval);
			lterm = '+';
		     }
		     else if (segvaltype == DSEGSYM)
		     {
			sprintf (objbuf, OBJFORMAT, DSEGDATA_TAG, d);
			lterm = '"';
		     }
		     if (lterm)
		     {
			if (listmode && !printed)
			{
			   sprintf (opbuf, OP1FORMAT, d, '"');
			   printlit (lstfd, pc, symbols[i]->symbol);
			}
			printerrors (lstfd, listmode);
			punchrecord (outfd);
			pc += 2;
			datapunched = TRUE;
		     }
		  }
		  else if ((s = symlookup (cp, FALSE, FALSE)) == NULL)
		  {
		     if (*cp == '_' && *(cp+1) == '_')
		     {
			/* GCC generated add & mark LREF */
			if ((s=symlookup (cp, TRUE, TRUE)) != NULL)
			{
			   s->value = pc;
			   s->flags |= RELOCATABLE;
			   s->flags |= EXTERNAL | LONGSYM;
			}
		     }
		     else
		     {
			sprintf (errtmp, "Literal symbol %s not found", cp);
			logerror (errtmp);
		     }
		     d = 0;
#ifdef DEBUGEXTNDX
		     if (s && s->flags & EXTERNAL || s->flags & SREFSYM)
		     {
			printf (
			     "LITNEW: symbol = %s, value = %04X, flags = %X\n", 
				s->symbol, s->value, s->flags);
			printf ("   refcnt = %d, extcnt = %d\n",
				s->refcnt, s->extcnt);
		     }
#endif
		     if (s && (s->flags & EXTERNAL || s->flags & SREFSYM))
			s->refcnt++;
		  }
		  else
		  {
#ifdef DEBUGEXTNDX
		     printf ("LIT: symbol = %s, value = %04X, flags = %X\n", 
			     s->symbol, s->value, s->flags);
		     printf ("   refcnt = %d, extcnt = %d\n",
			     s->refcnt, s->extcnt);
#endif
		     d = s->value;
		     lrelo = s->flags & RELOCATABLE;
		     if (s->flags & DSEGSYM)
		     {
			dsegsym = TRUE;
			lrelochar = '"';
		     }
		     else if (s->flags & CSEGSYM)
		     {
			csegsym = TRUE;
			csegnum = s->csegndx;
			lrelochar = '+';
		     }
		     if (s->flags & EXTERNAL || s->flags & SREFSYM)
		     {
		        s->value = pc;
			s->flags |= RELOCATABLE;
			s->refcnt++;
			if (d == 0) lrelo = FALSE;
		     }
		  }
	       }
	       break;

	    case 'B': /* Byte decimal */
	       cp++;
	       d = atoi (cp) << 8;
	       break;

	    case 'D': /* Double precision float */
	       cp++;
	       qd = cvtdouble (cp);

	       d = (int)((qd >> 48) & 0xFFFF);
	       outputlitword (outfd, lstfd, d, TRUE, symbols[i]->symbol);
	       d = (int)((qd >> 32) & 0xFFFF);
	       outputlitword (outfd, lstfd, d, FALSE, symbols[i]->symbol);
	       d = (int)((qd >> 16) & 0xFFFF);
	       outputlitword (outfd, lstfd, d, FALSE, symbols[i]->symbol);
	       d = (int)(qd & 0xFFFF);
	       outputlitword (outfd, lstfd, d, FALSE, symbols[i]->symbol);
	       datapunched = TRUE;
	       break;
	       
	    case 'F': /* Single precision float */
	       cp++;
	       ld = cvtfloat (cp);

	       d = (int)((ld >> 16) & 0xFFFF);
	       outputlitword (outfd, lstfd, d, TRUE, symbols[i]->symbol);
	       d = (int)(ld & 0xFFFF);
	       outputlitword (outfd, lstfd, d, FALSE, symbols[i]->symbol);
	       datapunched = TRUE;
	       break;

	    case 'L': /* Long decimal */
	       cp++;
	       ld = atoi (cp);

	       d = (int)((ld >> 16) & 0xFFFF);
	       outputlitword (outfd, lstfd, d, TRUE, symbols[i]->symbol);
	       d = (int)(ld & 0xFFFF);
	       outputlitword (outfd, lstfd, d, FALSE, symbols[i]->symbol);
	       datapunched = TRUE;
	       break;

	    case 'O': /* Octal word */
	       cp++;
	       d = strtol (cp, NULL, 8);
	       break;

	    case 'Q': /* Quad decimal */
	       cp++;
#ifdef WIN32
	       sscanf (cp, "%I64d", &qd);
#else
	       sscanf (cp, "%lld", &qd);
#endif

	       d = (int)((qd >> 48) & 0xFFFF);
	       outputlitword (outfd, lstfd, d, TRUE, symbols[i]->symbol);
	       d = (int)((qd >> 32) & 0xFFFF);
	       outputlitword (outfd, lstfd, d, FALSE, symbols[i]->symbol);
	       d = (int)((qd >> 16) & 0xFFFF);
	       outputlitword (outfd, lstfd, d, FALSE, symbols[i]->symbol);
	       d = (int)(qd & 0xFFFF);
	       outputlitword (outfd, lstfd, d, FALSE, symbols[i]->symbol);
	       datapunched = TRUE;
	       break;

	    case 'W': /* Word decimal */
	       cp++;
	       d = atoi (cp);
	       break;

	    case 'X': /* Hex word */
	       cp++;
	       d = strtol (cp, NULL, 16);
	       break;

	    default:
	       sprintf (errtmp, "Invalid literal: %s", cp);
	       logerror (errtmp);
	       d = 0;
	    }

	    if (!datapunched)
	    {
	       d &= 0xFFFF;

	       if (csegsym)
		  sprintf (objbuf, CMNOBJFORMAT, CMNDATA_TAG, d, csegnum);
	       else if (dsegsym)
		  sprintf (objbuf, OBJFORMAT, DSEGDATA_TAG, d);
	       else
		  sprintf (objbuf, OBJFORMAT,
			   lrelo ? RELDATA_TAG : ABSDATA_TAG, d);

	       if (listmode && !printed)
	       {
		  sprintf (opbuf, OP1FORMAT, d, lrelo ? lrelochar : ' ');
		  printlit (lstfd, pc, symbols[i]->symbol);
	       }
	       printerrors (lstfd, listmode);
	       punchrecord (outfd);

	       pc += 2;
	    }

	    literalcount--;
	    break;
	 }
      }
   }
   return (status);
}

/***********************************************************************
* generalarg - Process general operand arg.
***********************************************************************/

static char *
generalarg (OpCode *op, char *bp, int *ts, int *tr, int *saddr, int *srelo,
int *spc, char *term, int pcinc, int src, int cmpr)
{
   char *cp;
   int type = 0;
   int laddr = -1;
   int lr = 0;
   int lrelo = 0;
   int val;
   int lpc = 0;
   char lterm;

   while (isspace(*bp)) bp++;
   lpc = *spc;
   *ts = 0;
   cp = checkexpr (bp, &type);
   if (*bp == MEMSYM || type == MEMEXPR)
   {
      char *ep;

      if (*bp == MEMSYM) bp++;
      *ts = 0x2;
      lpc += 2;
      cp = bp;
      while (*cp && !isspace(*cp) && *cp != ',') cp++;
      if ((cp - bp) == 0)
      {
	 logerror ("Missing memory reference");
      }
      ep = cp;

      /*
      ** Isolate parens for indexing, there may be parens in addr expr.
      */
      if (*(cp-1) == ')')
      {
	 cp --;
	 *cp = '\0';
	 cp --;
	 while (*cp != '(') cp--;
	 *cp++ = '\0';
         cp = exprscan (cp, &lr, &lterm, &val, 2, FALSE, 0);
      }
      bp = exprscan (bp, &laddr, &lterm, &lrelo, 2, FALSE, pcinc);
      laddr = laddr & 0xFFFF;
      lterm = *ep++;
      bp = ep;
   }
   else if (*bp == LITERALSYM)
   {
      if (txmiramode)
      {
	 while (*bp && !isspace(*bp) && *bp != ',') bp++;
	 *term = *bp++;
         logerror ("Literals are not supported in TXMIRA mode");
	 return (bp);
      }

      if (src || cmpr)
      {
	 char *ep;

	 *ts = 0x2;
	 lpc += 2;
	 cp = bp;
	 while (*cp && !isspace(*cp) && *cp != ',') cp++;
	 ep = cp;
	 bp = exprscan (bp, &laddr, &lterm, &lrelo, 2, FALSE, pcinc);
	 laddr = laddr & 0xFFFF;
	 lterm = *ep++;
	 bp = ep;
      }
      else
      {
	 logerror ("Literal not allowed for destination operand");
      }
   }
   else
   {
      if (*bp == '*')
      {
	 bp++;
	 *ts = 0x1;
	 cp = bp;
	 while (!isspace(*cp) && *cp != ',') cp++;
	 if (*(cp-1) == '+')
	 {
	    *ts = 0x3;
	    *(cp-1) = '\0';
	    bp = exprscan (bp, &lr, &lterm, &val, 2, FALSE, 0);
	    lterm = *cp++;
	    bp = cp;
	 }
	 else
	    bp = exprscan (bp, &lr, &lterm, &val, 2, FALSE, 0);
      }
      else
	 bp = exprscan (bp, &lr, &lterm, &val, 2, FALSE, 0);
   }

   if (lr < 0 || lr > 15)
   {
      sprintf (errtmp, "Invalid register value: %d", lr);
      logerror (errtmp);
      lr = 0;
   }

   *saddr = laddr;
   *tr = lr;
   *srelo = lrelo;
   *spc = lpc;
   *term = lterm;

   return (bp);
}

/***********************************************************************
* registerarg - Process register operand arg.
***********************************************************************/

static char *
registerarg (char *bp, int *reg, char *term)
{
   int rd;
   int relo;
   char lt;

   while (isspace(*bp)) bp++;
   if (*bp == MEMSYM || *bp == LITERALSYM)
   {
	 sprintf (errtmp, "Not a register: %s", bp);
	 logerror (errtmp);
	 rd = 0;
   }
   else
   {

      bp = exprscan (bp, &rd, &lt, &relo, 2, FALSE, 0);
      if (rd < 0 || rd > 15)
      {
	 sprintf (errtmp, "Invalid register value: %d", rd);
	 logerror (errtmp);
	 rd = 0;
      }
   }
   *reg = rd;
   *term = lt;
   return (bp);
}

/***********************************************************************
* countarg - Process count operand arg.
***********************************************************************/

static char *
countarg (char *bp, int *cnt, int size, char *type, char *term)
{
   int ct;
   int junk;
   char lt;

   while (isspace(*bp)) bp++;
   if (*bp == MEMSYM || *bp == LITERALSYM)
   {
	 sprintf (errtmp, "Invalid %s field: %s", type, bp);
	 logerror (errtmp);
	 ct = 0;
   }
   else
   {

      bp = exprscan (bp, &ct, &lt, &junk, 2, FALSE, 0);
      if (ct < 0 || ct > size)
      {
	 sprintf (errtmp, "Invalid %s value: %d", type, ct);
	 logerror (errtmp);
	 ct = 0;
      }
   }
   *cnt = ct;
   *term = lt;
   return (bp);
}

/***********************************************************************
* outputoperand - Output operand.
***********************************************************************/

static void
outputoperand (FILE *outfd, FILE *lstfd, char *extname, int incr, int addr,
	       int relo, int segtype, int segnum)
{
   char rchar;

   if (!printed)
   {
      printed = TRUE;
      printline (lstfd);
   }

   rchar = (relo > 0) ? relochar : ' ';
   if (segtype)
      rchar = (segtype & DSEGSYM) ? '"' : '+';

   sprintf (opbuf, OP1FORMAT, addr, rchar);
   printdata (lstfd, pc + incr);

   if (segtype & DSEGSYM)
      sprintf (objbuf, OBJFORMAT, DSEGDATA_TAG, addr);
   else if (segtype & CSEGSYM)
      sprintf (objbuf, CMNOBJFORMAT, CMNDATA_TAG, addr, segnum);
   else if (relo < 0)
      processextndx (objbuf, addr, extname, outfd);
   else 
      sprintf (objbuf, OBJFORMAT, relo ? RELDATA_TAG : ABSDATA_TAG, addr);

   punchrecord (outfd);
}

/***********************************************************************
* p201op - Process type 1 operation code.
***********************************************************************/

static void
p201op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int td = 0;
   int ts = 0;
   int rd = 0;
   int rs = 0;
   int daddr = -1;
   int saddr = -1;
   int srelo;
   int drelo;
   int ssegtype;
   int dsegtype;
   int ssegnum;
   int dsegnum;
   int val;
   int spc;
   int incr;
   char term;
   char srcextname[MAXSYMLEN+2];
   char dstextname[MAXSYMLEN+2];

   sprintf (pcbuf, PCFORMAT, pc);
   spc = 0;
   bp = generalarg (op, bp, &ts, &rs, &saddr, &srelo, &spc, &term, spc + 2,
		    TRUE, FALSE);
   strcpy (srcextname, extname);
   ssegtype = segvaltype;
   ssegnum = csegval;

   if (term == ',')
   {
      int cmpr;

      if (op->opvalue == 0x8000 || op->opvalue == 0x9000)
	 cmpr = TRUE;
      else
	 cmpr = FALSE;
      if (txmiramode && isspace (*bp))
	 logerror ("Syntax error, space follows comma");
      bp = generalarg (op, bp, &td, &rd, &daddr, &drelo, &spc, &term, spc + 2,
		       FALSE, cmpr);
      strcpy (dstextname, extname);
      dsegtype = segvaltype;
      dsegnum = csegval;
   }

   else
   {
      logerror ("Missing destination operand");
   }

   val = op->opvalue | (td << 10) | (rd << 6) | (ts << 4) | rs;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   incr = 0;
   if (saddr >= 0)
   {
      incr += 2;
      outputoperand (outfd, lstfd, srcextname, incr, saddr, srelo,
		     ssegtype, ssegnum);
   }
   if (daddr >= 0)
   {
      incr += 2;
      outputoperand (outfd, lstfd, dstextname, incr, daddr, drelo,
		     dsegtype, dsegnum);
   }
   pc += 2 + spc;
}

/***********************************************************************
* p202op - Process type 2 operation code.
***********************************************************************/

static void
p202op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int addr = 0;
   int val;
   int relocatable;
   int disp;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);

   bp = exprscan (bp, &addr, &term, &relocatable, 2, FALSE, 0);
   if (op->opmod == 0) /* A Jump instruction */
   {
      if (*bp == MEMSYM || *bp == LITERALSYM)
      {
	 sprintf (errtmp, "Not a valid jump operand: %s", bp);
	 logerror (errtmp);
	 disp = 0;
      }
      else
      {
	 disp = addr/2 - (pc+2)/2;
	 if (disp < -128 || disp > 128)
	 {
	       sprintf (errtmp, "Jump displacement too large: %d", disp);
	       logerror (errtmp);
	       disp = 0;
	 }
      }
   }
   else	/* A CRU instruction */
   {
      disp = addr;
   }
   val = op->opvalue | (disp & 0xFF);
   sprintf (opbuf, OP1FORMAT, val, ' ');
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);
   pc += 2;
}

/***********************************************************************
* p203op - Process type 3 operation code.
***********************************************************************/

static void
p203op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int ts = 0;
   int rs = 0;
   int rd = 0;
   int saddr = -1;
   int srelo;
   int ssegtype;
   int ssegnum;
   int val;
   int spc;
   char term;
   char sextname[MAXSYMLEN+2];

   sprintf (pcbuf, PCFORMAT, pc);
   spc = 0;
   bp = generalarg (op, bp, &ts, &rs, &saddr, &srelo, &spc, &term, 2,
		    TRUE, FALSE);
   strcpy (sextname, extname);
   ssegtype = segvaltype;
   ssegnum = csegval;
   if (term == ',')
   {
      if (txmiramode && isspace (*bp))
	 logerror ("Syntax error, space follows comma");
      bp = registerarg (bp, &rd, &term);
   }
   else
   {
      logerror ("Missing register operand");
   }

   val = op->opvalue | (rd << 6) | (ts << 4) | rs;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   if (saddr >= 0)
   {
      outputoperand (outfd, lstfd, sextname, 2, saddr, srelo,
		     ssegtype, ssegnum);
   }
   pc += 2 + spc;
}

/***********************************************************************
* p204op - Process type 4 operation code.
***********************************************************************/

static void
p204op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int ts = 0;
   int rs = 0;
   int count = 0;
   int saddr = -1;
   int srelo;
   int ssegtype;
   int ssegnum;
   int val;
   int spc;
   char term;
   char sextname[MAXSYMLEN+2];

   sprintf (pcbuf, PCFORMAT, pc);
   spc = 0;
   bp = generalarg (op, bp, &ts, &rs, &saddr, &srelo, &spc, &term, 2,
		    TRUE, FALSE);
   strcpy (sextname, extname);
   ssegtype = segvaltype;
   ssegnum = csegval;
   if (term == ',')
   {
      if (txmiramode && isspace (*bp))
	 logerror ("Syntax error, space follows comma");
      bp = countarg (bp, &count, 15, "transfer count", &term);
   }
   else
   {
      logerror ("Missing count operand");
   }

   val = op->opvalue | (count << 6) | (ts << 4) | rs;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   if (saddr >= 0)
   {
      outputoperand (outfd, lstfd, sextname, 2, saddr, srelo,
		     ssegtype, ssegnum);
   }
   pc += 2 + spc;
}

/***********************************************************************
* p205op - Process type 5 operation code.
***********************************************************************/

static void
p205op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int r = 0;
   int c = 0;
   int val;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   bp = registerarg (bp, &r, &term);
   if (term == ',')
   {
      if (txmiramode && isspace (*bp))
	 logerror ("Syntax error, space follows comma");
      bp = countarg (bp, &c, 15, "shift count", &term);
   }
   else
   {
      logerror ("Missing shift count operand");
   }

   val = op->opvalue | (c << 4) | r;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);
   pc += 2;
}

/***********************************************************************
* p206op - Process type 6 operation code. 
***********************************************************************/

static void
p206op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int ts = 0;
   int rs = 0;
   int saddr = -1;
   int srelo;
   int val;
   int spc;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   spc = 0;
   bp = generalarg (op, bp, &ts, &rs, &saddr, &srelo, &spc, &term, 2,
		    TRUE, FALSE);

   val = op->opvalue | (ts << 4) | rs;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   if (saddr >= 0)
   {
      outputoperand (outfd, lstfd, extname, 2, saddr, srelo,
		     segvaltype, csegval);
   }
   pc += 2 + spc;
}

/***********************************************************************
* p207op - Process type 7 operation code. 
***********************************************************************/

static void
p207op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   sprintf (pcbuf, PCFORMAT, pc);
   sprintf (opbuf, OP1FORMAT, op->opvalue, ' ');
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, op->opvalue);
   punchrecord (outfd);
   pc += 2;
}

/***********************************************************************
* p208op - Process type 8 operation code. 
***********************************************************************/

static void
p208op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int r = 0;
   int i = 0;
   int relo;
   int val;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   val = op->opvalue;

   if (op->opmod == 1) /* Instructions without register operand */
   {
      sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
      goto NOREG;
   }

   bp = registerarg (bp, &r, &term);
   val = op->opvalue | r;
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);

   if (op->opmod == 2) /* Instructions with only register operand */
   {
      sprintf (opbuf, OP1FORMAT, val, ' ');
      punchrecord (outfd);
      pc += 2;
      return;
   }

   if (term == ',')
   {
      if (txmiramode)
      {
	 if (isspace (*bp))
	    logerror ("Syntax error, space follows comma");
         else if (*bp == MEMSYM)
	    logerror ("Syntax error, '@' in value");
         else if (*bp == LITERALSYM)
	    logerror ("Literals are not supported in TXMIRA mode");
      }
      if (*bp == MEMSYM) bp++;
NOREG:
      bp = exprscan (bp, &i, &term, &relo, 2, FALSE, 2);
      if (i < -65634 || i > 65535)
      {
	 sprintf (errtmp, "Invalid immediate value: %d", i);
	 logerror (errtmp);
	 i = 0;
      }
      i &= 0xFFFF;
   }
   else
   {
      logerror ("Missing immediate operand");
   }

   punchrecord (outfd);
   sprintf (opbuf, OP1FORMAT, val, ' ');

   outputoperand (outfd, lstfd, extname, 2, i, relo, segvaltype, csegval);
   pc += 4;
}

/***********************************************************************
* p209op - Process type 9 operation code. 
***********************************************************************/

static void
p209op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int ts = 0;
   int rs = 0;
   int rd = 0;
   int saddr = -1;
   int srelo;
   int ssegtype;
   int ssegnum;
   int val;
   int spc;
   char term;
   char sextname[MAXSYMLEN+2];

   sprintf (pcbuf, PCFORMAT, pc);
   spc = 0;
   bp = generalarg (op, bp, &ts, &rs, &saddr, &srelo, &spc, &term, 2,
		    TRUE, FALSE);
   strcpy (sextname, extname);
   ssegtype = segvaltype;
   ssegnum = csegval;
   if (term == ',')
   {
      if (txmiramode && isspace (*bp))
	 logerror ("Syntax error, space follows comma");
      bp = registerarg (bp, &rd, &term);
   }
   else
   {
      logerror ("Missing register operand");
   }

   val = op->opvalue | (rd << 6) | (ts << 4) | rs;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   if (saddr >= 0)
   {
      outputoperand (outfd, lstfd, sextname, 2, saddr, srelo,
		     ssegtype, ssegnum);
   }
   pc += 2 + spc;
}

/***********************************************************************
* p210op - Process type 10 operation code. 
***********************************************************************/

static void
p210op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int rs = 0;
   int rd = 0;
   int junk;
   int val;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   bp = registerarg (bp, &rs, &term);
   if (term == ',')
   {
      if (txmiramode && isspace (*bp))
      {
	 logerror ("Syntax error, space follows comma");
	 while (isspace (*bp)) bp++;
      }
      if (*bp == MEMSYM || *bp == LITERALSYM)
      {
	    sprintf (errtmp, "Not a map file: %s", bp);
	    logerror (errtmp);
	    rd = 0;
      }
      else
      {
	 bp = exprscan (bp, &rd, &term, &junk, 2, FALSE, 0);
	 if (rd < 0 || rd > 1)
	 {
	    sprintf (errtmp, "Invalid map file value: %d", rd);
	    logerror (errtmp);
	    rd = 0;
	 }
      }
   }
   else
   {
      logerror ("Missing map file operand");
   }

   val = op->opvalue | (rd << 4) | rs;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   pc += 2;
}

/***********************************************************************
* p211op - Process type 11 operation code.
***********************************************************************/

static void
p211op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int td = 0;
   int ts = 0;
   int rd = 0;
   int rs = 0;
   int bc = 0;
   int daddr = -1;
   int saddr = -1;
   int srelo;
   int drelo;
   int ssegtype;
   int dsegtype;
   int ssegnum;
   int dsegnum;
   int val;
   int spc;
   int incr;
   char term;
   char srcextname[MAXSYMLEN+2];
   char dstextname[MAXSYMLEN+2];

   sprintf (pcbuf, PCFORMAT, pc);
   spc = 0;
   bp = generalarg (op, bp, &ts, &rs, &saddr, &srelo, &spc, &term, spc + 4,
		    TRUE, FALSE);
   strcpy (srcextname, extname);
   ssegtype = segvaltype;
   ssegnum = csegval;
   if (term == ',')
   {
      bp = generalarg (op, bp, &td, &rd, &daddr, &drelo, &spc, &term, spc + 4,
		       FALSE, FALSE);
      strcpy (dstextname, extname);
      dsegtype = segvaltype;
      dsegnum = csegval;
      if (term == ',')
      {
	 bp = countarg (bp, &bc, 15, "byte count", &term);
      }
   }
   else
   {
      logerror ("Missing destination operand");
   }

   val = op->opvalue;
   printed = TRUE;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printline (lstfd);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   val = (bc << 12) | (td << 10) | (rd << 6) | (ts << 4) | rs;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printdata (lstfd, pc + 2);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   incr = 2;
   if (saddr >= 0)
   {
      incr += 2;
      outputoperand (outfd, lstfd, srcextname, incr, saddr, srelo,
		     ssegtype, ssegnum);
   }
   if (daddr >= 0) 
   {
      incr += 2;
      outputoperand (outfd, lstfd, dstextname, incr, daddr, drelo,
		     dsegtype, dsegnum);
   }
   pc += 4 + spc;
}

/***********************************************************************
* p212op - Process type 12 operation code.
***********************************************************************/

static void
p212op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int td = 0;
   int ts = 0;
   int rd = 0;
   int rs = 0;
   int bc = 0;
   int ck = -1;
   int daddr = -1;
   int saddr = -1;
   int srelo;
   int drelo;
   int ssegtype;
   int dsegtype;
   int ssegnum;
   int dsegnum;
   int val;
   int spc;
   int incr;
   char term;
   char srcextname[MAXSYMLEN+2];
   char dstextname[MAXSYMLEN+2];

   sprintf (pcbuf, PCFORMAT, pc);
   spc = 0;
   bp = generalarg (op, bp, &ts, &rs, &saddr, &srelo, &spc, &term, spc + 4,
		    TRUE, FALSE);
   strcpy (srcextname, extname);
   ssegtype = segvaltype;
   ssegnum = csegval;
   if (term == ',')
   {
      bp = generalarg (op, bp, &td, &rd, &daddr, &drelo, &spc, &term, spc + 4,
		       FALSE, FALSE);
      strcpy (dstextname, extname);
      dsegtype = segvaltype;
      dsegnum = csegval;
      if (term == ',')
      {
	 bp = countarg (bp, &bc, 15, "byte count", &term);
	 if (term == ',')
	 {
	    bp = registerarg (bp, &ck, &term);
	 }
      }
   }
   else
   {
      logerror ("Missing destination operand");
   }
   if (ck < 0)
   {
      if (ckpt < 0)
      {
	 logerror ("Missing checkpoint operand");
	 ck = 0;
      }
      else ck = ckpt;
   }

   val = op->opvalue | ck;
   printed = TRUE;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printline (lstfd);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   val = (bc << 12) | (td << 10) | (rd << 6) | (ts << 4) | rs;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printdata (lstfd, pc + 2);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   incr = 2;
   if (saddr >= 0)
   {
      incr += 2;
      outputoperand (outfd, lstfd, srcextname, incr, saddr, srelo,
		     ssegtype, ssegnum);
   }
   if (daddr >= 0) 
   {
      incr += 2;
      outputoperand (outfd, lstfd, dstextname, incr, daddr, drelo,
		     dsegtype, dsegnum);
   }
   pc += 4 + spc;
}

/***********************************************************************
* p213op - Process type 13 operation code.
***********************************************************************/

static void
p213op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int ts = 0;
   int rs = 0;
   int bc = 0;
   int sc = 0;
   int saddr = -1;
   int srelo;
   int ssegtype;
   int ssegnum;
   int val;
   int spc;
   char term;
   char sextname[MAXSYMLEN+2];

   sprintf (pcbuf, PCFORMAT, pc);
   spc = 0;
   bp = generalarg (op, bp, &ts, &rs, &saddr, &srelo, &spc, &term, spc + 4,
		    TRUE, FALSE);
   strcpy (sextname, extname);
   ssegtype = segvaltype;
   ssegnum = csegval;
   if (term == ',')
   {
      bp = countarg (bp, &bc, 15, "byte count", &term);
      if (term == ',')
      {
	 bp = countarg (bp, &sc, 15, "shift count", &term);
      }
   }

   val = op->opvalue;
   printed = TRUE;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printline (lstfd);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   val = (bc << 12) | (sc << 6) | (ts << 4) | rs;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printdata (lstfd, pc + 2);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   if (saddr >= 0) 
   {
      outputoperand (outfd, lstfd, sextname, 4, saddr, srelo,
		     ssegtype, ssegnum);
   }
   pc += 4 + spc;
}

/***********************************************************************
* p214op - Process type 14 operation code.
***********************************************************************/

static void
p214op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int ts = 0;
   int rs = 0;
   int pos = 0;
   int saddr = -1;
   int srelo;
   int ssegtype;
   int ssegnum;
   int val;
   int spc;
   char term;
   char sextname[MAXSYMLEN+2];

   sprintf (pcbuf, PCFORMAT, pc);
   spc = 0;
   bp = generalarg (op, bp, &ts, &rs, &saddr, &srelo, &spc, &term, spc + 4,
		    TRUE, FALSE);
   strcpy (sextname, extname);
   ssegtype = segvaltype;
   ssegnum = csegval;
   if (term == ',')
   {
      bp = countarg (bp, &pos, 0x3FF, "position", &term);
   }

   val = op->opvalue;
   printed = TRUE;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printline (lstfd);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   val = (pos << 6) | (ts << 4) | rs;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printdata (lstfd, pc + 2);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   if (saddr >= 0) 
   {
      outputoperand (outfd, lstfd, sextname, 4, saddr, srelo,
		     ssegtype, ssegnum);
   }
   pc += 4 + spc;
}

/***********************************************************************
* p215op - Process type 15 operation code.
***********************************************************************/

static void
p215op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int ts = 0;
   int rs = 0;
   int pos = 0;
   int wid = 0;
   int saddr = -1;
   int srelo;
   int ssegtype;
   int ssegnum;
   int val;
   int spc;
   char term;
   char sextname[MAXSYMLEN+2];

   sprintf (pcbuf, PCFORMAT, pc);
   spc = 0;
   bp = generalarg (op, bp, &ts, &rs, &saddr, &srelo, &spc, &term, spc + 4,
		    TRUE, FALSE);
   strcpy (sextname, extname);
   ssegtype = segvaltype;
   ssegnum = csegval;
   if (term == ',')
   {
      while (isspace (*bp)) bp++;
      if (*bp == '(')
      {
	 bp++;
	 bp = countarg (bp, &pos, 15, "position", &term);
	 if (term == ',')
	 {
	    char *cp;

	    cp = bp;
	    while (*bp && *bp != ')') bp++;
	    if (*bp != ')')
	    {
	       logerror ("Invalid width operand");
	    }
	    else
	    {
	       *bp = '\0';
	       bp = countarg (cp, &wid, 15, "width", &term);
	    }
	 }
	 else
	 {
	    logerror ("Missing width operand");
	 }
      }
      else
      {
	 logerror ("Invalid position operand");
      }
   }
   else
   {
      logerror ("Missing position operand");
   }

   val = op->opvalue | wid;
   printed = TRUE;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printline (lstfd);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   val = (pos << 12) | (ts << 4) | rs;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printdata (lstfd, pc + 2);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   if (saddr >= 0) 
   {
      outputoperand (outfd, lstfd, sextname, 4, saddr, srelo,
		     ssegtype, ssegnum);
   }
   pc += 4 + spc;
}

/***********************************************************************
* p216op - Process type 16 operation code.
***********************************************************************/

static void
p216op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int ts = 0;
   int rs = 0;
   int td = 0;
   int rd = 0;
   int pos = 0;
   int wid = 0;
   int saddr = -1;
   int daddr = -1;
   int srelo;
   int drelo;
   int ssegtype;
   int dsegtype;
   int ssegnum;
   int dsegnum;
   int val;
   int spc;
   int incr;
   char term;
   char srcextname[MAXSYMLEN+2];
   char dstextname[MAXSYMLEN+2];

   sprintf (pcbuf, PCFORMAT, pc);
   spc = 0;
   bp = generalarg (op, bp, &ts, &rs, &saddr, &srelo, &spc, &term, spc + 4,
		    TRUE, FALSE);
   strcpy (srcextname, extname);
   ssegtype = segvaltype;
   ssegnum = csegval;
   if (term == ',')
   {
      bp = generalarg (op, bp, &td, &rd, &daddr, &drelo, &spc, &term, spc + 4,
		       FALSE, FALSE);
      strcpy (dstextname, extname);
      dsegtype = segvaltype;
      dsegnum = csegval;
      if (term == ',')
      {
	 while (isspace (*bp)) bp++;
	 if (*bp == '(')
	 {
	    bp++;
	    bp = countarg (bp, &pos, 15, "position", &term);
	    if (term == ',')
	    {
	       char *cp;

	       cp = bp;
	       while (*bp && *bp != ')') bp++;
	       if (*bp != ')')
	       {
		  logerror ("Invalid width operand");
	       }
	       else
	       {
		  *bp = '\0';
		  bp = countarg (cp, &wid, 15, "width", &term);
	       }
	    }
	    else
	    {
	       logerror ("Missing width operand");
	    }
	 }
	 else
	 {
	    logerror ("Invalid position operand");
	 }
      }
      else
      {
	 logerror ("Missing position operand");
      }
   }
   else
   {
      logerror ("Missing destination operand");
   }

   val = op->opvalue | wid;
   printed = TRUE;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printline (lstfd);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   val = (pos << 12) | (td << 10) | (rd << 6) | (ts << 4) | rs;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printdata (lstfd, pc + 2);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   incr = 2;
   if (saddr >= 0)
   {
      incr += 2;
      outputoperand (outfd, lstfd, srcextname, incr, saddr, srelo,
		     ssegtype, ssegnum);
   }
   if (daddr >= 0) 
   {
      incr += 2;
      outputoperand (outfd, lstfd, dstextname, incr, daddr, drelo,
		     dsegtype, dsegnum);
   }
   pc += 4 + spc;
}

/***********************************************************************
* p217op - Process type 17 operation code.
***********************************************************************/

static void
p217op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int addr = 0;
   int disp = 0;
   int r = 0;
   int c = 0;
   int relocatable;
   int val;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   bp = exprscan (bp, &addr, &term, &relocatable, 2, FALSE, 0);
   while (isspace (*bp)) bp++;

   if (*bp == MEMSYM || *bp == LITERALSYM)
   {
      sprintf (errtmp, "Not a valid jump operand: %s", bp);
      logerror (errtmp);
      disp = 0;
   }
   else
   {
      disp = addr/2 - (pc+4)/2;
      if (disp < -128 || disp > 128)
      {
	    sprintf (errtmp, "Jump displacement too large: %d", disp);
	    logerror (errtmp);
	    disp = 0;
      }
   }
   if (term == ',')
   {
      bp = countarg (bp, &c, 15, "count", &term);
      if (term == ',')
      {
	 bp = registerarg (bp, &r, &term);
      }
      else
      {
	 logerror ("Missing register operand");
      }
   }
   else
   {
      logerror ("Missing count operand");
   }

   val = op->opvalue;
   printed = TRUE;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printline (lstfd);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   val = (c << 12) | (r << 8) | (disp & 0xFF);
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printdata (lstfd, pc + 2);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   pc += 4;
}

/***********************************************************************
* p218op - Process type 18 operation code.
***********************************************************************/

static void
p218op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int r = 0;
   int val;
   char term;

   sprintf (pcbuf, PCFORMAT, pc);
   bp = registerarg (bp, &r, &term);

   val = op->opvalue | r;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);
   pc += 2;
}

/***********************************************************************
* p219op - Process type 19 operation code.
***********************************************************************/

static void
p219op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int td = 0;
   int ts = 0;
   int rd = 0;
   int rs = 0;
   int daddr = -1;
   int saddr = -1;
   int srelo;
   int drelo;
   int ssegtype;
   int dsegtype;
   int ssegnum;
   int dsegnum;
   int val;
   int spc;
   int incr;
   char term;
   char srcextname[MAXSYMLEN+2];
   char dstextname[MAXSYMLEN+2];

   sprintf (pcbuf, PCFORMAT, pc);
   spc = 0;
   bp = generalarg (op, bp, &ts, &rs, &saddr, &srelo, &spc, &term, spc + 4,
		    TRUE, FALSE);
   strcpy (srcextname, extname);
   ssegtype = segvaltype;
   ssegnum = csegval;
   if (term == ',')
   {
      bp = generalarg (op, bp, &td, &rd, &daddr, &drelo, &spc, &term, spc + 4,
		       FALSE, FALSE);
      strcpy (dstextname, extname);
      dsegtype = segvaltype;
      dsegnum = csegval;
   }
   else
   {
      logerror ("Missing destination operand");
   }

   val = op->opvalue;
   printed = TRUE;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printline (lstfd);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   val = (td << 10) | (rd << 6) | (ts << 4) | rs;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printdata (lstfd, pc + 2);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   incr = 2;
   if (saddr >= 0)
   {
      incr += 2;
      outputoperand (outfd, lstfd, srcextname, incr, saddr, srelo,
		     ssegtype, ssegnum);
   }
   if (daddr >= 0) 
   {
      incr += 2;
      outputoperand (outfd, lstfd, dstextname, incr, daddr, drelo,
		     dsegtype, dsegnum);
   }
   pc += 4 + spc;
}

/***********************************************************************
* p220op - Process type 20 operation code.
***********************************************************************/

static void
p220op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   char *token;
   int tokentype;
   int td = 0;
   int ts = 0;
   int rd = 0;
   int rs = 0;
   int cond = -1;
   int daddr = -1;
   int saddr = -1;
   int srelo;
   int drelo;
   int ssegtype;
   int dsegtype;
   int ssegnum;
   int dsegnum;
   int val;
   int spc;
   int incr;
   char term;
   char srcextname[MAXSYMLEN+2];
   char dstextname[MAXSYMLEN+2];

   sprintf (pcbuf, PCFORMAT, pc);
   spc = 0;
   bp = tokscan (bp, &token, &tokentype, &val, &term);
   if (tokentype == SYM)
   {
      if (!strcmp (token, "EQ")) cond = 0;
      else if (!strcmp (token, "NE")) cond = 1;
      else if (!strcmp (token, "HE")) cond = 2;
      else if (!strcmp (token, "L")) cond = 3;
      else if (!strcmp (token, "GE")) cond = 4;
      else if (!strcmp (token, "LT")) cond = 5;
      else if (!strcmp (token, "LE")) cond = 6;
      else if (!strcmp (token, "H")) cond = 7;
      else if (!strcmp (token, "LTE")) cond = 8;
      else if (!strcmp (token, "GT")) cond = 9;
      else
      {
	 sprintf (errtmp, "Invalid condition code: %s", token);
	 logerror (errtmp);
	 cond = 0;
      }
   }
   else if (tokentype == DECNUM)
   {
      cond = val;
   }
   if (cond < 0 || cond > 9)
   {
      sprintf (errtmp, "Invalid condition code: %s", token);
      logerror (errtmp);
      cond = 0;
   }
   if (term == ',')
   {
      bp = generalarg (op, bp, &ts, &rs, &saddr, &srelo, &spc, &term, spc + 4,
		       TRUE, FALSE);
      strcpy (srcextname, extname);
      ssegtype = segvaltype;
      ssegnum = csegval;
      if (term == ',')
      {
	 bp = generalarg (op, bp, &td, &rd, &daddr, &drelo, &spc, &term, spc+4,
			  FALSE, FALSE);
	 strcpy (dstextname, extname);
	 dsegtype = segvaltype;
	 dsegnum = csegval;
      }
      else
      {
	 logerror ("Missing destination operand");
      }
   }
   else
   {
      logerror ("Missing source operand");
   }

   val = op->opvalue;
   printed = TRUE;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printline (lstfd);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   val = (cond << 12) | (td << 10) | (rd << 6) | (ts << 4) | rs;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printdata (lstfd, pc + 2);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   incr = 2;
   if (saddr >= 0)
   {
      incr += 2;
      outputoperand (outfd, lstfd, srcextname, incr, saddr, srelo,
		     ssegtype, ssegnum);
   }
   if (daddr >= 0) 
   {
      incr += 2;
      outputoperand (outfd, lstfd, dstextname, incr, daddr, drelo,
		     dsegtype, dsegnum);
   }
   pc += 4 + spc;
}

/***********************************************************************
* p221op - Process type 21 operation code.
***********************************************************************/

static void
p221op (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   int td = 0;
   int rd = 0;
   int cd = 0;
   int daddr = -1;
   int ts = 0;
   int rs = 0;
   int cs = 0;
   int saddr = -1;
   int srelo;
   int drelo;
   int ssegtype;
   int dsegtype;
   int ssegnum;
   int dsegnum;
   int val;
   int spc;
   int incr;
   char term;
   char srcextname[MAXSYMLEN+2];
   char dstextname[MAXSYMLEN+2];

   sprintf (pcbuf, PCFORMAT, pc);
   spc = 0;
   bp = generalarg (op, bp, &ts, &rs, &saddr, &srelo, &spc, &term, spc + 4,
		    TRUE, FALSE);
   strcpy (srcextname, extname);
   ssegtype = segvaltype;
   ssegnum = csegval;
   if (term == ',')
   {
      bp = generalarg (op, bp, &td, &rd, &daddr, &drelo, &spc, &term, spc + 4,
		       FALSE, FALSE);
      strcpy (dstextname, extname);
      dsegtype = segvaltype;
      dsegnum = csegval;
      if (term == ',')
      {
         bp = countarg (bp, &cs, 15, "source count", &term);
	 if (term == ',')
	 {
	    bp = countarg (bp, &cd, 15, "destination count", &term);
	 }
      }
   }
   else
   {
      logerror ("Missing destination operand");
   }

   val = op->opvalue | cd;
   printed = TRUE;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printline (lstfd);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   val = (cs << 12) | (td << 10) | (rd << 6) | (ts << 4) | rs;
   sprintf (opbuf, OP1FORMAT, val, ' ');
   printdata (lstfd, pc + 2);
   sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
   punchrecord (outfd);

   incr = 2;
   if (saddr >= 0)
   {
      incr += 2;
      outputoperand (outfd, lstfd, srcextname, incr, saddr, srelo,
		     ssegtype, ssegnum);
   }
   if (daddr >= 0) 
   {
      incr += 2;
      outputoperand (outfd, lstfd, dstextname, incr, daddr, drelo,
		     dsegtype, dsegnum);
   }
   pc += 4 + spc;
}

/***********************************************************************
* processoption - Process options.
***********************************************************************/

int
processoption (char *bp, int incode)
{
   char *token;
   char *oldbuf;
   int tokentype;
   int val;
   int ret;
   char term;

   oldbuf = pscanbuf;
   pscanbuf = bp;

   ret = 0;
   do {
      bp = tokscan (bp, &token, &tokentype, &val, &term);
      if (tokentype == SYM)
      {
	 char *cp;
	 int j;

	 for (cp = token; *cp; cp++) 
	    if (islower(*cp)) *cp = toupper(*cp);

	 j = strlen (token);
	 if (!(strncmp (token, "BUNLST", j))) bunlist = TRUE;
	 else if (!(strncmp (token, "BUNLIST", j))) bunlist = TRUE;
	 else if (!(strncmp (token, "DUNLST", j))) dunlist = TRUE;
	 else if (!(strncmp (token, "DUNLIST", j))) dunlist = TRUE;
	 else if (!(strncmp (token, "MUNLST", j))) munlist = TRUE;
	 else if (!(strncmp (token, "MUNLIST", j))) munlist = TRUE;
	 else if (!(strncmp (token, "TUNLST", j))) tunlist = TRUE;
	 else if (!(strncmp (token, "TUNLIST", j))) tunlist = TRUE;
	 else if (!(strncmp (token, "NOLIST", j))) listmode = FALSE;
	 else if (!(strncmp (token, "SYMT", j))) gensymt = TRUE;
	 else if (!(strncmp (token, "XREF", j))) genxref = TRUE;
	 else goto BADOPTIONTOKEN;
      }
      else if (tokentype == DECNUM)
      {
	 if (val == 10) model = 10;
	 else if (val == 12) model = 12;
	 else
	 {
	    sprintf (token, "%d", val);
	    goto BADOPTIONTOKEN;
	 }
      }
      else
      {
      BADOPTIONTOKEN:
	 sprintf (errtmp, "Invalid OPTION token: %s", token);
	 if (incode)
	    logerror (errtmp);
	 else
	    fprintf (stderr, "%s\n", errtmp);
         ret = -1;
      }
   } while (term == ',');

   pscanbuf = oldbuf;
   return (ret);
}

/***********************************************************************
* checkpc - Check for ODD pc.
***********************************************************************/

static void 
checkpc (FILE *outfd)
{
   int opc;

   opc = pc;
   if (pc & 0x0001) pc++;
   sprintf (pcbuf, PCFORMAT, pc);
   if (pc != opc)
   {
      if (incseg)
	 sprintf (objbuf, CMNOBJFORMAT, CMNORG_TAG,
		  pc, csegs[curcseg].number);
      else if (indseg)
	 sprintf (objbuf, OBJFORMAT, DSEGORG_TAG, pc);
      else
	 sprintf (objbuf, OBJFORMAT, absolute ? ABSORG_TAG : RELORG_TAG, pc);
      punchrecord (outfd);
   }
}

/***********************************************************************
* p2pop - Process Pseudo operation code.
***********************************************************************/

static int
p2pop (OpCode *op, char *bp, FILE *outfd, FILE *lstfd)
{
   SymNode *s;
   char *cp;
   char *token;
   int i, j, k, l;
   int opc;
   int val;
   int oval;
   int relocatable;
   int tokentype;
   int oddbss;
   char srelochar;
   char term;
   char temp[10];


   sprintf (pcbuf, PCFORMAT, pc);

   /*
   ** If word based op and "hanging" byte, punch it.
   ** For TEXT/BYTE ops output RELORG if needed.
   */

   switch (op->opvalue)
   {
   case BES_T:
   case BSS_T:
      oddbss = bytecnt;
   case AORG_T:
   case ASMIF_T:
   case ASMELS_T:
   case ASMEND_T:
   case DATA_T:
   case DORG_T:
   case DOUBLE_T:
   case END_T:
   case EVEN_T:
   case FLOAT_T:
   case LONG_T:
   case NOP_T:
   case QUAD_T:
   case RT_T:
      if (bytecnt)
      {
	 byteacc = (byteacc << 8) | 0;
	 sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, byteacc);
	 punchrecord (outfd);
	 bytecnt = 0;
      }
      break;
   case BYTE_T:
   case TEXT_T:
      if (!orgout && !absolute)
      {
	 if (incseg)
	    sprintf (objbuf, CMNOBJFORMAT, CMNORG_TAG,
	    	     pc & 0xFFFE, csegs[curcseg].number);
	 else
	    sprintf (objbuf, OBJFORMAT, RELORG_TAG, pc & 0xFFFE);
	 orgout = TRUE;
	 punchrecord (outfd);
      }
      break;
   default: ;
   }

   switch (op->opvalue)
   {

   case AORG_T:
      if (incseg)
      {
	 incseg = FALSE;
	 relochar = '\'';
      }
      if (!absolute && !indorg) rorgpc = pc;
      cp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      if (val < 0 || val > 65535)
      {
	 sprintf (errtmp, "Invalid AORG value: %d", val);
	 logerror (errtmp);
	 val = 0;
      }
      indorg = FALSE;
      absolute = TRUE;
      pc = val;
      sprintf (pcbuf, PCFORMAT, pc);
      sprintf (objbuf, OBJFORMAT, ABSORG_TAG, val & 0xFFFE);
      orgout = TRUE;
      break;

   case ASMIF_T:
      strcpy (pcbuf, PCBLANKS);
      bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      inskip = if_start (val);
      break;

   case ASMELS_T:
      strcpy (pcbuf, PCBLANKS);
      inskip = if_else();
      break;

   case ASMEND_T:
      strcpy (pcbuf, PCBLANKS);
      inskip = if_end();
      break;

   case BES_T:
   case BSS_T:
      bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
      if (val < 0 || val > 65535)
      {
	 sprintf (errtmp, "Invalid storage value: %d", val);
	 logerror (errtmp);
	 val = 0;
      }
      if (oddbss)
      {
	 if (incseg)
	    sprintf (objbuf, CMNOBJFORMAT, CMNORG_TAG,
	    	     pc & 0xFFFE, csegs[curcseg].number);
	 else if (indseg)
	    sprintf (objbuf, OBJFORMAT, DSEGORG_TAG, pc & 0xFFFE);
	 else
	    sprintf (objbuf, OBJFORMAT, absolute ? ABSORG_TAG : RELORG_TAG,
		     pc & 0xFFFE);
	 punchrecord (outfd);
      }
      pc += val;
      if (incseg)
	 sprintf (objbuf, CMNOBJFORMAT, CMNORG_TAG,
		  pc & 0xFFFE, csegs[curcseg].number);
      else if (indseg)
	 sprintf (objbuf, OBJFORMAT, DSEGORG_TAG, pc & 0xFFFE);
      else
	 sprintf (objbuf, OBJFORMAT, absolute ? ABSORG_TAG : RELORG_TAG,
		  pc & 0xFFFE);
      if (pc & 1)
      {
         byteacc = 0;
	 bytecnt = 1;
      }
      break;

   case BYTE_T:
      i = j = k = l = 0;
      opc = pc;
      do {
	 bp = exprscan (bp, &val, &term, &relocatable, 1, FALSE, 0);
	 val &= 0xFF;

	 byteacc = byteacc << 8 | val;
	 bytecnt++;
	 sprintf (temp, CHAR1FORMAT, val);
	 strncpy (&opbuf[j], temp, 2);
	 j += 2;
	 pc++;
	 i++;
	 if (bytecnt == 2)
	 {
	    sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, byteacc);
	    punchrecord (outfd);
	    bytecnt = 0;
	    byteacc = 0;
	 }
	 printed = TRUE;
	 if (l++ == 0)
	 {
	    printline (lstfd);
	    opc = pc;
	 }
	 else
	 {
	    if (!bunlist)
	       printdata (lstfd, opc);
	    opc = pc;
	 }
	 strcpy (opbuf, OPBLANKS);
	 j = k = 0;
      
      } while (term == ',');

      break;

   case CKPT_T:
      strcpy (pcbuf, PCBLANKS);
      ckpt = -1;
      bp = registerarg (bp, &val, &term);
      if (errnum == 0) ckpt = val;
      break;

   case COPY_T:
      strcpy (pcbuf, PCBLANKS);
      printed = TRUE;
      printline (lstfd);
      break;
      
   case DATA_T:
      checkpc (outfd);
      opc = pc;
      i = 0;
      do {
	 if (*bp == MEMSYM && !txmiramode) bp++;
	 bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
	 srelochar = relocatable ? '\'' : ' ';
	 if (segvaltype)
	    srelochar = (segvaltype & DSEGSYM) ? '"' : '+';
	 if (val < -65534 || val > 65535)
	 {
	    sprintf (errtmp, "Invalid DATA value: %d", val);
	    logerror (errtmp);
	 }
	 val &= 0xFFFF;
	 sprintf (opbuf, DATA1FORMAT, val, srelochar);

	 if (segvaltype & DSEGSYM)
	    sprintf (objbuf, OBJFORMAT, DSEGDATA_TAG, val);
	 else if (segvaltype & CSEGSYM)
	    sprintf (objbuf, CMNOBJFORMAT, CMNDATA_TAG, val, csegval);
	 else if (relocatable < 0)
	    processextndx (objbuf, val, extname, outfd);
	 else
	    sprintf (objbuf, OBJFORMAT,
		     relocatable ? RELDATA_TAG : ABSDATA_TAG, val);
	 punchrecord (outfd);

	 pc += 2;
	 printed = TRUE;
	 if (i++ == 0)
	 {
	    printline (lstfd);
	 }
	 else
	 {
	    if (!dunlist)
	       printdata (lstfd, opc);
	 }
	 opc = pc;
	 strcpy (opbuf, OPBLANKS);
      } while (term == ',');
      break;

   case DEF_T:
      strcpy (pcbuf, PCBLANKS);
      do {
	 bp = tokscan (bp, &token, &tokentype, &val, &term);
	 if (tokentype != SYM)
	 {
	    sprintf (errtmp, "DEF requires symbol: %s", token);
	    logerror (errtmp);
	 }
	 else
	 {
	    if (strlen (token) > MAXSYMLEN)
	       token[MAXSYMLEN] = '\0';

	    if ((s = symlookup (token, FALSE, TRUE)) == NULL)
	    {
	       sprintf (errtmp, "DEF undefined: %s", token);
	       logerror (errtmp);
	    }
	    else
	    {
	       s->flags |= GLOBAL;
	    }
	 }
      } while (term == ',');
      break;

   case DOUBLE_T:
      checkpc (outfd);
      opc = pc;
      i = 0;
      do {
	 t_uint64 d;

	 while (*bp && isspace(*bp)) bp++;
	 cp = bp;
	 while (*bp != ',' && !isspace(*bp)) bp++;
	 term = *bp;
	 *bp++ = '\0';
	 d = cvtdouble (cp);
	 val =  (int)((d >> 48) & 0xFFFF);
	 opc = outputdataword (outfd, lstfd, opc, val, i == 0 ? TRUE : FALSE);
	 val =  (int)((d >> 32) & 0xFFFF);
	 opc = outputdataword (outfd, lstfd, opc, val, FALSE);
	 val =  (int)((d >> 16) & 0xFFFF);
	 opc = outputdataword (outfd, lstfd, opc, val, FALSE);
	 val =  (int)(d & 0xFFFF);
	 opc = outputdataword (outfd, lstfd, opc, val, FALSE);
	 i++;
      } while (term == ',');
      break;

   case DORG_T:
      if (incseg)
      {
	 incseg = FALSE;
	 relochar = '\'';
      }
      if (!absolute && !indorg) rorgpc = pc;
      cp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      if (val < 0 || val > 65535)
      {
	 sprintf (errtmp, "Invalid DORG value: %d", val);
	 logerror (errtmp);
	 val = 0;
      }
      absolute = TRUE;
      indorg = TRUE;
      pc = val;
      sprintf (pcbuf, PCFORMAT, pc);
      break;


   case DFOP_T:
   case DXOP_T:
   case LIBIN_T:
   case LOAD_T:
   case REF_T:
   case LREF_T:
   case SETRM_T:
   case SREF_T:
      strcpy (pcbuf, PCBLANKS);
      break;

   case EQU_T:
      strcpy (pcbuf, PCBLANKS);
      if (cursym[0] == '\0')
      {
	 logerror ("EQU requires a label");
      }
      else
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 srelochar = relochar;
	 if (s->flags & DSEGSYM) srelochar = '"';
	 if (s->flags & CSEGSYM) srelochar = '+';
	 sprintf (opbuf, ADDRFORMAT, s->value & 0xFFFF,
		  (s->flags & RELOCATABLE) ? srelochar : ' ');
      }
      break;

   case EVEN_T:
      checkpc (outfd);
      break;

   case FLOAT_T:
      checkpc (outfd);
      opc = pc;
      i = 0;
      do {
	 long d;

	 while (*bp && isspace(*bp)) bp++;
	 cp = bp;
	 while (*bp != ',' && !isspace(*bp)) bp++;
	 term = *bp;
	 *bp++ = '\0';
	 d = cvtfloat (cp);
	 val =  (d >> 16) & 0xFFFF;
	 opc = outputdataword (outfd, lstfd, opc, val, i == 0 ? TRUE : FALSE);
	 val =  d & 0xFFFF;
	 opc = outputdataword (outfd, lstfd, opc, val, FALSE);
	 i++;
      } while (term == ',');
      break;

   case IDT_T:
      strcpy (pcbuf, PCBLANKS);
      while (*bp && isspace(*bp)) bp++;
      if (*bp == '\'')
      {
	 cp = ++bp;
	 while (*bp != '\'' && *bp != '\n') bp++;
      }
      *bp = '\0';
      if (strlen(cp) > IDTSIZE)
      {
	 sprintf (errtmp, "Invalid IDT value: %s", cp);
	 logerror (errtmp);
	 *(cp+IDTSIZE) = '\0';
      }
      strcpy (idtbuf, cp);
      break;

   case LDEF_T:
      strcpy (pcbuf, PCBLANKS);
      do {
	 bp = tokscan (bp, &token, &tokentype, &val, &term);
	 if (tokentype != SYM)
	 {
	    sprintf (errtmp, "LDEF requires symbol: %s", token);
	    logerror (errtmp);
	 }
	 else
	 {
	    if (strlen (token) > MAXSYMLEN)
	       token[MAXSYMLEN] = '\0';

	    if ((s = symlookup (token, FALSE, TRUE)) == NULL)
	    {
	       sprintf (errtmp, "LDEF undefined: %s", token);
	       logerror (errtmp);
	    }
	    else
	    {
	       s->flags |= GLOBAL | LONGSYM;
	    }
	 }
      } while (term == ',');
      break;

   case LIST_T:
      if (listing)
      {
	 strcpy (pcbuf, PCBLANKS);
	 printline (lstfd);
      }
      printed = TRUE;
      listing = TRUE;
      break;

   case LONG_T:
      checkpc (outfd);
      opc = pc;
      i = 0;
      do {
	 long d;

	 while (*bp && isspace(*bp)) bp++;
	 cp = bp;
	 while (*bp != ',' && !isspace(*bp)) bp++;
	 term = *bp;
	 *bp++ = '\0';
	 if (*cp == '>')
	 {
	    cp++;
	    d = strtol (cp, NULL, 16);
	 }
	 else
	 {
	    d = strtol (cp, NULL, 10);
	 }
	 val =  (d >> 16) & 0xFFFF;
	 opc = outputdataword (outfd, lstfd, opc, val, i == 0 ? TRUE : FALSE);
	 val =  d & 0xFFFF;
	 opc = outputdataword (outfd, lstfd, opc, val, FALSE);
	 i++;
      } while (term == ',');
      break;

   case NOP_T:
   case RT_T:
      sprintf (opbuf, OP1FORMAT, op->opmod, ' ');
      sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, op->opmod);
      pc += 2;
      break;

   case OPTION_T:
      strcpy (pcbuf, PCBLANKS);
      processoption (bp, TRUE);
      break;

   case PAGE_T:
      if (listing)
      {
	 strcpy (pcbuf, PCBLANKS);
	 printed = TRUE;
	 /*printline (lstfd);*/
	 linecnt = MAXLINE;
      }
      break;

   case QUAD_T:
      checkpc (outfd);
      opc = pc;
      i = 0;
      do {
	 t_uint64 d;

	 while (*bp && isspace(*bp)) bp++;
	 cp = bp;
	 while (*bp != ',' && !isspace(*bp)) bp++;
	 term = *bp;
	 *bp++ = '\0';
	 if (*cp == '>')
	 {
	    cp++;
#ifdef WIN32
	    sscanf (cp, "%I64x", &d);
#else
	    sscanf (cp, "%llx", &d);
#endif
	 }
	 else
	 {
#ifdef WIN32
	    sscanf (cp, "%I64d", &d);
#else
	    sscanf (cp, "%lld", &d);
#endif
	 }
	 val =  (int)((d >> 48) & 0xFFFF);
	 opc = outputdataword (outfd, lstfd, opc, val, i == 0 ? TRUE : FALSE);
	 val =  (int)((d >> 32) & 0xFFFF);
	 opc = outputdataword (outfd, lstfd, opc, val, FALSE);
	 val =  (int)((d >> 16) & 0xFFFF);
	 opc = outputdataword (outfd, lstfd, opc, val, FALSE);
	 val =  (int)(d & 0xFFFF);
	 opc = outputdataword (outfd, lstfd, opc, val, FALSE);
	 i++;
      } while (term == ',');
      break;

   case RORG_T:
      if (incseg)
      {
	 incseg = FALSE;
	 relochar = '\'';
      }
      bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      indorg = FALSE;
      absolute = FALSE;
      if (val == 0)
      {
	 pc = rorgpc;
      }
      else
      {
	 if (val < 0 || val > 65535)
	 {
	    sprintf (errtmp, "Invalid RORG value: %d", val);
	    logerror (errtmp);
	    val = 0;
	 }
         pc = val & 0xFFFE;
      }
      sprintf (pcbuf, PCFORMAT, pc);
      sprintf (objbuf, OBJFORMAT, RELORG_TAG, pc);
      orgout = TRUE;
      break;

   case TEXT_T:
      bp = tokscan (bp, &token, &tokentype, &val, &term);
      oval = 0;
      if (term == '-')
      {
	 oval = 1;
	 bp = tokscan (bp, &token, &tokentype, &val, &term);
      }
      if (tokentype == STRING)
      {
	 strcpy (opbuf, OPBLANKS);

	 bp = token;
	 i = l = 0;
	 val = 0;
	 opc = pc;
	 while (*bp)
	 {
	    if (oval && *(bp+1) == '\0') *bp = (0 - *bp) & 0xFF;
	    byteacc = byteacc << 8 | (*bp & 0xFF);
	    sprintf (opbuf, CHAR1FORMAT, (*bp++ & 0xFF));
	    pc++;
	    bytecnt++;
	    i++;
	    if (bytecnt == 2)
	    {
	       sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, byteacc);
	       punchrecord (outfd);
	       byteacc = 0;
	       bytecnt = 0;
	    }
	    i = 0;
	    printed = TRUE;
	    if (l++ == 0)
	    {
	       printline (lstfd);
	       opc = pc;
	    }
	    else
	    {
	       if (!tunlist)
	       printdata (lstfd, opc);
	       opc = pc;
	    }
	    strcpy (opbuf, OPBLANKS);
	 }
      }
      else
      {
	 sprintf (errtmp, "Invalid TEXT string: %s", token);
	 logerror (errtmp);
      }
      break;

   case TITL_T:
      strcpy (pcbuf, PCBLANKS);
      while (isspace(*bp)) bp++;
      if (*bp == '\'')
      {
	 cp = ++bp;
	 while (*bp != '\'' && *bp != '\n') bp++;
      }
      *bp = '\0';
      if (strlen (cp) > TTLSIZE)
      {
	 sprintf (errtmp, "Invalid TITL value: %s", cp);
	 logerror (errtmp);
	 *(cp+TTLSIZE) = '\0';
      }
      strcpy (ttlbuf, cp);
      printed = TRUE;
      break;

   case UNL_T:
      strcpy (pcbuf, PCBLANKS);
      listing = FALSE;
      break;

   case XVEC_T:
      bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      srelochar = relochar;
      if (segvaltype)
	 srelochar = (segvaltype & DSEGSYM) ? '"' : '+';
      sprintf (opbuf, DATA1FORMAT, val, relocatable ? srelochar : ' ');
      printed = TRUE;
      printline (lstfd);
      if (incseg)
      {
	 if (relocatable)
	    sprintf (objbuf, CMNOBJFORMAT, CMNDATA_TAG, val,
		     csegs[curcseg].number);
	 else
	    sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
      }
      else if (relocatable < 0)
         processextndx (objbuf, val, extname, outfd);
      else
	 sprintf (objbuf, OBJFORMAT,
		  relocatable ? RELDATA_TAG : ABSDATA_TAG, val);
      punchrecord (outfd);

      if (term == ',')
      {
	 bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      }
      else
      {
	 relocatable = TRUE;
         val = pc + 4;
      }
      sprintf (opbuf, DATA1FORMAT, val, relocatable ? srelochar : ' ');
      if (!dunlist)
	 printdata (lstfd, pc + 2);
      if (incseg)
      {
	 if (relocatable)
	    sprintf (objbuf, CMNOBJFORMAT, CMNDATA_TAG, val,
		     csegs[curcseg].number);
	 else
	    sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, val);
      }
      else if (relocatable < 0)
         processextndx (objbuf, val, extname, outfd);
      else
	 sprintf (objbuf, OBJFORMAT,
		  relocatable ? RELDATA_TAG : ABSDATA_TAG, val);
      punchrecord (outfd);
      pc += 4;
      break;

   case CEND_T:
      csegs[curcseg].length = pc;
      incseg = FALSE;
      absolute = FALSE;
      relocatable = TRUE;
      inpseg = TRUE;
      pc = rorgpc;
      orgout = FALSE;
      relochar = '\'';
      break;

   case DEND_T:
      dseglength = pc;
      indseg = FALSE;
      absolute = FALSE;
      relocatable = TRUE;
      inpseg = TRUE;
      pc = rorgpc;
      orgout = FALSE;
      relochar = '\'';
      break;

   case PEND_T:
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
	 orgout = FALSE;
      }
      if (incseg)
      {
	 csegs[curcseg].length = pc;
	 incseg = FALSE;
	 pc = rorgpc;
	 orgout = FALSE;
      }
      indorg = FALSE;
      inpseg = TRUE;
      relochar = '\'';
      strcpy (pcbuf, PCBLANKS);
      bp = tokscan (bp, &token, &tokentype, &val, &term);
      if (tokentype == SYM)
      {
	 if (strlen (token) > MAXSYMLEN)
	 {
	    token[MAXSYMLEN] = '\0';
	 }
	 if ((s = symlookup (token, FALSE, TRUE)) == NULL)
	 {
	    sprintf (errtmp, "Undefined symbol: %s", token);
	    logerror (errtmp);
	 }
	 else
	 {
	    sprintf (opbuf, ADDRFORMAT,
		     s->value,
		     (s->flags & RELOCATABLE) ? relochar : ' ');
	    sprintf (objbuf, OBJFORMAT,
		  (!absolute && (s->flags & RELOCATABLE)) ?
			RELENTRY_TAG : ABSENTRY_TAG,
		  s->value);
	 }
      }
      return (1);
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
      orgout = FALSE;
      relochar = '+';
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
         strcpy (temp, "$BLANK");
	 cp = temp;
      }
      *bp = '\0';
      if (strlen(cp) > MAXSHORTSYMLEN)
      {
	 *(cp+MAXSHORTSYMLEN) = '\0';
      }
      for (i = 0; i < csegcnt; i++)
      {
         if (!strcmp (csegs[i].name, cp))
	 {
	    curcseg = i;
	    break;
	 }
      }
      csegval = csegs[curcseg].number;
      pc = csegs[curcseg].length;
      sprintf (pcbuf, PCFORMAT, pc);
      break;

   case DSEG_T:
      if (!absolute && !incseg && !indseg && !indorg) rorgpc = pc;
      if (incseg)
      {
         csegs[curcseg].length = pc;
	 incseg = FALSE;
      }
      absolute = FALSE;
      inpseg = FALSE;
      relocatable = TRUE;
      indseg = TRUE;
      relochar = '"';
      if (dseglength >= 0)
      {
	 sprintf (objbuf, OBJFORMAT, DSEGORG_TAG, dseglength);
      }
      else
         dseglength = 0;
      pc = dseglength;
      sprintf (pcbuf, PCFORMAT, pc);
      break;

   case PSEG_T:
      if (indseg)
      {
	 dseglength = pc;
      }
      if (incseg)
      {
         csegs[curcseg].length = pc;
      }
      incseg = FALSE;
      indseg = FALSE;
      indorg = FALSE;
      absolute = FALSE;
      relocatable = TRUE;
      if (!inpseg)
	 pc = rorgpc;
      inpseg = TRUE;
      relochar = '\'';
      sprintf (pcbuf, PCFORMAT, pc);
      orgout = FALSE;
      break;

   default: ;
   }
   return (0);
}

/***********************************************************************
* asmpass2 - Pass 2 
***********************************************************************/

int
asmpass2 (FILE *infd, FILE *outfd, int listingmode, FILE *lstfd)
{
   char *bp;
   char *token;
   int i;
   int status = 0;
   int done = 0;
   int val;
   int tokentype;
   time_t curtime;
   char term;

#ifdef DEBUG
   printf ("asmpass2: Entered\n");
#endif

   /*
   ** Get current date/time.
   */

   listmode = listingmode;
   curtime = time(NULL);
   timeblk = localtime(&curtime);
   strcpy (datebuf, ctime(&curtime));
   *strchr (datebuf, '\n') = '\0';

   /*
   ** Rewind input file.
   */

   tmpfd = infd;
   if (fseek (tmpfd, 0, SEEK_SET) < 0)
   {
      perror ("asm990: Can't rewind temp file");
      return (-1);
   }

   /*
   ** Initialize
   */

   memset (objrec, ' ', sizeof(objrec));

   pc = 0;
   rorgpc = 0;
   extndx = 0;
   pagenum = 0;
   curp1err = 0;
   objrecnum = 0;
   objcnt = 0;

   relochar = '\'';
   linecnt = MAXLINE;
   absolute = FALSE;
   indorg = FALSE;
   printed = FALSE;
   inskip = FALSE;
   listing = TRUE;

   ckpt = -1;

   orgout = FALSE;

   /*
   ** Output IDT
   */

   orgout = TRUE;
   sprintf (objbuf, IDTFORMAT, IDT_TAG, pgmlength & 0xFFFF, idtbuf);
   punchrecord (outfd);
   orgout = FALSE;

   /*
   ** Output DSEG, if defined.
   */

   if (dseglength >= 0)
   {
      orgout = TRUE;
      sprintf (objbuf, CMNFORMAT, COMMON_TAG, dseglength, "$DATA ", 0);
      punchrecord (outfd);
      if (dseglength == 0)
	 dseglength = -1;
      else 
         dseglength = 0;
      orgout = FALSE;
   }

   /*
   ** Output CSEG, if defined.
   */

   for (i = 0; i < csegcnt; i++)
   {
      orgout = TRUE;
      sprintf (objbuf, CMNFORMAT, COMMON_TAG, csegs[i].length,
               csegs[i].name, csegs[i].number);
      punchrecord (outfd);
      orgout = FALSE;
      csegs[i].length = 0;
   }

   /*
   ** Process the source.
   */

   byteacc = 0;
   bytecnt = 0;
   linenum = 0;
   pscanbuf = inbuf;

   while (!done)
   {
      /*
      ** Read source line mode, line number and text.
      */

      if (readsource (tmpfd, &inmode, &linenum, &filenum, inbuf) < 0)
      {
         done = TRUE;
	 break;
      }

      printed = FALSE;
      errline[0][0] = '\0';
      errnum = 0;
      objbuf[0] = '\0';
      strcpy (lstbuf, inbuf);
      bp = inbuf;

      strcpy (pcbuf, PCBLANKS);
      strcpy (opbuf, OPBLANKS);

      if (!(inmode & MACDEF) && (*bp != COMMENTSYM))
      {
	 OpCode *op;

	 /*
	 ** If label present, scan it off.
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
	    if (!inskip && term == '\n')
	    {
	       sprintf (pcbuf, PCFORMAT, pc);
	       goto PRINTLINE;
	    }
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
	 if (!token[0]) goto PRINTLINE;
	 if ((inmode & MACCALL) || (inmode & SKPINST))
	 {
	    sprintf (pcbuf, PCFORMAT, pc);
	    goto PRINTLINE;
	 }

	 /*
	 ** Process according to type.
	 */

	 if (inskip)
	 {
	    op = oplookup (token, 1);
	    if (op != NULL && op->optype == TYPE_P)
	       switch (op->opvalue)
	       {
	       case ASMIF_T:
	       case ASMELS_T:
	       case ASMEND_T:
		  p2pop (op, bp, outfd, lstfd);
		  break;

	       default: ;
	       }
	 }
	 else if ((op = oplookup (token, 2)) != NULL)
	 {
	    while (*bp && isspace(*bp)) bp++;

	    if (op->optype != TYPE_P)
	    {
	       if (bytecnt)
	       {
		  byteacc = (byteacc << 8) | 0;
		  sprintf (objbuf, OBJFORMAT, ABSDATA_TAG, byteacc);
		  punchrecord (outfd);
		  bytecnt = 0;
	       }
	       if (pc & 0x0001) pc++;
	       inst_proc[op->optype].proc (op, bp, outfd, lstfd);
	    }
	    else
	       done = p2pop (op, bp, outfd, lstfd);
	 }
      }

      /*
      ** Write out a print line.
      */
PRINTLINE:
      if (!printed)
      {
         printline (lstfd);
	 printed = FALSE;
      }

      /*
      ** Write an object buffer.
      */

      if (objbuf[0])
      {
         punchrecord (outfd);
      }

      /*
      ** Process errors
      */

      if (printerrors (lstfd, listmode) < 0)
         status = -1;
   }

   if (!done)
   {
      errcount++;
      if (listmode)
      {
         fprintf (lstfd, "ERROR: No END record\n");
      }
      else
      {
	 fprintf (stderr, "asm990: %d: No END record\n", linenum);
      }
      status = -1;
   }
   else
   {

      /*
      ** Process literals
      */

      if (processliterals (outfd, lstfd, listmode) < 0) status = -1;

      /*
      ** Punch REF and DEF entries
      */

      punchsymbols (outfd);
      puncheof (outfd);

      /*
      ** Print symbol table and other information.
      */

      if (listmode)
      {
         printsymbols (lstfd);

	 if (libincnt)
	 {
	    fprintf (lstfd, "\nActive LIBIN directories:\n");
	    for (i = 0; i < libincnt; i++)
	    {
	       fprintf (lstfd,"   %d   %s\n", i+1, libinfiles[i].dirname);
	    }
	 }
      }
   }

   return (status);
}
