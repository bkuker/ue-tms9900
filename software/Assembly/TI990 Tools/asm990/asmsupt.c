/***********************************************************************
*
* asmsupt.c - Support routines for the TI 990 assembler.
*
* Changes:
*   05/21/03   DGP   Original.
*   07/10/03   DGP   If external is absolute and val 0, change relocatable.
*   07/23/03   DGP   Added EBCDIC->ASCII for IBM OS/390 cross assemble.
*   08/13/03   DGP   Added XREF support.
*   08/14/03   DGP   Added opencopy and closecopy.
*   08/18/03   DGP   Added parenthesis to expressions.
*   05/20/04   DGP   Added literal support.
*   06/07/04   DGP   Added FLOAT and DOUBLE support.
*   12/26/05   DGP   Added DSEG support.
*   12/30/05   DGP   Changes SymNode to use flags.
*   07/30/07   DGP   Change mktemp to mkstemp.
*   03/16/09   DGP   Correct float/double literal
*   03/27/09   DGP   Unified error reporting using logerror.
*   06/29/09   DGP   Check for duplicate error in logerror.
*   04/22/10   DGP   Strip single quotes in COPY directive.
*   12/01/10   DGP   Added ILL_EXTERNAL error.
*   12/07/10   DGP   Added checkexpr function.
*   12/14/10   DGP   If opencopy fopen fails try lower caseing the name.
*                    Added file number to error processing.
*   12/16/10   DGP   Added a unified source read function readsource().
*   12/16/10   DGP   Added a unified source write function writesource().
*   01/06/11   DGP   Fixed cross reference issues.
*   01/04/14   DGP   Fixed more cross reference issues.
*   11/18/15   DGP   Initialize segvaltype and csegval.
*   10/01/16   DGP   Try COPY open "as is" before adding incldir.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <io.h>
#endif

#include "asmdef.h"
#include "errors.h"

extern int pc;
extern int symbolcount;
extern int rightmargin;
extern int linenum;
extern int indseg;
extern int genxref;
extern int filenum;
extern int fileseq;
extern int errcount;
extern int errnum;
extern int absolute;
extern int pgmlength;
extern int codelength;
extern int p1errcnt;
extern int inpass;
extern int txmiramode;
extern int segvaltype;
extern int csegval;
extern char inbuf[MAXLINE];
extern char incldir[MAXLINE];
extern char *pscanbuf;
extern SymNode *symbols[MAXSYMBOLS];
extern char errline[10][120];
extern COPYfile files[MAXASMFILES];
extern ErrTable p1error[MAXERRORS];

static int memexpr;

#if defined(USS) || defined(OS390)
/*
** EBCDIC to ASCII conversion table. 
*/

