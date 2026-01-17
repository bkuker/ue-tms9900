/***********************************************************************
*
* asmpass0 - Pass 0 (Macro processor) for the TI 990 assembler.
*
* Changes:
*   05/27/09   DGP   Original.
*   06/03/09   DGP   Cleaned up C/D/P SEGS.
*   06/06/09   DGP   Added LIBIN/LIBOUT support.
*   06/09/09   DGP   Added DFOP support.
*   06/29/09   DGP   Do not defer on ASMIF_T lookup.
*   04/22/10   DGP   Correct missing operand parsing.
*   11/29/10   DGP   Added HEXNUM to token scanning. Fixed macro scan
*                    margins and apostrope processing.
*   11/30/10   DGP   Added QUAL_xx definitions and ATTR_xx.
*   12/14/10   DGP   Added line number to temporary file.
*                    Added newline on input if missing.
*   12/16/10   DGP   Added a unified source write function writesource().
*   01/06/11   DGP   Allowed LIBIN loads in macros.
*                    Allowed Literal symbols in macros. New attribute.
*                    Fixed cross reference issues.
*   01/17/11   DGP   Fixed non terminated IF in Macro expansion.
*   01/04/14   DGP   Fixed more cross reference issues.
*   02/03/14   DGP   Added SYMT support.
*   10/07/14   DGP   Many MACRO processing fixes.
*   10/15/14   DGP   Defer EQU expressions.
*   12/29/14   DGP   Allow for nested $IF/ASMIF statements.
*   02/10/15   DGP   Fixed REF/SREF in DATA statements.
*   02/20/15   DGP   Fixed $ASG/$SBSTG processing.
*   10/01/15   DGP   Fixed invalid symbol "sym:".
*   11/18/15   DGP   Added inpseg flag.
*   01/24/17   DGP   Fixed segment flags on non-relo EQU.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <errno.h>

#include "asmdef.h"

extern int pc;			/* the assembler pc */
extern int symbolcount;		/* Number of symbols in symbol table */
extern int absolute;		/* In absolute section */
extern int linenum;		/* Source line number */
extern int exprtype;		/* Expression type */
extern int pgmlength;		/* Length of program */
extern int genxref;		/* Generate cross reference listing */
extern int gensymt;		/* Generate symbol table in object */
extern int model;		/* CPU model */
extern int errcount;		/* Number of errors in assembly */
extern int indorg;		/* In DORG section */
extern int rightmargin;		/* Right margin for scanner */
extern int rorgpc;		/* Relocatable PC */
extern int dseglength;		/* Length of DSEG */
extern int inpseg;		/* In PESG section */
extern int indseg;		/* In DESG section */
extern int incseg;		/* In CSEG section */
extern int csegcnt;
extern int filenum;
extern int fileseq;
extern int libincnt;		/* Number of LIBIN's active */
extern int libout;		/* LIBOUT active */
extern int txmiramode;

extern char inbuf[MAXLINE];	/* The input buffer for the scanners */

extern SymNode *symbols[MAXSYMBOLS];/* The Symbol table */
extern CSEGNode csegs[MAXCSEGS];
extern LIBfile libinfiles[MAXASMFILES];/* LIBIN file descriptors */
extern LIBfile liboutfile;	/* LIBOUT file descriptor */
extern char *pscanbuf;		/* Pointer for tokenizers */

static FILE *tmpfd;		/* Input fd */

static int lblgennum;		/* LBL generated number */
static int macrocount;		/* Number of macros defined */
static int eofflg = FALSE;	/* EOF on input file */
static int macscan;		/* Currently scanning a MACRO definition */
static int litlen;		/* Length of literals */
static int bytecnt;		/* Byte count for TEXT/BYTE insts */
static int curcseg;		/* Current CSEG */
static int gblmode;		/* Global file mode */
static int macnest;		/* Macro nesting level */
static int maxnest;		/* Maximum Macro nesting level */
static int genlinenum;		/* Generated line number */
static int macvarscount;	/* Number of macro variables */

static int inskip;		/* In an ASMIF Skip */

static char cursym[MAXSYMLEN+2];/* Current label field symbol */
static char errtmp[256];	/* Error print string */

static MacLines *freemaclines = NULL;/* resusable Macro lines */
static MacDef macdef[MAXMACROS];/* Macro template table */
static MacArg macvars[MAXMACARGS]; /* $VAR table */
static int p0checklibin (char *, char *, char *, int, int,
			FILE *, FILE *, char *);

typedef struct
{
   char *symbol;
   int value;
} ATTRType;

static ATTRType attrtypes[] =
{
   { "$PCALL", ATTR_PCALL }, { "$POPL",  ATTR_POPL },   { "$PIND", ATTR_PIND },
   { "$PNDX",  ATTR_PNDX },  { "$PATO",  ATTR_PATO },   { "$PSYM", ATTR_PSYM },
   { "$PLIT",  ATTR_PLIT },  { "$REL",   ATTR_RELO },   { "$REF",  ATTR_REF },
   { "$DEF",   ATTR_DEF  },  { "$STR",   ATTR_STR },    { "$VAL",  ATTR_VAL },
   { "$MAC",   ATTR_MAC },   { "$UNDF",  ATTR_UNDEF },  { "",      0 }
};

/***********************************************************************
* maclogerror - Macro logerror interlude.
***********************************************************************/

static void
maclogerror (MacDef *mac, char *msg, int line)
{
   int t;

#ifdef DEBUGERROR
   printf ("maclogerror: startline = %d, line = %d\n",
	   mac->macstartline, line);
   printf ("   msg = %s\n", msg);
#endif
   t = linenum;
   linenum = mac->macstartline;
   if (line > 0) linenum += line;
   logerror (msg);
   linenum = t;
}

/***********************************************************************
* p0literal - Process literal.
***********************************************************************/

static char *
p0literal (char *bp)
{
   if (!txmiramode)
   {
      pc += 2;
      bp++;
      if (*bp == 'F' || *bp == 'L')
	 litlen += 4;
      else if (*bp == 'D' || *bp == 'Q')
	 litlen += 8;
      else
	 litlen += 2;
   }
   return (bp);
}

/***********************************************************************
* p012mop - One word opcode 2 word memory operand processor.
***********************************************************************/

static void
p012mop (OpCode *op, char *bp)
{
   while (isspace (*bp)) bp++;
   if (*bp == MEMSYM)
      pc += 2;
   else if (*bp == LITERALSYM)
      bp = p0literal (bp);

   while (*bp && *bp != ',') bp++;
   if (*bp == ',')
   {
      int cmpr;

      if (op->opvalue == 0x8000 || op->opvalue == 0x9000)
	 cmpr = TRUE;
      else
	 cmpr = FALSE;
      bp++;
      if (*bp == MEMSYM)
	 pc += 2;
      else if (cmpr && *bp == LITERALSYM)
	 bp = p0literal (bp);
   }
   pc += 2;
}

/***********************************************************************
* p011mop - One word opcode 1 word memory operand processor.
***********************************************************************/

static void
p011mop (OpCode *op, char *bp)
{
   while (isspace (*bp)) bp++;
   if (*bp == MEMSYM)
      pc += 2;
   else if (*bp == LITERALSYM)
      bp = p0literal (bp);
   pc += 2;
}

/***********************************************************************
* p011iop - One word opcode 1 word immediate operand processor.
***********************************************************************/

static void
p011iop (OpCode *op, char *bp)
{
   if (op->opmod != 2) pc += 2;
   pc += 2;
}

/***********************************************************************
* p010mop - One word opcode 0 word memory operand processor.
***********************************************************************/

static void
p010mop (OpCode *op, char *bp)
{
   pc += 2;
}

/***********************************************************************
* p022mop - Two word opcode 2 word memory operand processor.
***********************************************************************/

static void
p022mop (OpCode *op, char *bp)
{
   while (isspace (*bp)) bp++;
   if (*bp == MEMSYM)
      pc += 2;
   else if (*bp == LITERALSYM)
      bp = p0literal (bp);

   while (*bp && *bp != ',') bp++;
   if (*bp == ',')
   {
      bp++;
      while (isspace (*bp)) bp++;
      if (*bp == MEMSYM)
	 pc += 2;
   }
   pc += 4;
}

/***********************************************************************
* p022cop - Two word opcode 2 word memory operand processor with condition.
***********************************************************************/

static void
p022cop (OpCode *op, char *bp)
{
   while (isspace (*bp)) bp++;
   while (*bp && *bp != ',') bp++;
   if (*bp == ',')
   {
      bp++;
      while (isspace (*bp)) bp++;
      if (*bp == MEMSYM)
	 pc += 2;
      else if (*bp == LITERALSYM)
	 bp = p0literal (bp);

      while (*bp && *bp != ',') bp++;
      if (*bp == ',')
      {
	 bp++;
	 while (isspace (*bp)) bp++;
	 if (*bp == MEMSYM)
	    pc += 2;
      }
   }
   pc += 4;
}

/***********************************************************************
* p021mop - Two word opcode 1 word memory operand processor.
***********************************************************************/

static void
p021mop (OpCode *op, char *bp)
{
   while (isspace (*bp)) bp++;
   if (*bp == MEMSYM)
      pc += 2;
   else if (*bp == LITERALSYM)
      bp = p0literal (bp);

   pc += 4;
}

/***********************************************************************
* p020mop - Two word opcode 0 word memory operand processor.
***********************************************************************/

static void
p020mop (OpCode *op, char *bp)
{
   pc += 4;
}

/***********************************************************************
* maclookup - Lookup macro in table
***********************************************************************/

static MacDef *
maclookup (char *name)
{
   MacDef *mac = NULL;
   int i;

   for (i = 0; i < macrocount; i++)
   {
      mac = &macdef[i];
      if (!strcmp (mac->macname, name))
      {
	 return (mac);
      }
   }
   return (NULL);
}

/***********************************************************************
* p0maccvtarg - Convert token to mac args.
***********************************************************************/