unsigned char ebcasc[256] =
{
 /*00  NU    SH    SX    EX    PF    HT    LC    DL */
      0x00, 0x01, 0x02, 0x03, 0x3F, 0x09, 0x3F, 0x7F,
 /*08              SM    VT    FF    CR    SO    SI */
      0x3F, 0x3F, 0x3F, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
 /*10  DE    D1    D2    TM    RS    NL    BS    IL */
      0x10, 0x11, 0x12, 0x13, 0x14, 0x0A, 0x08, 0x3F,
 /*18  CN    EM    CC    C1    FS    GS    RS    US */
      0x18, 0x19, 0x3F, 0x3F, 0x1C, 0x1D, 0x1E, 0x1F,
 /*20  DS    SS    FS          BP    LF    EB    EC */
      0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x0A, 0x17, 0x1B,
 /*28              SM    C2    EQ    AK    BL       */
      0x3F, 0x3F, 0x3F, 0x3F, 0x05, 0x06, 0x07, 0x3F,
 /*30              SY          PN    RS    UC    ET */
      0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x04,
 /*38                    C3    D4    NK          SU */
      0x3F, 0x3F, 0x3F, 0x3F, 0x14, 0x15, 0x3F, 0x1A,
 /*40  SP                                           */
      0x20, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
 /*48             CENT    .     <     (     +     | */
      0x3F, 0x3F, 0x9B, 0x2E, 0x3C, 0x28, 0x2B, 0x7C,
 /*50   &                                           */
      0x26, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
 /*58               !     $     *     )     ;     ^ */
      0x3F, 0x3F, 0x21, 0x24, 0x2A, 0x29, 0x3B, 0x5E,
 /*60   -     /                                     */
      0x2D, 0x2F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
 /*68               |     ,     %     _     >     ? */
      0x3F, 0x3F, 0x7C, 0x2C, 0x25, 0x5F, 0x3E, 0x3F,
 /*70                                               */
      0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
 /*78         `     :     #     @     '     =     " */
      0x3F, 0x60, 0x3A, 0x23, 0x40, 0x27, 0x3D, 0x22,
 /*80         a     b     c     d     e     f     g */
      0x3F, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
 /*88   h     i           {                         */
      0x68, 0x69, 0x3F, 0x7B, 0x3F, 0x3F, 0x3F, 0x3F,
 /*90         j     k     l     m     n     o     p */
      0x3F, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70,
 /*98   q     r           }                         */
      0x71, 0x72, 0x3F, 0x7D, 0x3F, 0x3F, 0x3F, 0x3F,
 /*A0         ~     s     t     u     v     w     x */
      0x3F, 0x7E, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
 /*A8   y     z                       [             */
      0x79, 0x7A, 0x3F, 0x3F, 0x3F, 0x5B, 0x3F, 0x3F,
 /*B0                                               */
      0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
 /*B8                                 ]             */
      0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x5D, 0x3F, 0x3F,
 /*C0   {     A     B     C     D     E     F     G */
      0x7B, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
 /*C8   H     I                                     */
      0x48, 0x49, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
 /*D0   }     J     K     L     M     N     O     P */
      0x7D, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50,
 /*D8   Q     R                                     */
      0x51, 0x52, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
 /*E0   \           S     T     U     V     W     X */
      0x5C, 0x3F, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
 /*E8   Y     Z                                     */
      0x59, 0x5A, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
 /*F0   0     1     2     3     4     5     6     7 */
      0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
 /*F8   8     9                                     */
      0x38, 0x39, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0xFF
};
#endif


/***********************************************************************
* readsource - Read source mode, line number and text from the temp file.
***********************************************************************/

int
readsource (FILE *fd, int *inmode, int *linenum, int *filenum, char *inbuf)
{
#ifdef WIN32
   int i;
   char temp[32];
#endif
#if defined(DEBUGP1RDR) || defined(DEBUGP2RDR)
   int flag = FALSE;
#endif

#ifdef WIN32
   /*
   ** Stupid WinBLOWS can't handle inter mixed binary/ascii reads....
   ** Even though we fdopen in binary mode.
   */

   for (i = 0; i < 8; i++)
   {
      if ((temp[i] = fgetc(fd)) == EOF)
         return (-1);
   }
   temp[8] = 0;
   sscanf (temp, "%x", inmode);
   for (i = 0; i < 8; i++)
   {
      if ((temp[i] = fgetc(fd)) == EOF)
         return (-1);
   }
   temp[8] = 0;
   sscanf (temp, "%d", linenum);
#else
   if (fread(inmode, 1, 4, fd) != 4)
      return (-1);
   if (fread(linenum, 1, 4, fd) != 4)
      return (-1);
#endif

   if (fgets(inbuf, MAXLINE, fd) == NULL)
      return (-1);

   *filenum = (*inmode & NESTMASK) >> NESTSHFT;

#ifdef DEBUGP1RDR
   if (inpass == 1) flag = TRUE;
#endif
#ifdef DEBUGP2RDR
   if (inpass == 2) flag = TRUE;
#endif
#if defined(DEBUGP1RDR) || defined(DEBUGP2RDR)
   if (flag)
   {
      printf ("linenum = %d, inmode = %06x, filenum = %d\n",
	      *linenum, *inmode, *filenum);
      printf ("inbuf = %s", inbuf);
   }
#endif
   return (0);
}

/***********************************************************************
* writesource - Write source to the temp file.
***********************************************************************/

void
writesource (FILE *fd, int mode, int num, char *buf, char *loc)
{
#ifdef WIN32
   char temp[16];

   /*
   ** Stupid WinBLOWS can't handle inter mixed binary/ascii reads....
   ** Even though we fdopen in binary mode.
   */

   sprintf (temp, "%08x", mode);
   fwrite (temp, 1, 8, fd);
   sprintf (temp, "%08d", num);
   fwrite (temp, 1, 8, fd);
#else
   fwrite (&mode, 1, 4, fd);
   fwrite (&num, 1, 4, fd);
#endif

   fputs (buf, fd);
#ifdef DEBUGP0OUT
   printf ("%s(%06X) %04d:%s", loc, mode, num, buf);
#endif
}

#ifdef WIN32
/***********************************************************************
* mkstemp - Brain dead support for WinBlows.
***********************************************************************/

int mkstemp (char *template)
{
   int fd;
   char tmpnam[1024];

   strcpy (tmpnam, mktemp(template));
   fd = open (tmpnam, O_CREAT | O_TRUNC | O_RDWR, 0666);
   strcpy (template, tmpnam);
   return (fd);
}
#endif

/***********************************************************************
* opencopy - Open a COPY
***********************************************************************/

FILE *
opencopy (char *bp, FILE *tmpfd)
{
   FILE *infd;
   char *cp;
   int tmpdes;
   char tname[1024];

   while (isspace (*bp)) bp++;
   cp = bp;
   if (*cp == '\'') cp++;
   while (!isspace (*bp)) bp++;
   *bp = '\0';
   if (*(bp-1) == '\'') *(bp-1) = '\0';
   strcpy (tname, cp);
   if ((infd = fopen (tname, "r")) == NULL)
   {
      if (incldir[0])
	 sprintf (tname, "%s/%s", incldir, cp);
      if ((infd = fopen (tname, "r")) == NULL)
      {
	 if (isupper(*cp))
	 {
	    char *ccp;
	    for (ccp = cp; *ccp; ccp++)
	       if (isupper(*ccp)) *ccp = tolower (*ccp);
	    if (incldir[0])
	       sprintf (tname, "%s/%s", incldir, cp);
	    else
	       strcpy (tname, cp);
	    infd = fopen (tname, "r");
	 }
      }
      if (infd == NULL)
      {
	 sprintf (tname, "asm990: Can't open input file for COPY: %s", cp);
	 perror (tname);
	 exit (ABORT);
      }
   }
   files[filenum].fd = tmpfd;
   files[filenum].line = linenum;

#ifdef WIN32
   sprintf (tname, "%s", TEMPSPEC);
#else
   sprintf (tname, "/tmp/%s", TEMPSPEC);
#endif
   if ((tmpdes = mkstemp (tname)) < 0)
   {
      perror ("asm990: Can't mkstemp for COPY");
      exit (ABORT);
   }
   strcpy (files[filenum].tmpnam, tname);

   filenum++;
   fileseq++;

   if (filenum >= MAXASMFILES)
   {
      fprintf (stderr, "Too many COPY files\n");
      exit (ABORT);
   }
   linenum = 0;
   if ((tmpfd = fdopen (tmpdes, "w+")) == NULL)
   {
      perror ("asm990: Can't open temporary file for COPY");
      exit (ABORT);
   }
   detab (infd, tmpfd, "copy");
   fclose (infd);
   if (fseek (tmpfd, 0, SEEK_SET) < 0)
   {
      perror ("asm990: Can't rewind temp file");
      exit (ABORT);
   }

   /*
   ** Mark tmp file for deletion
   */

   unlink (tname);

   return (tmpfd);
}