static void
p0maccvtarg (MacDef *mac, char *token, char *targ, int strval, int line)
{
   int j;
   int count = mac->macargcount + mac->macvarcount;
   char *tp = strchr (token, '.');

   if (tp) *tp++ = '\0';
   for (j = 0; j < count; j++)
   {
      if (!strcmp (token, mac->macargs[j].defsym))
      {
	 char qual[2];

	 qual[0] = '\0';
	 qual[1] = '\0';
	 sprintf (targ, "#%02d", j);
	 if (!tp) qual[0] = '0' + QUAL_ALL;
	 else if (!strcmp (tp, "A")) qual[0] = '0' + QUAL_A;
	 else if (!strcmp (tp, "L")) qual[0] = '0' + QUAL_L;
	 else if (!strcmp (tp, "S"))
	 {
	    if (strval)
	       qual[0] = '0' + QUAL_S;
	    else
	       qual[0] = '0' + QUAL_NS;
	 }
	 else if (!strcmp (tp, "V")) qual[0] = '0' + QUAL_V;
	 else if (!strcmp (tp, "SA")) qual[0] = '0' + QUAL_SA;
	 else if (!strcmp (tp, "SL")) qual[0] = '0' + QUAL_SL;
	 else if (!strcmp (tp, "SS")) qual[0] = '0' + QUAL_SS;
	 else if (!strcmp (tp, "SV")) qual[0] = '0' + QUAL_SV;
	 else if (!strcmp (tp, "SU")) qual[0] = '0' + QUAL_SU;
	 else
	 {
	    sprintf (errtmp, "Invalid MACRO variable qualifier: %s", tp);
	    maclogerror (mac, errtmp, line);
	 }
	 strcat (targ, qual);
	 break;
      }
   }
   if (j == count)
      strcpy (targ, token);
}

/***********************************************************************
* p0mactokens - Process macro tokens
***********************************************************************/

static char *
p0mactokens (char *cp, int field, int index, MacDef *mac, int line)
{
   char *token;
   char *ocp;
   int tokentype;
   int val;
   int barequote;
   int done;
   char term;
   char targ[TOKENLEN];
   char largs[MAXLINE];

#ifdef DEBUGMACRO
   printf ("p0mactokens: field = %d, index = %d, cp = %s\n",
	    field, index, cp);
#endif

   /*
   ** Check for NULL operands 
   */

   if (field == 2)
   {
      ocp = cp;
      while (*cp && isspace (*cp))
      {
         cp++;
	 if ((cp - ocp) > rightmargin)
	    return (cp);
      }
      if (*cp == 0)
         return (cp);
   }

   /*
   ** Substitute #NNT for each macro parameter. The number, NN, corresponds to
   ** the parameter number and T is it's type.
   */

   term = '\0';
   memset (largs, 0, MAXLINE);
   barequote = FALSE;
   done = FALSE;
   while (!done)
   {

      targ[0] = '\0';
      if (field < 0)
      {
	 ocp = largs + strlen (largs);
         while (*cp && isspace(*cp))
	    *ocp++ = *cp++;
	 *ocp++ = 0;
      }

      cp = tokscan (cp, &token, &tokentype, &val, &term);
#ifdef DEBUGMACRO
      printf ("   token1 = %s, tokentype = %d, val = %d, term = %c(%x)\n",
	    token, tokentype, val, term, term);
      printf ("   cp = %s\n", cp);
#endif

      /*
      ** If operator in input stream, copy it out
      */

      if (token[0] == '\0')
      {
	 if (tokentype == DECNUM)
	 {
	    sprintf (targ, "%d", val);
	    if (term == ',') strncat (targ, &term, 1);
	 }
	 else if (tokentype == HEXNUM)
	 {
	    sprintf (targ, ">%X", val);
	    if (term == ',') strncat (targ, &term, 1);
	 }
	 else if (tokentype == PC)
	    strcpy (targ, "$");
	 else if (term == '\'')
	    barequote = TRUE;
	 else
	    sprintf (targ, "%c", term);
      }

      /*
      ** If token is a parameter, process
      */

      else
      {
	 if (tokentype == MACSYM || tokentype == SYM)
	 {
	    p0maccvtarg (mac, token, targ, tokentype == MACSYM, line);
	    if (term == ',') strncat (targ, &term, 1);
	 }
	 else if (tokentype == STRING)
	 {
	    sprintf (targ, "'%s'", token);
	 }
	 else if (IS_RELOP(tokentype))
	 {
	    strcpy (targ, token);
	 }
	 else
	 {
	    strcpy (targ, token);
	    if (term != ':' && !isspace(term))
	    {
	       strncat (targ, &term, 1);
	       if (term != ',') cp++;
	    }
	 }
      }

      strcat (largs, targ);
      if (*cp == 0) done = TRUE;
      else if (field >= 0 && (isspace(term) || isspace(*cp))) done = TRUE;
   }

#ifdef DEBUGMACRO
   printf ("   largs = %s\n", largs);
#endif

   /*
   ** Place processed field into macro template
   */

   switch (field)
   {
   case 0:
      strcat (mac->maclines[index]->label, largs);
      break;
   case 1:
      strcat (mac->maclines[index]->opcode, largs);
      break;
   case 2:
      strcat (mac->maclines[index]->operand, largs);
      break;
   default:
      strcat (mac->maclines[index]->operand, largs);
      break;
   }
   return (cp);
}

/***********************************************************************
* p0allocline - Allocate macro line template
***********************************************************************/

static void
p0allocline (MacDef *mac, int linendx, char *srcline, int ndx)
{
   if (freemaclines)
   {
      mac->maclines[linendx] = freemaclines;
      freemaclines = mac->maclines[linendx]->next;
   }
   else if ((mac->maclines[linendx] =
		  (MacLines *)malloc (sizeof(MacLines))) == NULL)
   {
      fprintf (stderr, "asm990: Unable to allocate memory\n");
      exit (ABORT);
   }
   memset (mac->maclines[linendx], '\0', sizeof(MacLines));
   if (srcline)
      strcpy (mac->maclines[linendx]->source, srcline);
   mac->maclines[linendx]->nestndx = ndx;
}

/***********************************************************************
* p0macro - Process macro templates
***********************************************************************/

static int
p0macro (char *bp, FILE *infd, FILE *outfd)
{
   MacDef *mac;
   char *token;
   char *cp;
   int tokentype;
   int val;
   int i;
   int done;
   int insbstg;
   int genmode;
   int linendx;
   int linecnt;
   int nestndx;
   char term;
   char genline[MAXLINE];
   char srcline[MAXLINE];

#ifdef DEBUGMACRO
   printf ("p0macro: cursym[0] = %x\n", cursym[0]);
#endif
   macscan = TRUE;
   nestndx = 0;

   mac = &macdef[macrocount];
   memset (mac, '\0', sizeof (MacDef));
   mac->macstartline = linenum;

   if (cursym[0])
   {
      strcpy (mac->macname, cursym);
      i = 0;
   }
   else
   {
      maclogerror (mac, "MACRO definition has no name", -1);
   }

   /*
   ** Scan off the parameters
   */

   do {
      bp = tokscan (bp, &token, &tokentype, &val, &term);
      if (!token[0]) break;
      strcpy (mac->macargs[i].defsym, token);
      i++;
   } while (term == ',');
   mac->macargcount = i;

#ifdef DEBUGMACRO
   printf ("\nMACRO0: name = %s\n", mac->macname);
   for (i = 0; i < mac->macargcount; i++)
      printf ("   arg%02d = %s\n", i, mac->macargs[i].defsym);
#endif

   /*
   ** Read the source into the template until $END.
   */

   done = FALSE;
   linendx = 0;
   linecnt = 0;
   pscanbuf = genline;
   srcline[0] = '\0';
   while (!done)
   {
      /*
      ** Write source line to intermediate file with MACDEF mode
      */

      genmode = gblmode | MACDEF;
      fgets (genline, MAXLINE, infd);

      if (feof(infd))
      {
	 maclogerror (mac, "Non terminated MACRO definition", -1);
	 return (-1);
      }

      if (outfd)
      {
	 linenum++;
	 writesource (outfd, genmode, linenum, genline, "m01");
      }
      linecnt++;

      if (genline[0] != COMMENTSYM)
      {
	 OpCode *op;
	 int ismodel;

	 if (libout)
	    strcpy (srcline, genline);

	 if ((cp = strchr (genline, '\n')) != NULL)
	    *cp = '\0';

	 cursym[0] = '\0';
	 cp = genline;
         if (isalpha(*cp) || *cp == '$' || *cp == '_')
	 {
	    cp = tokscan (cp, &token, &tokentype, &val, &term);
	    strcpy (cursym, token);
	 }
	 while (*cp && isspace(*cp)) cp++;
	 cp = tokscan (cp, &token, &tokentype, &val, &term);
	 op = oplookup (token, 0);

	 ismodel = TRUE;
	 if (op && op->optype == TYPE_P)
	 {
	    int hasoperand;

	    insbstg = FALSE;
	    hasoperand = TRUE;
	    switch (op->opvalue)
	    {
	    case CALL_T:
	    case GOTO_T:
	       p0allocline (mac, linendx, srcline, nestndx);
	       strcpy (mac->maclines[linendx]->opcode, token);
	       cp = tokscan (cp, &token, &tokentype, &val, &term);
	       strcpy (mac->maclines[linendx]->operand, token);
	       ismodel = FALSE;
	       linendx++;
	       break;

	    case MEND_T:
	       if (nestndx)
	       {
		  sprintf (errtmp, "Non terminated IF in MACRO: %s",
			   mac->macname);
		  maclogerror (mac, errtmp, linendx);
	       }
	       macscan = FALSE;
	       done = TRUE;

	    case NAME_T:
	       p0allocline (mac, linendx, srcline, nestndx);
	       strcpy (mac->maclines[linendx]->label, cursym);
	       strcpy (mac->maclines[linendx]->opcode, token);
	       mac->maclines[linendx]->operand[0] = '\0';
	       ismodel = FALSE;
	       linendx++;
	       break;

	    case SBSTG_T:
	       insbstg = TRUE;
	    case ASG_T:
	       {
		  int asgdone;
		  int spccnt;
		  int getvar;
		  char targ[MAXLINE];

		  p0allocline (mac, linendx, srcline, nestndx);
		  strcpy (mac->maclines[linendx]->opcode, token);
		  while (*cp && isspace(*cp)) cp++;

		  asgdone = FALSE;
		  getvar = FALSE;
		  spccnt = 0;
		  while (!asgdone)
		  {
		     targ[0] = '\0';
		     cp = tokscan (cp, &token, &tokentype, &val, &term);
#ifdef DEBUGMACROASG
		     printf (
		    "   argtok = %s, tokentype = %d, val = %d, term = %c(%x)\n",
			   token, tokentype, val, term, term);
		     printf ("   cp = %s\n", cp);
#endif
		     /*
		     ** If operator in input stream, copy it out
		     */

		     if (token[0] == '\0')
		     {
			if (tokentype == DECNUM)
			   sprintf (targ, "%d", val);
			else if (tokentype == HEXNUM)
			   sprintf (targ, ">%X", val);
			else if (tokentype == PC)
			   strcpy (targ, "$");
			else
			   sprintf (targ, "%c", term);
		     }

		     /*
		     ** If token is a parameter, process
		     */

		     else
		     {
			if (tokentype == MACSYM || tokentype == SYM)
			{
			   if (term == ' ' && !strcmp(token,"TO"))
			   {
			      sprintf (targ, " %s ", token);
			      getvar = TRUE;
			   }
			   else
			   {
			      p0maccvtarg (mac, token, targ, TRUE, linecnt);
			   }
			}
			else if (tokentype == '+')
			{
			   strcpy (targ, token);
			}
			else if (tokentype == STRING)
			{
			   sprintf (targ, "'%s'", token);
			}
			else
			{
			   if (getvar)
			   {
			      p0maccvtarg (mac, token, targ, TRUE, linecnt);
			   }
			   else
			      strcpy (targ, token);
			   if (term != ':' && !isspace(term))
			   {
			      strncat (targ, &term, 1);
			   }
			}
		     }
		     if (insbstg && term == ',')
			strncat (targ, &term, 1);

		     strcat (mac->maclines[linendx]->operand, targ);

		     if (isspace(term) || (term == 0))
		     {
		        spccnt++;
			if (spccnt == 3 || term == 0)
			   asgdone = TRUE;
		     }
		  }
		  if (!getvar)
		  {
		     sprintf (errtmp, "No TO in %s statement",
			      insbstg ? "$SBSTG" : "$ASG" );
		     maclogerror (mac, errtmp, linendx);
		  }
		  linendx++;
		  ismodel = FALSE;
	       }
	       break;

	    case VAR_T:
	       do {
		  int ndx;

		  ndx = mac->macargcount + mac->macvarcount;
		  cp = tokscan (cp, &token, &tokentype, &val, &term);
		  strcpy (mac->macargs[ndx].defsym, token);
		  for (i = 0; i < macvarscount; i ++)
		  {
		     if (!strcmp (macvars[i].defsym, token))
		        break;
		  }
		  if (i == macvarscount)
		  {
		     mac->macargs[ndx].ptr = &macvars[macvarscount];
		     strcpy (macvars[macvarscount].defsym, token);
		     macvarscount++;
		  }
	          mac->macvarcount++;
	       } while (term == ',');
#ifdef DEBUGMACROVAR
	       printf ("Variables: macvarcount = %d, macvarscount = %d\n",
		       mac->macvarcount, macvarscount);
	       for (i = 0; i < mac->macvarcount; i++)
		  printf ("   var%02d = %s\n",
			  i, mac->macargs[mac->macargcount + i].defsym);
#endif
#ifdef DEBUGMACRO
               printf ("STMT: label = %s\n", cursym);
               printf ("      opcode = %s\n", "$VAR");
               printf ("      operand = ");
	       for (i = 0; i < mac->macvarcount; i++)
		  printf ("%s,", mac->macargs[mac->macargcount + i].defsym);
	       printf ("\n");
#endif
	       ismodel = FALSE;
	       break;
	       
	    case ENDIF_T:
	    case ELSE_T:
	    case EXIT_T:
	       hasoperand = FALSE;

	    case IF_T:
	       if (op->opvalue == IF_T)
	          nestndx++;

	       p0allocline (mac, linendx, srcline, nestndx);

	       if (op->opvalue == ENDIF_T && nestndx)
	          nestndx--;
	       cp = genline;

	       /*
	       ** If label field process label
	       */

	       if (isalpha(*cp) || *cp == '$' || *cp == '_' || *cp == ':')
	       {
		  cp = p0mactokens (cp, 0, linendx, mac, linecnt);
	       }

	       /*
	       ** Process opcode field
	       */

	       cp = p0mactokens (cp, 1, linendx, mac, linecnt);

	       /*
	       ** Process operand field
	       */

	       if (hasoperand)
	       {
		  cp = p0mactokens (cp, 2, linendx, mac, linecnt);
	       }
	       linendx++;
	       ismodel = FALSE;
	       break;

	    default: ;
	    }
#ifdef DEBUGMACRO
	    if (!ismodel && op->opvalue != VAR_T)
	    {
	       i = linendx - 1;
	       printf ("STMT: label = %s\n", mac->maclines[i]->label);
	       printf ("      opcode = %s\n", mac->maclines[i]->opcode);
	       printf ("      operand = %s\n", mac->maclines[i]->operand);
	    }
#endif
	 }

	 /*
	 ** Process model template line
	 */

	 if (ismodel)
	 {
	    p0allocline (mac, linendx, srcline, nestndx);
	    mac->maclines[linendx]->ismodel = TRUE;
	    cp = genline;
	    cp = p0mactokens (cp, -1, linendx, mac, linecnt);
#ifdef DEBUGMACRO
	    printf ("   model = '%s'\n", mac->maclines[linendx]->operand);
#endif
	    linendx++;
	 }
      }
   }
   mac->maclinecount = linendx;
#ifdef DEBUGMACRO
   printf ("MACRO: %s\n", mac->macname);
   for (i = 0; i < mac->macargcount; i++)
      printf ("arg%02d: %s\n", i, mac->macargs[i].defsym);
   for (i = 0; i < mac->macvarcount; i++)
      printf ("var%02d: %s\n", i, mac->macargs[mac->macargcount + i].defsym);
   for (i = 0; i < mac->maclinecount; i++)
   {
      char nest[4];

      nest[0] = 0;
      if (mac->maclines[i]->nestndx)
         sprintf (nest, "%d", mac->maclines[i]->nestndx);
      if (mac->maclines[i]->ismodel)
         printf ("%4s: %s\n", nest, mac->maclines[i]->operand);
      else
	 printf ("%4s: %-6.6s %-7.7s %s\n",
	       nest,
	       mac->maclines[i]->label,
	       mac->maclines[i]->opcode,
	       mac->maclines[i]->operand);
   }
#endif

   /*
   ** If we have an active LIBOUT, write out the definition.
   */

   if (libout)
   {
      FILE *libfd;
      char libfile[MAXPATHNAMESIZE*2];

      snprintf (libfile, sizeof(libfile), "%s%s.mac",
      		liboutfile.dirname, mac->macname);
      if ((libfd = fopen (libfile, "w")) != NULL)
      {
         fprintf (libfd,"%-6.6s $MACRO ", mac->macname);
	 for (i = 0; i < mac->macargcount; i++)
	 {
	    if (i) fprintf (libfd, ",");
	    fprintf (libfd, "%s", mac->macargs[i].defsym);
	 }
	 fprintf (libfd, "\n");
	 if (mac->macvarcount)
	 {
	    fprintf (libfd,"%-6.6s $VAR   ", " ");
	    for (i = 0; i < mac->macvarcount; i++)
	    {
	       if (i) fprintf (libfd, ",");
	       fprintf (libfd, "%s", mac->macargs[mac->macargcount + i].defsym);
	    }
	    fprintf (libfd, "\n");
	 }
	 for (i = 0; i < mac->maclinecount; i++)
	 {
	    fprintf(libfd, "%s", mac->maclines[i]->source);
	 }
	 fclose (libfd);
      }
   }

   macrocount++;

   pscanbuf = inbuf;
   return (0);
}

/***********************************************************************
* p0pop - Process Pseudo operation codes.
***********************************************************************/