/***********************************************************************
* closecopy - Close a COPY
***********************************************************************/

FILE *
closecopy (FILE *tmpfd)
{
   if (filenum)
   {
      fclose (tmpfd);
      filenum--;
      unlink (files[filenum].tmpnam);
      linenum = files[filenum].line;
      return (files[filenum].fd);
   }
   return (NULL);
}

/***********************************************************************
* Parse_Error - Print parsing error
***********************************************************************/

void
Parse_Error (int cause, int state, int defer)
{
   char errorstring[256];
   char errtmp[256];

   if (defer) return;
   strcpy (errorstring, "Unknown error");
   if (cause == PARSE_ERROR)
   {
      switch (state)
      {
      /* Include generated parser errors */
#include "asm990.err"
      default: ;
      }
   }
   else if (cause == SCAN_ERROR)
   {
      switch (state)
      {
      case 8:
	 strcpy (errorstring, "Non-terminated STRING");
	 break;
      default: ;
      }
   }
   else if (cause == INTERP_ERROR)
   {
      switch (state)
      {
      case ZERO_DIVIDE:
	 strcpy (errorstring, "Divide by zero");
	 break;
      case ILL_EXTERNAL:
	 strcpy (errorstring, "Illegal use of external variable");
	 break;
      case INTERNAL_ERR:
	 strcpy (errorstring, "Internal Error");
	 break;
      default: ;
      }
   }
   sprintf (errtmp, "%s in expression", errorstring);
   logerror (errtmp);

} /* Parse_Error */

/***********************************************************************
* tokscan - Scan a token.
***********************************************************************/

char *
tokscan (char *buf, char **token, int *tokentype, int *tokenvalue, char *term)
{
   int index;
   toktyp tt;
   tokval val;
   static char t[80];
   char c;

#ifdef DEBUGTOK
   printf ("tokscan: Entered: buf = %s", buf);
#endif
   while (*buf && isspace(*buf))
   {
      /*
      ** We stop scanning on a newline or when we're passed the operands.
      */

      if (*buf == '\n' || (buf - pscanbuf > rightmargin))
      {
	 t[0] = '\0';
	 *token = t;
	 *term = '\n';
	 *tokentype = EOS;
         return (buf);
      }
      buf++;
   }

   if (*buf == MEMSYM)
   {
      t[0] = '\0';
      *token = t;
      *tokentype = MEMREF;
      *term = *buf;
      buf++;
      return (buf);
   }

   index = 0;
   tt = Scanner (buf, &index, &val, t, &c, 1); 
#ifdef DEBUGTOK
   printf ("  token = %s, tokentype = %d, term = %02x, val = %d\n",
	    t, tt, c, val);
   printf (" index = %d, *(buf+index) = %02X\n",  index, *(buf+index));
#endif
   if (tt == DECNUM)
      c = *(buf+index);
   if (c == ',') index++;
   *token = t;
   *tokentype = tt;
   *tokenvalue = val;
   *term = c;
   return (buf+index);
}

/***********************************************************************
* symlookup - Lookup a symbol in the symbol table.
***********************************************************************/