static int
p0pop (OpCode *op, char *bp, FILE *infd, FILE *outfd, int inmacro)
{
   SymNode *s;
   char *token;
   char *cp;
   int tokentype;
   int relocatable;
   int val;
   int i, j;
   char term;

   switch (op->opvalue)
   {

   case AORG_T:
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

   case ASG_T:
   case CALL_T:
   case ELSE_T:
   case MEND_T:
   case ENDIF_T:
   case EXIT_T:
   case GOTO_T:
   case IF_T:
   case NAME_T:
   case SBSTG_T:
   case VAR_T:
      if (!inmacro)
      {
	 sprintf (errtmp, "Operation %s not in a MACRO", op->opcode);
	 logerror (errtmp);
      }
      break;

   case ASMIF_T:
      if (inmacro) goto MACERROR;
      bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      inskip = if_start (val);
      break;

   case ASMELS_T:
      if (inmacro) goto MACERROR;
      inskip = if_else();
      break;

   case ASMEND_T:
      if (inmacro) goto MACERROR;
      inskip = if_end();
      break;

   case BES_T:
      bp = exprscan (bp, &val, &term, &relocatable, 2, TRUE, 0);
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
      bp = exprscan (bp, &val, &term, &relocatable, 2, TRUE, 0);
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
      if (inmacro) goto MACERROR;
      tmpfd = opencopy (bp, tmpfd);
      gblmode = INSERT | ((filenum ? fileseq : 0) << NESTSHFT);
      break;

   case DATA_T:
      if (pc & 0x0001) pc++;
#ifdef DEBUGPC
      printf ("DATA pc = %04X\n", pc);
#endif
      if (cursym[0])
      {
	 if ((s = symlookup (cursym, FALSE, FALSE)) == NULL)
	    s = symlookup (cursym, TRUE, FALSE);
	 s->value = pc;
      }
      do {
	 while (*bp && isspace(*bp)) bp++;
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

   case DFOP_T:
      bp = tokscan (bp, &token, &tokentype, &val, &term);
      if (term == ',')
      {
	 OpCode *op;
	 MacDef *lclmac;
         char newop[MAXSYMLEN];

	 strcpy (newop, token);
	 bp = tokscan (bp, &token, &tokentype, &val, &term);
	 if ((lclmac = maclookup (token)) != NULL)
	 {
	    MacDef *mac;

	    if ((mac = maclookup (newop)) == NULL)
	    {
	       mac = &macdef[macrocount];
	       macrocount++;
	    }
	    else
	    {
	       for (i = 0; mac->maclinecount; i++)
	       {
		  mac->maclines[i]->next = freemaclines;
		  freemaclines = mac->maclines[i];
	       }
	    }
	    memset (mac, '\0', sizeof (MacDef));
	    strcpy (mac->macname, newop);
	    mac->macstartline = linenum;
	    mac->macargcount = lclmac->macargcount;
	    mac->macvarcount = lclmac->macvarcount;
	    mac->maclinecount = lclmac->maclinecount;
	    for (i = 0; i < lclmac->macargcount; i++)
	    {
	       memcpy (&mac->macargs[i], &lclmac->macargs[i], sizeof(MacArg));
	    }
	    for (i = 0; i < lclmac->maclinecount; i++)
	    {
	       p0allocline (mac, i, NULL, 0);
	       memcpy (mac->maclines[i], lclmac->maclines[i], sizeof(MacLines));
	    }
	 }
	 else
	 {
	    if ((op = oplookup (token, 0)) != NULL)
	    {
	       opdel (newop);
	       opadd (op, newop);
	    }
	    else
	    {
	       sprintf (errtmp, "Undefined opcode: %s", token);
	       logerror (errtmp);
	    }
	 }
      }
      else
      {
         logerror ("DFOP missing operation");
      }
      break;

   case DORG_T:
      if (!absolute && !incseg && !indseg && !indorg) rorgpc = pc;
      bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      if (val < 0 || val > 65535)
      {
	 val = 0;
      }
      absolute = TRUE;
      indorg = TRUE;
      pc = val & 0xFFFE;
#ifdef DEBUGDORG
      printf ("p0: DORG: pc = %04X\n", pc);
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

   case EQU_T:
      bp = exprscan (bp, &val, &term, &relocatable, 2, TRUE, 0);
#if defined(DEBUGPC) || defined(DEBUGDORG)
      printf ("p0: EQU pc = %04X, val = %04X\n", pc, val);
#endif
      if (cursym[0])
      {
	 if ((s = symlookup (cursym, FALSE, FALSE)) == NULL)
	    s = symlookup (cursym, TRUE, FALSE);
	 s->value = val;
	 s->line = linenum;
	 if (relocatable)
	    s->flags |= RELOCATABLE;
	 else
	    s->flags &= ~(RELOCATABLE | DSEGSYM | CSEGSYM);
#if defined(DEBUGPC) || defined(DEBUGDORG)
	 printf ("   newval = %04X\n", s->value);
#endif
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
	 if ((s = symlookup (cursym, FALSE, FALSE)) != NULL)
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

   case LIBIN_T:
      if (inmacro) goto MACERROR;
      else
      {
	 struct stat dirstat;

	 cp = libinfiles[libincnt].dirname;
	 while (*bp && !isspace(*bp)) *cp++ = *bp++;
	 *cp = '\0';
	 cp = libinfiles[libincnt].dirname;
	 if (stat (cp, &dirstat) < 0)
	 {
	    sprintf (errtmp, "Can't open directory for LIBIN: %s: %s",
		     cp, strerror(errno));
	    logerror (errtmp);
	    return (FALSE);
	 }
	 if (!S_ISDIR(dirstat.st_mode))
	 {
	    sprintf (errtmp, "Pathname for LIBIN is not a directory: %s",
		     cp);
	    logerror (errtmp);
	    return (FALSE);
	 }
	 strcat (cp, "/");
	 libincnt++;
      }
      break;

   case LIBOUT_T:
      if (inmacro) goto MACERROR;
      else
      {
	 struct stat dirstat;

	 cp = liboutfile.dirname;
	 while (*bp && !isspace(*bp)) *cp++ = *bp++;
	 *cp = '\0';
	 cp = liboutfile.dirname;
	 if (stat (cp, &dirstat) < 0)
	 {
	    sprintf (errtmp, "Can't open directory for LIBOUT: %s: %s",
		     cp, strerror(errno));
	    logerror (errtmp);
	    return (FALSE);
	 }
	 if (!S_ISDIR(dirstat.st_mode))
	 {
	    sprintf (errtmp, "Pathname for LIBOUT is not a directory: %s",
		     cp);
	    logerror (errtmp);
	    return (FALSE);
	 }
	 strcat (cp, "/");
	 libout = TRUE;
      }
      break;

   case MACRO_T:		/* Macro */
      if (p0macro (bp, infd, outfd))
      {
	 return (TRUE);
      }
      break;

   case NOP_T:
   case RT_T:
      if (pc & 0x0001) pc++;
      pc += 2;
      break;

   case OPTION_T:
      if (inmacro) goto MACERROR;
      do {
	 bp = tokscan (bp, &token, &tokentype, &val, &term);
	 if (tokentype == SYM)
	 {
	    j = strlen (token);
	    if (!(strncmp (token, "XREF", j))) genxref = TRUE;
	    else if (!(strncmp (token, "SYMT", j))) gensymt = TRUE;
	 }
	 else if (tokentype == DECNUM)
	 {
	    if (val == 10) model = 10;
	    else if (val == 12) model = 12;
	 }
      } while (term == ',');
      break;

   case RORG_T:
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

   case SETMNL_T:
      if (inmacro) goto MACERROR;
      bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      maxnest = val;
      break;

   case SETRM_T:
      if (inmacro) goto MACERROR;
      bp = exprscan (bp, &val, &term, &relocatable, 2, FALSE, 0);
      if (val >= 10 && val <= 80)
	 rightmargin = val;
      else
      {
	 sprintf (errtmp, "Invalid SETRM value: %d", val);
	 logerror (errtmp);
      }
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

   case XVEC_T:
      pc += 4;
      break;

   case CEND_T:
      if (inmacro) goto MACERROR;
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = pc;
	 s->flags |= RELOCATABLE;
	 s->flags |= CSEGSYM;
      }
      incseg = FALSE;
      absolute = FALSE;
      relocatable = TRUE;
      inpseg = TRUE;
      pc = rorgpc;
      break;

   case DEND_T:
      if (inmacro) goto MACERROR;
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
      if (inmacro) goto MACERROR;
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
	 incseg = FALSE;
	 pc = rorgpc;
      }
      indorg = FALSE;
      inpseg = TRUE;
      return (TRUE);

   case CSEG_T:
      if (inmacro) goto MACERROR;
      if (!absolute && !incseg && !indseg && !indorg) rorgpc = pc;
      if (indseg)
      {
	 dseglength = pc;
	 indseg = FALSE;
      }
      absolute = FALSE;
      indorg = FALSE;
      inpseg = FALSE;
      relocatable = TRUE;
      incseg = TRUE;
      pc = 0;
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = pc;
	 s->flags |= RELOCATABLE;
	 s->flags |= CSEGSYM;
      }
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
         strcpy (errtmp, "$BLANK");
	 cp = errtmp;
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
      if (!j)
      {
	 strcpy (csegs[csegcnt].name, cp);
	 csegs[csegcnt].length = 0;
	 csegs[csegcnt].number = csegcnt + 1;
	 curcseg = csegcnt;
	 csegcnt++;
      }
      break;

   case DSEG_T:
      if (inmacro) goto MACERROR;
      if (!absolute && !incseg && !indseg && !indorg) rorgpc = pc;
      if (incseg)
      {
         csegs[curcseg].length = pc;
	 incseg = FALSE;
      }
      absolute = FALSE;
      relocatable = TRUE;
      indseg = TRUE;
      inpseg = FALSE;
      indorg = FALSE;
      incseg = FALSE;
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
      if (inmacro) goto MACERROR;
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
      if (cursym[0])
      {
	 s = symlookup (cursym, FALSE, FALSE);
	 s->value = pc;
	 s->flags |= RELOCATABLE;
      }
      break;

   default: ;
   }

   return (FALSE);

MACERROR:
   sprintf (errtmp, "Operation %s not allowed in a MACRO", op->opcode);
   logerror (errtmp);
   return (FALSE);
}

/***********************************************************************
* p0localattr - Process local symbol attributes.
***********************************************************************/

static void
p0localattr (MacDef *mac, int index, int attribute, int quoteit, char *gentoken)
{
   MacArg *macarg;

#ifdef DEBUGMACROEXP1
   printf ("p0localattr: index = %d, attribute = %d\n", index, attribute);
#endif
   if (mac->macargs[index].ptr)
      macarg = mac->macargs[index].ptr;	/* It's a varaiable */
   else
      macarg = &mac->macargs[index];    /* It's an argument */

   switch (attribute)
   {
   case QUAL_A: /* :VV.A: */
      sprintf (gentoken, "%d", macarg->attribute);
      break;
   case QUAL_L: /* :VV.L: */
      sprintf (gentoken, "%d", macarg->length);
      break;
   case QUAL_S: /* :VV.S: */
      if (quoteit)
	 sprintf (gentoken, "'%s'", macarg->usersym);
      else
	 strcpy (gentoken, macarg->usersym);
      break;
   case QUAL_NS: /* VV.S */
      sprintf (gentoken, ">%02X%02X", 
	       macarg->usersym[0], macarg->usersym[1]);
      break;
   case QUAL_V: /* :VV.V: */
      sprintf (gentoken, "%d", macarg->value);
      break;
   default:
      gentoken[0] = '\0';
   }
}

/***********************************************************************
* p0linkedattr - Process linked symbol attributes.
***********************************************************************/

static void
p0linkedattr (MacDef *mac, int index, int attribute, char *gentoken)
{
   MacArg *macarg;

   if (mac->macargs[index].ptr)
      macarg = mac->macargs[index].ptr;	/* It's a varaiable */
   else
      macarg = &mac->macargs[index];    /* It's an argument */

   switch (attribute)
   {
   case QUAL_SA: /* :VV.SA: */
      sprintf (gentoken, "%d", macarg->attribute);
      break;
   case QUAL_SL: /* :VV.SL: */
      sprintf (gentoken, "%d", (int)strlen(macarg->usersym));
      break;
   case QUAL_SS: /* :VV.SS: */
      strcpy (gentoken, macarg->usersym);
      break;
   case QUAL_SV: /* :VV.SV: */
      sprintf (gentoken, "%d", macarg->value);
      break;
   case QUAL_SU: /* :VV.SU: */
      sprintf (gentoken, "%d", macarg->uattribute);
      break;
   default:
      gentoken[0] = '\0';
   }
}

/***********************************************************************
* p0setattr - Set linked symbol attributes.
***********************************************************************/

static void
p0setattr (MacDef *mac, int index, int attribute, int val, char *genexpr)
{
   MacArg *macarg;

   if (mac->macargs[index].ptr)
      macarg = mac->macargs[index].ptr;	/* It's a varaiable */
   else
      macarg = &mac->macargs[index];    /* It's an argument */

   switch (attribute)
   {
   case QUAL_SA: /* :VV.SA: */
      macarg->attribute = val;
      break;
   case QUAL_SL: /* :VV.SL: */
      macarg->length = strlen(genexpr);
      break;
   case QUAL_SS: /* :VV.SS: */
      strcpy (macarg->usersym, genexpr);
      break;
   case QUAL_SV: /* :VV.SV: */
      macarg->value = val;
      break;
   case QUAL_SU: /* :VV.SU: */
      macarg->uattribute = val;
      break;
   default: ;
   }
}

/***********************************************************************
* p0macexpr - Expression in macro.
***********************************************************************/

static void
p0macexpr (MacDef *mac, char *exp, char *var, int insbstg)
{
   char *ocp, *cp;
   int j, k;
   int val;
   int gentype;
   int vartype;
   int relocatable;
   char term;
   char genindex[3];
   char varindex[3];
   char genexpr[MAXLINE];
   char gentoken[MAXLINE];

#ifdef DEBUGMACROEXP
   printf ("p0macexpr: exp = %s, var = %s, SBSTG = %d\n", exp, var, insbstg);
#endif
   memset (genexpr, 0, MAXLINE);
   ocp = exp;
   while ((cp = strchr(ocp, '#')) != NULL)
   {
      int tc;

      tc = strlen (genexpr);
      strncat (genexpr, ocp, cp-ocp);
      genexpr[tc+(cp-ocp)] = '\0';
      cp++;
      strncpy (genindex, cp, 2);
      genindex[2] = '\0';
      j = atoi (genindex);
      cp += 2;
      gentype = *cp++ - '0';
      switch (gentype)
      {
      case QUAL_A: /* :VV.A: */
      case QUAL_L: /* :VV.L: */
      case QUAL_S: /* :VV.S: */
      case QUAL_V: /* :VV.V: */
      case QUAL_NS: /* VV.S */
	 p0localattr (mac, j, gentype, TRUE, gentoken);
	 break;
      case QUAL_SA: /* :VV.SA: */
      case QUAL_SL: /* :VV.SL: */
      case QUAL_SS: /* :VV.SS: */
      case QUAL_SV: /* :VV.SV: */
      case QUAL_SU: /* :VV.SU: */
	 p0linkedattr (mac, j, gentype, gentoken);
	 break;
      default: 
         gentoken[0] = '\0';
      }
#ifdef DEBUGMACROEXP
      printf ("   gentoken = %s\n", gentoken);
#endif
      strcat (genexpr, gentoken);
      ocp = cp;
   }
   strcat (genexpr, ocp);
#ifdef DEBUGMACROEXP
   printf ("   genexpr = %s\n", genexpr);
#endif
   ocp = cp = genexpr;

   if (strchr (genexpr, '\''))
   {
      while (*cp)
      {
         if (*cp != '\'')
	    *ocp++ = *cp;
	 cp++;
	 if (insbstg) *ocp = '\0';
	 if (insbstg && *cp == ',')
	    break;
      }
      *ocp = '\0';
      if (insbstg)
      {
	 int start, len;
	 ocp = gentoken;
         cp++;
	 while (*cp && *cp != ',') *ocp++ = *cp++;
	 *ocp = '\0';
	 exprscan (gentoken, &start, &term, &relocatable, 2, FALSE, 0);
	 start--;
	 ocp = gentoken;
	 cp++;
	 while (*cp && *cp != ',') *ocp++ = *cp++;
	 *ocp = '\0';
	 exprscan (gentoken, &len, &term, &relocatable, 2, FALSE, 0);
	 if (start < strlen(genexpr))
	 {
	    int len2;
	    len2 = strlen (&genexpr[start]);
	    strncpy (gentoken, &genexpr[start], len);
	    if (len2 > len)
	       gentoken[len] = '\0';
	    else
	       gentoken[len2] = '\0';
	 }
	 else
	    gentoken[0] = '\0';
	 strcpy (genexpr, gentoken);
#ifdef DEBUGMACROEXP
	 printf ("   subexpr = %s\n", genexpr);
#endif
      }
      val = 0;
   }
   else
   {
#ifdef DEBUGMACROEXP
      printf ("   expr = %s\n", ocp);
#endif
      exprscan (ocp, &val, &term, &relocatable, 2, FALSE, 0);
   }

   if ((cp = strchr (var, '#')) != NULL)
   {
      MacArg *macarg;

      cp++;
      strncpy (varindex, cp, 2);
      varindex[2] = '\0';
      k = atoi (varindex);
      cp += 2;
      vartype = *cp++ - '0';
      if (mac->macargs[k].ptr)
	 macarg = mac->macargs[k].ptr;	/* It's a varaiable */
      else
	 macarg = &mac->macargs[k];    /* It's an argument */

      switch (vartype)
      {
      case QUAL_A: /* VV.A */
         macarg->attribute = val;
	 break;
      case QUAL_L: /* VV.L */
         macarg->length = strlen (genexpr);
	 break;
      case QUAL_S: /* VV.S */
	 strcpy (macarg->usersym, genexpr);
         macarg->length = strlen (genexpr);
	 break;
      case QUAL_V: /* VV.V */
         macarg->value = val;
	 break;
      case QUAL_SA: /* VV.SA */
      case QUAL_SL: /* VV.SL */
      case QUAL_SS: /* VV.SS */
      case QUAL_SV: /* VV.SV */
      case QUAL_SU: /* VV.SU */
         p0setattr (mac, k, vartype, val, genexpr);
	 break;
      case QUAL_ALL:
	 strcpy (macarg->usersym, mac->macargs[j].usersym);
         if (k >= mac->macargcount)
	 {
	    macarg->attribute = mac->macargs[j].attribute;
	    macarg->length = mac->macargs[j].length;
	    macarg->value = mac->macargs[j].value;
	 }
      default: ;
      }
   }
}

/***********************************************************************
* p0subfields - Substitute fields.
***********************************************************************/

static void
p0subfields (MacDef *mac, char *cp, char *token)
{
   char *ocp;
   int done;
   int j;
   char gentoken[MAXLINE];

   done = FALSE;
   while (*cp && !done)
   {
      if ((ocp = strchr(cp, '#')) != NULL)
      {
	 int gentype;
	 char genindex[3];

	 if (ocp > cp)
	    strncat (token, cp, ocp-cp);
	 ocp++;
	 strncpy (genindex, ocp, 2);
	 ocp += 2;
	 gentype = *ocp++ - '0';
	 cp = ocp;
	 genindex[2] = '\0';
	 j = atoi (genindex);
	 switch (gentype)
	 {
	 case QUAL_A: /* :VV.A: */
	 case QUAL_L: /* :VV.L: */
	 case QUAL_S: /* :VV.S: */
	 case QUAL_V: /* :VV.V: */
	 case QUAL_NS: /* VV.S */
	    p0localattr (mac, j, gentype, FALSE, gentoken);
	    break;
	 case QUAL_SA: /* :VV.SA: */
	 case QUAL_SL: /* :VV.SL: */
	 case QUAL_SS: /* :VV.SS: */
	 case QUAL_SV: /* :VV.SV: */
	 case QUAL_SU: /* :VV.SU: */
	    p0linkedattr (mac, j, gentype, gentoken);
	    break;
	 default: 
	    gentoken[0] = '\0';
	 }
	 strcat (token, gentoken);
      }
      else
      {
	 strcat (token, cp);
	 done = TRUE;
      }
   }
}

/***********************************************************************
* macif_start - Start $IF statement.
***********************************************************************/

static int
macif_start (IFTable *iftable, int *ifindex, int val)
{
   int inskip = FALSE;
   int lclindex = *ifindex;

   if ((lclindex + 1) > MAXIFDEPTH)
   {
      logerror ("$IF nesting depth exceeded.");
      return (FALSE);
   }
   lclindex++;
   if (iftable[lclindex-1].skip)
   {
      iftable[lclindex].skip = iftable[lclindex-1].skip;
      inskip = TRUE;
   }
   else
   {
      inskip = iftable[lclindex].skip = val ? FALSE : TRUE;
   }
#ifdef DEBUGMACIF
   printf ("$IF: ifindex = %d, val = %d, inskip = %s\n",
	   lclindex, val, inskip ? "TRUE" : "FALSE");
#endif
   *ifindex = lclindex;
   return (inskip);
}

/***********************************************************************
* macif_else - Prcoess $ELSE statement.
***********************************************************************/

static int
macif_else (IFTable *iftable, int *ifindex)
{
   int inskip = FALSE;
   int lclindex = *ifindex;

   if (lclindex == 0)
      return inskip;

   if (iftable[lclindex-1].skip)
   {
      inskip = TRUE;
   }
   else
   {
      iftable[lclindex].skip = iftable[lclindex].skip ? FALSE : TRUE;
      inskip = iftable[lclindex].skip;
   }
#ifdef DEBUGMACIF
   printf ("$ELSE: ifindex = %d, inskip = %s\n",
	   lclindex, inskip ? "TRUE" : "FALSE");
#endif
   return (inskip);
}

/***********************************************************************
* macif_end - Prcoess $END statement.
***********************************************************************/