SymNode *
symlookup (char *sym, int add, int xref)
{
   SymNode *ret = NULL;
   int done = FALSE;
   int mid;
   int last = 0;
   int lo;
   int up;
   int r;

#ifdef DEBUGSYM
   printf ("symlookup: Entered: sym = %s\n", sym);
#endif

   /*
   ** Empty symbol table.
   */

   if (txmiramode) sym[MAXSHORTSYMLEN] = 0;
   if (symbolcount == 0)
   {
      if (!add) return (NULL);

#ifdef DEBUGSYM
      printf ("add symbol at top\n");
#endif
      if ((symbols[0] = (SymNode *)malloc (sizeof (SymNode))) == NULL)
      {
         fprintf (stderr, "asm990: Unable to allocate memory\n");
	 exit (ABORT);
      }
      memset (symbols[0], '\0', sizeof (SymNode));
      strcpy (symbols[0]->symbol, sym);
      symbols[0]->value = pc;
      symbols[0]->line = linenum;
#ifdef DEBUGXREF
      if (genxref && linenum)
	 printf ("symlookup-1: sym = %s defline = %d, inpass = %d\n",
		 sym, linenum, inpass);
#endif
      if (indseg)
	 symbols[0]->flags = RELOCATABLE | DSEGSYM;
      else if (!absolute)
	 symbols[0]->flags = RELOCATABLE;
      if (!txmiramode && ((strlen (sym) > MAXSHORTSYMLEN) || (sym[0] == '_')))
	 symbols[0]->flags |= LONGSYM;
      symbolcount++;
      return (symbols[0]);
   }

   /*
   ** Locate using binary search
   */

   lo = 0;
   up = symbolcount;
   last = -1;
   
   while (!done)
   {
      mid = (up - lo) / 2 + lo;
#ifdef DEBUGSYM
      printf (" mid = %d, last = %d\n", mid, last);
#endif
      if (last == mid) break;
      r = strcmp (symbols[mid]->symbol, sym);
      if (r == 0)
      {
	 SymNode *symp = symbols[mid];

	 if (add && !(symp->flags & PREDEFINE))
	    return (NULL); /* must be a duplicate */

	 if (genxref && xref && (inpass != 1))
	 {
	    XrefNode *new;
	    XrefNode *curr, *last;
	    int found = FALSE;

	    curr = symp->xref_head;
	    last = NULL;
	    while (curr)
	    {
	       if (curr->line == linenum)
	       {
	          found = TRUE;
		  break;
	       }
	       if (curr->line > linenum)
	          break;
	       last = curr;
	       curr = curr->next;
	    }
	    if (!found)
	    {
	       if ((new = (XrefNode *)malloc (sizeof (XrefNode))) == NULL)
	       {
		  fprintf (stderr, "asm990: Unable to allocate memory\n");
		  exit (ABORT);
	       }
#ifdef DEBUGXREF
	       if (linenum)
		  printf ("symlookup-2: sym = %s defline = %d, inpass = %d\n",
			  sym, linenum, inpass);
#endif
	       new->next = NULL;
	       new->line = linenum;
	       if (symp->xref_head == NULL)
	       {
		  symp->xref_head = new;
		  symp->xref_tail = new;
	       }
	       else if (curr)
	       {
		  if (last)
		  {
		     new->next = last->next;
		     last->next = new;
		  }
		  else
		  {
		     new->next = symp->xref_head;
		     symp->xref_head = new;
		  }
	       }
	       else
	       {
		  (symp->xref_tail)->next = new;
		  symp->xref_tail = new;
	       }
	    }
	 }
#ifdef DEBUGSYM
	 printf (" sym found: val = %04X, flags = %X\n",
	 	 symp->value, symp->flags);
#endif
         return (symbols[mid]);
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

   /*
   ** Not found, check to add
   */

   if (add)
   {
      SymNode *new;

#ifdef DEBUGSYM
      printf ("add new symbol\n");
#endif
      if (symbolcount+1 > MAXSYMBOLS)
      {
         fprintf (stderr, "asm990: Symbol table exceeded\n");
	 exit (ABORT);
      }

      if ((new = (SymNode *)malloc (sizeof (SymNode))) == NULL)
      {
         fprintf (stderr, "asm990: Unable to allocate memory\n");
	 exit (ABORT);
      }

      memset (new, '\0', sizeof (SymNode));
      strcpy (new->symbol, sym);
#ifdef DEBUGXREF
      if (genxref && linenum)
	 printf ("symlookup-3: sym = %s defline = %d, inpass = %d\n",
		 sym, linenum, inpass);
#endif
      new->value = pc;
      new->line = linenum;
      if (indseg)
	 new->flags = RELOCATABLE | DSEGSYM;
      else if (!absolute)
	 new->flags = RELOCATABLE;
      if (!txmiramode && ((strlen (sym) > MAXSHORTSYMLEN) || (sym[0] == '_')))
	 new->flags |= LONGSYM;

      /*
      ** Insert pointer in sort order.
      */

      for (lo = 0; lo < symbolcount; lo++)
      {
         if (strcmp (symbols[lo]->symbol, sym) > 0)
	 {
	    for (up = symbolcount + 1; up > lo; up--)
	    {
	       symbols[up] = symbols[up-1];
	    }
	    symbols[lo] = new;
	    symbolcount++;
	    return (symbols[lo]);
	 }
      }
      symbols[symbolcount] = new;
      ret = symbols[symbolcount];
      symbolcount++;
   }
   return (ret);
}

/***********************************************************************
* exprscan - Scan an expression and return its value.
***********************************************************************/

char *
exprscan (char *bp, int *val, char *tterm, int *relo, int entsize, int defer,
	  int pcinc)
{
   int index = 0;
   int lrelo = 0;
   tokval lval = 0;
   char errtmp[MAXLINE];

#ifdef DEBUGEXPR
   printf ("exprscan: entered: bp = %s\n", bp);
#endif
   while (*bp && isspace(*bp)) bp++;

#ifdef DEBUGEXPR
   printf ("   bp = %s\n", bp);
#endif
   segvaltype = 0;
   csegval = 0;
   if (*bp == LITERALSYM)
   {
      if (!txmiramode)
      {
	 SymNode *s;
	 char *cp;
	 char temp[32];

	 cp = bp + 1;
	 while (*cp && !isspace(*cp) && *cp != ',') cp++;
	 index = cp - bp;
	 strncpy (temp, bp, index);
	 temp[index] = '\0';

#if defined(DEBUGLITERAL) || defined(DEBUGEXTNDX)
	 printf ("exprscan: Literal: pass = %d, temp = %s\n", inpass, temp);
#endif

	 if ((s = symlookup (temp, FALSE, TRUE)) == NULL)
	 {
	    if ((s = symlookup (temp, TRUE, TRUE)) == NULL)
	    {
	       sprintf (errtmp, "Unable to add Literal: %s", temp);
	       logerror (errtmp);
	       lval = 0;
	    }
	    s->value = codelength;
	    if (temp[1] == 'F' || temp[1] == 'L')
	       codelength += 4;
	    else if (temp[1] == 'D' || temp[1] == 'Q')
	       codelength += 8;
	    else
	       codelength += 2;
	 }
	 lval = s->value;
	 if (s->flags & RELOCATABLE) lrelo++;
      }
   }
   else if (strlen(bp) && *bp != ',')
   {
      lval = Parser (bp, &lrelo, &memexpr, &index, entsize, defer, pcinc);
   }
#ifdef DEBUGEXPR
   printf (" index = %d, *(bp+index) = %02X\n",  index, *(bp+index));
#endif
   *tterm = *(bp+index);
   if (*tterm == ',') index++;
   *val = lval;
   *relo = lrelo;
   return (bp+index);
}

/***********************************************************************
* cvtfloat - Convert floating number to long representation.
***********************************************************************/

unsigned long
cvtfloat (char *bp)
{
   unsigned long tv;
   int index;
   toktyp tt;
   tokval val;
   static char ct[80];
   char c;

   index = 0;
   tt = Scanner (bp, &index, &val, ct, &c, 12); 
#ifdef DEBUG_FLOAT
   printf ("  token = %s, tokentype = %d, term = %02x, val = %d\n",
	    ct, tt, c, val);
   printf (" index = %d, *(bp+index) = %02X\n",  index, *(bp+index));
#endif
   ct[8] = '\0';
   sscanf (ct, "%lx", &tv);
   return (tv);
}

/***********************************************************************
* cvtdouble - Convert double floating number to int representation.
***********************************************************************/

t_uint64
cvtdouble (char *bp)
{
   t_uint64 tv;
   int index;
   toktyp tt;
   tokval val;
   static char ct[80];
   char c;

   index = 0;
   tt = Scanner (bp, &index, &val, ct, &c, 12); 
#ifdef DEBUG_FLOAT
   printf ("  token = %s, tokentype = %d, term = %02x, val = %d\n",
	    ct, tt, c, val);
   printf (" index = %d, *(bp+index) = %02X\n",  index, *(bp+index));
#endif
#ifdef WIN32
   sscanf (ct, "%I64x", &tv);
#else
   sscanf (ct, "%llx", &tv);
#endif
   return (tv);
}

/***********************************************************************
* logerror - Log an error for printing.
***********************************************************************/

void
logerror (char *errmsg)
{
   int i;
#ifdef DEBUGERROR
   printf ("logerror: inpass = %d, linenum = %d, errcount = %d\n",
	   inpass, linenum, errcount);
   printf ("   p1errcnt = %d, errnum = %d, errorfile = %d\n",
	   p1errcnt, errnum, (filenum ? fileseq : 0));
   printf ("   errmsg = %s\n", errmsg);
#endif

   if (inpass != 2)
      for (i = 0; i < p1errcnt; i++)
      {
	 if ((p1error[i].errorline == linenum) && 
	     (p1error[p1errcnt].errorfile == (filenum ? fileseq : 0)) &&
	     !strcmp(p1error[i].errortext, errmsg))
	 {
#ifdef DEBUGERROR
	    printf ("   DUPLICATE\n");
#endif
	    return;
	 }
      }

   errcount++;
   if (inpass == 2)
   {
      strcpy (errline[errnum++], errmsg);
   }
   else
   {
#ifdef DEBUGERROR
      printf ("   NEW, queue\n");
#endif
      p1error[p1errcnt].errortext = (char *)malloc (120);
      p1error[p1errcnt].errorline = linenum;
      p1error[p1errcnt].errorfile = (filenum ? fileseq : 0);
      strcpy (p1error[p1errcnt].errortext, errmsg);
      p1errcnt++;
   }
}

/***********************************************************************
* checkexpr - Check expression.
***********************************************************************/

char *
checkexpr (char *bp, int *type)
{
   char *cp;
   int val, relo;
   char term;
   char tmpexpr[256];

   *type = 0;
   cp = bp;
   while (*cp && !isspace(*cp) && *cp != ',') cp++;
   val = cp - bp;
   strncpy (tmpexpr, bp, val);
   tmpexpr[val] = '\0';

#ifdef DEBUGCHECKEXPR
   printf ("checkexpr: expr = %s\n", tmpexpr);
#endif

   if (tmpexpr[--val] == ')')
   {
      tmpexpr[val--] = '\0';
      while (val && tmpexpr[val] != '(') val--;
      tmpexpr[val] = '\0';
      if (val)
      {
#ifdef DEBUGCHECKEXPR
	 printf ("   Indexed Memory expression\n");
#endif
	 *type = MEMEXPR;
	 return cp;
      }
   }

   if (*bp == MEMSYM)
   {
#ifdef DEBUGCHECKEXPR
      printf ("   Memory expression\n");
#endif
      *type = MEMEXPR;
      return cp;
   }
   else if (*bp == LITERALSYM)
   {
#ifdef DEBUGCHECKEXPR
      printf ("   Literal expression\n");
#endif
      *type = LITEXPR;
      return cp;
   }

   exprscan (tmpexpr, &val, &term, &relo, 2, TRUE, 0);
   if (relo || memexpr || val > 15)
   {
#ifdef DEBUGCHECKEXPR
      printf ("   Memory expression\n");
#endif
      *type = MEMEXPR;
      return cp;
   }
#ifdef DEBUGCHECKEXPR
   printf ("   Register expression\n");
#endif
   *type = REGEXPR;
   return cp;
}