static int
macif_end (IFTable *iftable, int *ifindex)
{
   int inskip = FALSE;
   int lclindex = *ifindex;

   if (lclindex == 0)
      return inskip;

   if (iftable[lclindex-1].skip)
   {
      inskip = TRUE;
   }
#ifdef DEBUGMACIF
   printf ("$ENDIF: lclindex = %d, inskip = %s\n",
	   lclindex, inskip ? "TRUE" : "FALSE");
#endif
   lclindex--;
   if (lclindex < 0)
      lclindex = 0;
   *ifindex = lclindex;
   return (inskip);
}

/***********************************************************************
* p0expand - Expand macros
***********************************************************************/

static void
p0expand (MacDef *mac, char *bp, FILE *infd, FILE *outfd)
{
   OpCode *op;
   MacDef *lclmac;
   char *token;
   char *cp, *ep;
   char *ocp;
   int tokentype;
   int val;
   int i, j;
   int macexit;
   int genmode;
   int argcnt;
   int macinskip;
   int macifindex;		/* $IF table index. */
   char term;
   char genline[MAXLINE];
   IFTable maciftbl[MAXIFDEPTH];/* In skip line mode $IF */

   ep = bp + rightmargin;
   while (*bp && isspace(*bp)) bp++;
#ifdef DEBUGMACROEXP
   printf ("\np0expand: macro = %s, macnest = %d, cnt = %d, args = %s",
	   mac->macname, macnest, mac->macargcount, bp);
   if (*bp == 0) printf ("\n");
#endif

   /* Macro $IF local to this expansion */

#if defined(DEBUGMACIF) && !defined(DEBUGMACROEXP)
   printf ("\np0expand: macro =  %s, macnest = %d\n",
	   mac->macname, macnest);
#endif
   macinskip = FALSE;
   macifindex = 0;
   memset (maciftbl, 0, sizeof(maciftbl));

   for (i = 0; i < mac->macargcount; i++)
   {
      mac->macargs[i].usersym[0] = '\0';
      mac->macargs[i].length = 0;
      mac->macargs[i].attribute = 0;
      mac->macargs[i].uattribute = 0;
      mac->macargs[i].value = 0;
   }

   /* Point VARiables to global table. */

   for (i = 0; i < mac->macvarcount; i++)
   {
      int ndx;
      
      ndx = mac->macargcount + i;
      mac->macargs[ndx].ptr = NULL;
      for (j = 0; j < macvarscount; j++)
      {
         if (!strcmp (mac->macargs[ndx].defsym, macvars[j].defsym))
	    mac->macargs[ndx].ptr = &macvars[j];
      }
   }

   macnest++;
   if (macnest > maxnest)
   {
      sprintf (genline, "Macro nesting limit exceeded, limit = %d", maxnest);
      logerror (genline);
      return;
   }

   argcnt = 0;

   /*
   ** Scan off MACRO arguments
   */

   if (mac->macargcount && *bp && (bp < ep))
   {
      do {
	 char tok[MAXLINE];

	 ocp = bp;
	 tok[0] = '\0';
	 token = tok;
	 while (*bp && !isspace(*bp) && *bp != ',')
	 {
	     *token++ = *bp++;
	 }
	 *token = '\0';
	 token = tok;
	 term = *bp++;
#ifdef DEBUGMACROEXP
	 printf ("   token = %s, term = %c(%x)\n",
		  token, term, term);
#endif
	 if (token[0] == '\0' && term == '(')
	 {
	    ocp = bp;
	    i = 1;
	    while (*bp && i > 0/* && !isspace(*bp)*/)
	    {
	       if (*bp == '(')
	       {
		  i++;
		  bp++;
	       }
	       else if (*bp == ')')
	       {
		  i--;
		  if (i)
		     bp++;
	       }
	       else bp++;
	    }
#ifdef DEBUGMACROEXP
	    printf ("   *bp = %c(%x) %s, pocp = %s\n", *bp, *bp, bp, ocp);
#endif
	    *bp++ = '\0';
	    strcat (mac->macargs[argcnt].usersym, ocp);
	    term = *bp;
	    if (term == ',') bp++;
	    else if (isalnum(term) || term == '.')
	       term = ',';
	 }
	 else
	 {
	    strcat (mac->macargs[argcnt].usersym, token);
	    if (term == '(')
	    {
		bp--;
		argcnt++;
		if (argcnt >= mac->macargcount)
		   break;
	    }
	 }
	 if (isspace (term) || term == ',') argcnt++;
      } while (!isspace(term));
   }

   /*
   ** Build macro argument attributes
   */

   for (i = 0; i < argcnt; i++)
   {
      MacArg *lma = &mac->macargs[i];
      char *lcp = mac->macargs[i].usersym;
      char *lpp;
      int len = strlen (lcp);
      int relocatable;
      int val;
      char term;
      char lclexpr[MAXLINE];

      lma->length = len;
      lma->value = 0;
      lma->attribute = ATTR_PCALL;
      if (*lcp == LITERALSYM)
      {
	 lma->attribute |= ATTR_PLIT;
      }
      else if (*lcp == MEMSYM && ((lpp = strchr (lcp, '(')) == NULL))
      {
	 lma->attribute |= ATTR_PSYM;
      }
      else if (*lcp == '*' && *(lcp+len-1) == '+')
      {
	 lma->attribute |= ATTR_PATO;
	 memset (lclexpr, '\0', len+2);
	 strncpy (lclexpr, lcp+1, len-2);
	 exprscan (lclexpr, &val, &term, &relocatable, 2, FALSE, 0);
	 lma->value = val;
      }
      else if (*lcp == '*' && *(lcp+len-1) != '+')
      {
	 lma->attribute |= ATTR_PIND;
	 memset (lclexpr, '\0', len+2);
	 strncpy (lclexpr, lcp+1, len-1);
	 exprscan (lclexpr, &val, &term, &relocatable, 2, FALSE, 0);
	 lma->value = val;
      }
      else if ((lpp = strchr (lcp, '(')) != NULL)
      {
	 char *rpp;
	 lma->attribute |= ATTR_PNDX;
	 lpp++;
	 if((rpp = strrchr (lpp, ')')) != NULL)
	 {
	    memset (lclexpr, '\0', len+2);
	    strncpy (lclexpr, lpp, (rpp-lpp));
	    exprscan (lclexpr, &val, &term, &relocatable, 2, FALSE, 0);
	    lma->value = val;
	 }
      }
      else
      {
	 SymNode *s;

	 if (isdigit(lcp[0]))
	 {
	    lma->value = atoi (lcp);
	 }
	 else if (lcp[0] == '>')
	 {
	    lma->value = strtol (&lcp[1], NULL, 16);
	 }
	 else if ((s = symlookup (lcp, FALSE, FALSE)) != NULL)
	 {
	    lma->value = s->value;
	    lma->attribute |= ATTR_VAL;
	 }
	 else
	 {
	    lma->attribute |= ATTR_UNDEF;
	    if ((lclmac = maclookup (lcp)) != NULL)
	    {
	       lma->attribute |= ATTR_MAC;
	    }
	 }
      }

#ifdef DEBUGMACROEXP
      printf ("   arg%02d = %s, len = %d, attribute = %04x, value = %d\n",
      	      i, lcp, lma->length, lma->attribute, lma->value);
#endif
   }

   /*
   ** Go through MACRO and fill in missing labels with generated labels.
   */

   for (i = 0; i < mac->maclinecount; i++)
   {
      char genindex[3];

      if (mac->maclines[i]->ismodel)
         continue;

      cp = mac->maclines[i]->label;
      if ((ocp = strchr(cp, '#')) != NULL)
      {
	 int gentype;

	 ocp++;
	 strncpy (genindex, ocp, 2);
	 genindex[2] = '\0';
	 ocp += 2;
	 gentype = *ocp++ - '0';
	 j = atoi (genindex);
	 if (j < mac->macargcount && !mac->macargs[j].usersym[0])
	 {
	    sprintf (mac->macargs[j].usersym, "A;;%03d", lblgennum);
	    lblgennum++;
	 }
      }
   }

   /*
   ** Go through MACRO template and replace parameters with the
   ** user arguments.
   */

   macexit = FALSE;
   for (i = 0; i < mac->maclinecount && !macexit; i++)
   {
      int ismodel;
      char genlabel[MAXSYMLEN+1];
      char genopcode[MAXSYMLEN+1];
      char genoperand[MAXLINE+1];

      /*
      ** If a model template, just substitute fields and pass on.
      */

      genlabel[0] = '\0';
      genopcode[0] = '\0';
      genoperand[0] = '\0';
      genline[0] = '\0';
      op = NULL;
      ismodel = FALSE;
      genmode = macnest << NESTSHFT;

      /*
      ** If model statement, process.
      */

      if (mac->maclines[i]->ismodel)
      {
         p0subfields (mac, mac->maclines[i]->operand, genline);

	 cp = genline;
	 if (!isspace (*cp))
	 {
	    ocp = genlabel;
	    while (*cp && !isspace(*cp)) *ocp++ = *cp++;
	    *ocp = '\0';
	    strcpy (cursym, genlabel);
	 }
	 while (*cp && isspace (*cp)) cp++;
	 if (*cp)
	 {
	    ocp = genopcode;
	    while (*cp && !isspace(*cp)) *ocp++ = *cp++;
	    *ocp = '\0';
	 }
	 while (*cp && isspace (*cp)) cp++;
	 if (*cp)
	 {
	    ocp = genoperand;
	    while (*cp) *ocp++ = *cp++;
	    *ocp = '\0';
	 }
	 if (((op = oplookup (genopcode, 0)) == NULL) &&
	     (maclookup (genopcode) == NULL))
	 {
	    ismodel = TRUE;
#ifdef DEBUGMACROEXP
	    printf ("   %sgenline = %s\n", macinskip ? "XX " : "", genline);
#endif
	 }
	 strcat (genline, "\n");
      }

      /*
      ** Not a model, process here.
      */

      else 
      {
	 /*
	 ** Process label field
	 */

	 p0subfields (mac, mac->maclines[i]->label, genlabel);

	 if (strlen(genlabel) > MAXSYMLEN) genlabel[MAXSYMLEN] = '\0';
	 strcpy (cursym, genlabel);

	 /*
	 ** Process opcode field
	 */

	 p0subfields (mac, mac->maclines[i]->opcode, genopcode);

	 /*
	 ** Lookup opcode for macro expansion control
	 */

	 op = oplookup (genopcode, 0);

	 /*
	 ** If opcode is $ASG or $SBSTG, process operands here.
	 */

	 if (op && op->optype == TYPE_P &&
	    (op->opvalue == ASG_T || op->opvalue == SBSTG_T))
	 {
	    char targ[MAXLINE];

	    cp = mac->maclines[i]->operand;
	    strcpy (genoperand, cp);
	    ocp = cp;
	    while (*cp && !isspace(*cp))
	    {
	       if (*cp == '\'') 
	       {
		  cp++;
		  while (*cp && *cp != '\'') cp++;
	       }
	       cp++;
	    }
	    strncpy (targ, ocp, cp-ocp);
	    targ[cp-ocp] = '\0';
	    while (*cp && isspace(*cp)) cp++;
#ifdef DEBUGMACROEXP
	    printf ("   targ = %s\n", targ);
	    printf ("   cp = %s\n", cp);
#endif
	    if (!macinskip && !strncmp(cp, "TO ", 3))
	    {
	       cp += 3;
	       p0macexpr (mac, targ, cp, op->opvalue == SBSTG_T ? TRUE : FALSE);
	    }
	 }
	 else
	 {
	    /*
	    ** Process operand field normally
	    */

	    genoperand[0] = '\0';
	    p0subfields (mac, mac->maclines[i]->operand, genoperand);
	 }

#ifdef DEBUGMACROEXP
	 printf ("   %sgenlabel = %s\n", macinskip ? "XX " : "", genlabel);
	 printf ("   %sgenopcode = %s\n", macinskip ? "XX " : "", genopcode);
	 printf ("   %sgenoperand = %s\n", macinskip ? "XX " : "", genoperand);
#endif
	 sprintf (genline, "%-6.6s %-7.7s %s\n",
	 	  genlabel, genopcode, genoperand);
      }
      
      /*
      ** If not skipping, $IF, process
      */

      if (!macinskip)
      {
	 int processed = FALSE;

	 /*
	 ** Check Pseudo ops that control macro
	 */

	 if (op && op->optype == TYPE_P)
	 {
	    int relocatable;
	    char psopline[MAXLINE];

	    processed = TRUE;
	    switch (op->opvalue)
	    {
	    case ELSE_T:
	    case ENDIF_T:
	    case IF_T:
#ifdef DEBUGMACROEXP
	       genmode |= MACEXP;
	       genlinenum++;
	       writesource (outfd, genmode, genlinenum, genline, "g01");
#endif
	       strcpy (psopline, genoperand);
	       cp = psopline;
	       if (!macexit) switch (op->opvalue)
	       {
	       case IF_T:
		  cp = exprscan (cp, &val, &term, &relocatable, 2, FALSE, 0);
		  macinskip = macif_start (maciftbl, &macifindex, val);
		  break;

	       case ELSE_T:
		  macinskip = macif_else (maciftbl, &macifindex);
		  break;

	       case ENDIF_T:
		  macinskip = macif_end (maciftbl, &macifindex);
		  break;
	       }
	       break;

	    case NAME_T:
	       mac->maclines[i]->ifindex = macifindex;

	    case ASG_T:
	    case SBSTG_T:
	    case VAR_T:
#ifdef DEBUGMACROEXP
	       genmode |= MACEXP;
	       genlinenum++;
	       writesource (outfd, genmode, genlinenum, genline, "g02");
#endif
	       break;

	    case EXIT_T:
#ifdef DEBUGMACIF
	       printf ("$EXIT: ifindex = %d, inskip = %s\n",
		       macifindex, macinskip ? "TRUE" : "FALSE");
#endif
	       macifindex = 0;
#ifdef DEBUGMACROEXP
	       genmode |= MACEXP;
	       genlinenum++;
	       writesource (outfd, genmode, genlinenum, genline, "g03");
#endif
	       macexit = TRUE;
	       break;

	    case MEND_T:
#ifdef DEBUGMACIF
	       printf ("$END-0: ifindex = %d, inskip = %s\n",
		       macifindex, macinskip ? "TRUE" : "FALSE");
#endif
	       if (macifindex)
	       {
		  sprintf (errtmp, "Non terminated $IF in MACRO: %s",
			   mac->macname);
		  logerror (errtmp);
	       }
	       macexit = TRUE;
	       break;

	    case GOTO_T:
#ifdef DEBUGMACIF
	       printf ("$GOTO: ifindex = %d, inskip = %s\n",
		       macifindex, macinskip ? "TRUE" : "FALSE");
#endif
	       macifindex = 0;
#ifdef DEBUGMACROEXP
	       genmode |= MACEXP;
	       genlinenum++;
	       writesource (outfd, genmode, genlinenum, genline, "g04");
#endif
	       cp = tokscan (genoperand, &token, &tokentype, &val, &term);
	       if (strlen(token) > MAXSHORTSYMLEN)
		  token[MAXSHORTSYMLEN] = '\0';
#ifdef DEBUGMACROEXP
	       printf ("   GOTO: token = '%s'\n", token);
#endif
	       for (j = 0; j < mac->maclinecount; j++)
	       {
		  if (!strcmp (mac->maclines[j]->label, token))
		  {
		     if (!strcmp (mac->maclines[j]->opcode, "$NAME"))
		     {
			i = j - 1;
			macifindex = mac->maclines[j]->ifindex;
		     }
		     else if (!strcmp (mac->maclines[j]->opcode, "$END"))
			macexit = TRUE;
		     break;
		  }
	       }
	       if (j == mac->maclinecount)
	       {
		  sprintf (errtmp, "$GOTO label '%s' not found", token);
		  logerror (errtmp);
	       }
	       break;

	    case MACRO_T:
#ifdef DEBUGMACROEXP
	       genmode |= MACEXP;
	       genlinenum++;
	       writesource (outfd, genmode, genlinenum, genline, "g05");
#endif
	       break;

	    case CALL_T:
#ifdef DEBUGMACROEXP
	       genmode |= MACEXP;
	       genlinenum++;
	       writesource (outfd, genmode, genlinenum, genline, "g06");
#endif
	       cp = tokscan (genoperand, &token, &tokentype, &val, &term);
	       if ((lclmac = maclookup (token)) != NULL)
	       {
#ifdef DEBUGMACROEXP
		  printf ("macro opcode = %s\n", token);
#endif
		  char macoperand[MAXLINE];

		  macoperand[0] = '\0';
		  for (j = 0; j < mac->macargcount; j++)
		  {
		     if (macoperand[0]) strcat (macoperand, ",");
		     strcat (macoperand, mac->macargs[j].usersym);
		  }
#ifdef DEBUGMACROEXP
		  printf ("   macoperand = '%s'\n", macoperand);
#endif
		  p0expand (lclmac, macoperand, infd, outfd);
	       }
	       break;

	    default:
	       genmode |= MACEXP;
	       genlinenum++;
	       writesource (outfd, genmode, genlinenum, genline, "g07");
	       p0pop (op, genoperand, infd, outfd, TRUE);
	    }
	 }

	 /*
	 ** Check if macro is called in macro
	 */

	 else if (!op && ((lclmac = maclookup (genopcode)) != NULL))
	 {
#ifdef DEBUGMACROEXP
	    printf ("macro opcode = %s\n", genopcode);
#endif
	    processed = TRUE;
	    genmode |= (MACEXP | MACCALL);
	    genlinenum++;
	    writesource (outfd, genmode, genlinenum, genline, "g08");
	    p0expand (lclmac, genoperand, infd, outfd);
	 }

	 /*
	 ** If LIBIN's active check if it is a MACRO.
	 */

	 else if (!op && libincnt)
	 {
	    genmode |= MACEXP;
	    genlinenum++;
	    if (p0checklibin (genopcode, genoperand, genline, genmode,
			      genlinenum, infd, outfd, "l01"))
	    {
	       processed = TRUE;
	    }
	    else
	    {
	       genlinenum--;
	    }
	 }

	 /*
	 ** None of the above, send it on to next pass
	 */

	 if (!processed)
	 {
	    genmode |= MACEXP;
	    genlinenum++;
	    writesource (outfd, genmode, genlinenum, genline, "g09");
	 }
      }

      /*
      ** In skip, Just send it out.
      */

      else
      {
	 if (op && op->optype == TYPE_P)
	 {
	    int relocatable;

	    switch (op->opvalue)
	    {
	    case IF_T:
	       exprscan (genoperand, &val, &term, &relocatable, 2, FALSE, 0);
	       macinskip = macif_start (maciftbl, &macifindex, val);
	       break;
	    case ELSE_T:
	       macinskip = macif_else (maciftbl, &macifindex);
	       break;
	    case ENDIF_T:
	       macinskip = macif_end (maciftbl, &macifindex);
	       break;
	    case MEND_T:
#ifdef DEBUGMACIF
	       printf ("$END-1: ifindex = %d, inskip = %s\n",
		       macifindex, macinskip ? "TRUE" : "FALSE");
#endif
	       if (macifindex)
	       {
		  sprintf (errtmp, "Non terminated $IF in MACRO: %s",
			   mac->macname);
		  logerror (errtmp);
	       }
	    default: ;
	    }
	 }
#ifdef DEBUGMACROEXP
	 genmode |= MACEXP | SKPINST;
	 genlinenum++;
	 writesource (outfd, genmode, genlinenum, genline, "g10");
#endif
      }
   }
   macnest--;
}

/***********************************************************************
* p0addattributes - Add macro attributes to the symbol table.
***********************************************************************/

static void
p0addattributes (void)
{
   SymNode *s;
   int i;

   for (i = 0; attrtypes[i].symbol[0]; i++)
   {
      if ((s = symlookup (attrtypes[i].symbol, TRUE, FALSE)) != NULL)
      {
	 s->value = attrtypes[i].value;
	 s->flags = P0SYM;
      }
   }
}

/***********************************************************************
* p0checklibin - Check LIBIN for MACRO.
***********************************************************************/

static int
p0checklibin (char *opcode, char *genoperand, char *genline, int genmode,
	      int gennum, FILE *infd, FILE *outfd, char *loc)
{
   FILE *libfd;
   char *cp;
   MacDef *mac = NULL;
   char *token;
   int tokentype;
   int val;
   int i;
   int found;
   char term;
   char libfile[MAXPATHNAMESIZE];

#ifdef DEBUGLIBIN
   printf ("p0checklibin: opcode = %s, line = %d, mode = %X, operand = %s",
	   opcode, gennum, genmode, genoperand);
   printf ("   line = %s", genline);
#endif

   found = FALSE;
   if (libincnt)
   {
      for (i = 0; !found && i < libincnt; i++)
      {
	 strcpy (libfile, libinfiles[libincnt-i-1].dirname);
	 strcat (libfile, opcode);
	 strcat (libfile, ".mac");
#ifdef DEBUGLIBIN
         printf ("   libfile = %s\n", libfile);
#endif
	 if ((libfd = fopen (libfile, "r")) != NULL)
	 {
	    fgets (libfile, MAXLINE, libfd);
	    cp = libfile;

	    if (isalpha(*cp) || *cp == '$' || *cp == '_')
	    {
	       cp = tokscan (cp, &token, &tokentype, &val, &term);
	       strcpy (cursym, token);
	       if (strlen(token) > MAXSYMLEN)
	       {
		  cursym[MAXSYMLEN] = '\0';
		  token[MAXSYMLEN] = '\0';
	       }
	    }
	    while (*cp && isspace(*cp)) cp++;
	    while (*cp && !isspace(*cp)) cp++;
	    while (*cp && isspace(*cp)) cp++;

#ifdef DEBUGLIBIN
	    printf ("   FOUND\n");
#endif
	    p0macro (cp, libfd, NULL);
	    if ((mac = maclookup (opcode)) != NULL)
	    {
	       genmode |= MACCALL;
	       writesource (outfd, genmode, gennum, genline, loc);
	       p0expand (mac, genoperand, infd, outfd);
	    }
	    found = TRUE;
	    fclose (libfd);
	 }
      }
   }
   return (found);
}

/***********************************************************************
* asmpass0 - Pass 0
***********************************************************************/

int
asmpass0 (FILE *tmpfd0, FILE *tmpfd1)
{
   SymNode *s;
   MacDef *mac;
   char *token;
   int i;
   int done;
   int genmode;
   int status = 0;
   int tokentype;
   int val;
   char term;
   char opcode[MAXSYMLEN+2];

#ifdef DEBUGP0RDR
   printf ("asmpass0: Entered\n");
#endif

   /*
   ** Clear out macro table.
   */

   memset ((void *)macdef, 0, sizeof(macdef));
   memset ((void *)macvars, 0, sizeof(macvars));

   /*
   ** Add attribute keywords to the symbol table in SDSMAC mode.
   */

   if (!txmiramode)
      p0addattributes();

   lblgennum = 1;
   macrocount = 0;
   gblmode = 0;
   macnest = 0;
   libincnt = 0;
   macvarscount = 0;
   inskip = FALSE;
   libout = FALSE;
   macscan = FALSE;
   maxnest = DEFMACNEST;
   dseglength = -1;

   /*
   ** Process the source.
   */

   pc = 0;
   rorgpc = 0;
   linenum = 0;

   done = FALSE;

   tmpfd = tmpfd0;

DOREAD:
   while (!done)
   {
      char *bp;

      inbuf[0] = '\0';
      if (!eofflg && !fgets(inbuf, MAXLINE, tmpfd))
      {
	 inbuf[0] = '\0';
	 eofflg = TRUE;
      }

      if (eofflg && !inbuf[0])
      {
	 if ((linenum > 1) && (tmpfd == tmpfd0))
	 {
	    logerror ("No END record");
	    status = 0;
	 }
	 else
	 {
	    status = 1;
	 }
         done = TRUE;
	 break;
      }

      linenum++;
      genlinenum = 0;
      genmode = gblmode;
      if ((bp = strchr (inbuf, '\n')) == NULL)
      {
#ifdef WIN32
        if ((bp = strchr (inbuf, '\r')) == NULL) 
	 strcat (inbuf, "\r");
#endif
	 strcat (inbuf, "\n");
      }

#ifdef DEBUGP0RDR
      printf ("P0in = %s", inbuf);
#endif
      bp = inbuf;

      /*
      ** If not a comment, then process.
      */

      if (*bp != COMMENTSYM)
      {
	 OpCode *op;

	 /*
	 ** If label present, add to symbol table.
	 */

         if (isalpha(*bp) || *bp == '$' || *bp == '_')
	 {
	    int addit = TRUE;

	    bp = tokscan (bp, &token, &tokentype, &val, &term);
	    strcpy (cursym, token);
	    if (strlen(token) > MAXSYMLEN)
	    {
	       cursym[MAXSYMLEN] = '\0';
	       token[MAXSYMLEN] = '\0';
	    }

	    if (!isspace (term))
	    {
	       sprintf (errtmp, "Invalid symbol: %s%c", token, term);
	       logerror (errtmp);
	       status = -1;
	       bp++;
	    }
	    while (*bp && isspace (*bp)) bp++;
	    if (*bp && !strncmp (bp, "$MACRO", 6)) addit = FALSE;
	    else if (inskip) addit = FALSE;

	    if (addit)
	    {
	       if ((s = symlookup (token, TRUE, FALSE)) == NULL)
	       {
		  sprintf (errtmp, "Duplicate symbol: %s", token);
		  logerror (errtmp);
		  status = -1;
	       }
	       else
	       {
		  if (indseg) s->flags |= DSEGSYM;
		  if (incseg)
		  {
		     s->flags |= CSEGSYM;
		     s->csegndx = curcseg + 1;
		  }
#if defined(DEBUGPC) || defined(DEBUGDORG)
		  printf ("p0: cursym = %s, pc = %04X\n", cursym, pc);
#endif
	       }
	    }
	 }
	 else 
	 {
	    cursym[0] = '\0';
	    while (*bp && isspace (*bp)) bp++;
	 }


	 if (*bp == '\0')
	 {
	    writesource (tmpfd1, genmode, linenum, inbuf, "s01");
	    continue;
	 }

	 /*
	 ** Scan off opcode.
	 */

	 bp = tokscan (bp, &token, &tokentype, &val, &term);
	 if (!token[0]) continue;
	 strcpy (opcode, token);
	 while (*bp && isspace(*bp)) bp++;

         if (inskip)
         {
	    writesource (tmpfd1, genmode, linenum, inbuf, "s02");
	    op = oplookup (opcode, 0);
            if (op != NULL && op->optype == TYPE_P)
               switch (op->opvalue)
               {
	       case ASMIF_T:
               case ASMELS_T:
               case ASMEND_T:
                  p0pop (op, bp, tmpfd, tmpfd1, FALSE);
                  break;

               default: ;
               }
         }

	 /*
	 ** Check macro table first, in case of name override.
	 */

	 else if ((mac = maclookup (opcode)) != NULL)
	 {
	    genmode |= MACCALL;
	    writesource (tmpfd1, genmode, linenum, inbuf, "s03");
	    p0expand (mac, bp, tmpfd, tmpfd1);
	 }

	 else
	 {
	    int processed = FALSE;

	    /*
	    ** Check opcodes
	    */

	    op = oplookup (opcode, 0);

	    /*
	    ** If op, process
	    */

	    if (op)
	    {
	       processed = TRUE;
	       if (op->optype == TYPE_P)
	       {
		  if (op->opvalue == MACRO_T) genmode |= MACDEF;
		  writesource (tmpfd1, genmode, linenum, inbuf, "s04");
		  done = p0pop (op, bp, tmpfd, tmpfd1, FALSE);
		  if (done) status = 0;
	       }
	       else
	       {
		  bytecnt = 0;
		  if (pc & 0x0001) pc++;
		  switch (op->optype)
		  {

		     case TYPE_1:
			p012mop (op, bp);
			break; 

		     case TYPE_2:
		     case TYPE_5:
		     case TYPE_7:
		     case TYPE_10:
		     case TYPE_18:
			p010mop (op, bp);
			break; 

		     case TYPE_3:
		     case TYPE_6:
		     case TYPE_4:
		     case TYPE_9:
			p011mop (op, bp);
			break; 

		     case TYPE_8:
			p011iop (op, bp);
			break; 

		     case TYPE_11:
		     case TYPE_12:
		     case TYPE_16:
		     case TYPE_19:
		     case TYPE_21:
			p022mop (op, bp);
			break; 

		     case TYPE_20:
			p022cop (op, bp);
			break; 

		     case TYPE_13:
		     case TYPE_14:
		     case TYPE_15:
			p021mop (op, bp);
			break; 

		     case TYPE_17:
			p020mop (op, bp);
			break; 

		     default: ;
		  }
		  writesource (tmpfd1, genmode, linenum, inbuf, "s05");
	       }
	    }

	    /*
	    ** If LIBIN's active check if it is a MACRO.
	    */

	    else if (libincnt &&
		     p0checklibin (opcode, bp, inbuf, genmode, linenum,
				   tmpfd, tmpfd1, "l02"))
	    {
	       processed = TRUE;
	    }

	    /*
	    ** None of the above, send on
	    */

	    if (!processed)
	    {
	       pc += 2;
	       writesource (tmpfd1, genmode, linenum, inbuf, "s06");
	    }
	 }
      }
      else
      {
	 writesource (tmpfd1, genmode, linenum, inbuf, "s07");
      }
   }

   if ((tmpfd = closecopy (tmpfd)) != NULL)
   {
      if (filenum == 0)
         gblmode = 0;
      done = FALSE;
      eofflg = FALSE;
      goto DOREAD;
   }

   /*
   ** Clean up
   */

   for (i = 0; i < macrocount; i++)
   {
      int j;
      for (j = 0; j < macdef[i].maclinecount; j++)
      {
         macdef[i].maclines[j]->next = freemaclines;
	 freemaclines = macdef[i].maclines[j];
      }
   }

#ifdef DEBUGMULTIFILE
   printf ("asmpass0: %d lines in file\n", linenum);
#endif

   pscanbuf = inbuf;
   return (status);
}
