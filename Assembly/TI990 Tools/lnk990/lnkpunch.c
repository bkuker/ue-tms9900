/************************************************************************
*
* lnkpunch - Punchs out memory.
*
* Changes:
*   06/05/03   DGP   Original.
*   06/23/03   DGP   Put date/time and creator on EOF record.
*   06/25/03   DGP   Added punchsymbols for partial link.
*   04/08/04   DGP   Added "Cassette" mode.
*   04/09/04   DGP   Added object checksum.
*   05/25/04   DGP   Added long ref/def support.
*   04/27/05   DGP   Put IDT in record sequence column 72.
*   12/30/05   DGP   Changed SymNode to use flags.
*   06/11/09   DGP   Added CSEG support.
*   11/03/11   DGP   Changed the way uninitalized memory is used/skipped.
*   08/20/13   DGP   Many fixes to CSEG/DSEG processing.
*                    Fixed concatenated modules processing.
*   02/10/15   DGP   Remove (DEPRECATE) Partial Link support.
*   10/20/15   DGP   Added a.out format.
*   11/20/16   DGP   Added punch symbol table (-g) support.
*
************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include "lnkdef.h"

extern FILE *lstfd;
extern int listmode;
extern int pc;
extern int errcount;
extern int modcount;
extern int absentry;
extern int cassette;
extern int aoutformat;
extern int relentry;
extern int symbolcount;
extern int dsegorg;
extern int dseglen;
extern int csegorg;
extern int cseglen;
extern int pgmlen;
extern int dsegatend;

extern char idtbuf[IDTSIZE+2];
extern SymNode *symbols[MAXSYMBOLS];
extern CSEGNode cmnnodes[MAXCSEGS];
extern struct tm *timeblk;
extern Module modules[MAXMODULES];

extern uint8 memory[MEMSIZE];
extern Memory memctl[MEMSIZE];
extern uint8 dsegmem[MEMSIZE];
extern Memory dsegctl[MEMSIZE];
extern uint8 csegmem[MEMSIZE];
extern Memory csegctl[MEMSIZE];

static int objcnt = 0;
static int objrecnum = 0;
static char objbuf[80];
static char objrec[82];

#define	A_FMAGIC	0407		/* normal */

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
      for (i = 0; i < objcnt; i++) cksum += objrec[i];
      cksum += CKSUM_TAG;
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

   if (objcnt+len >= (cassette ? CHARSPERCASREC : CHARSPERREC))
   {
      punchfinish (outfd);
   }
   strncpy (&objrec[objcnt], objbuf, len);
   objcnt += len;
   objbuf[0] = '\0';
}

/***********************************************************************
* puncheof - Punch EOF mark.
***********************************************************************/

static void
puncheof (FILE *outfd)
{
   char temp[80];

   punchfinish (outfd);
   objrec[0] = EOFSYM;
   if (cassette)
   {
      strcpy (&objrec[1], "\n");
   }
   else
   {
      sprintf (temp, "%-8.8s  %02d/%02d/%02d  %02d:%02d:%02d    LNK990 %s",
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
* punchsegment - Punch segment memory.
***********************************************************************/

static int
punchsegment (FILE *outfd, int origin, int endaddress, int segtype)
{
   Memory *mptr;
   int indseg;
   int skip;
   int retpc;
   int i, j;

#ifdef DEBUGPUNCH
   printf ("punchsegment: origin = %04X, endaddress = %04X, segtype = %s\n",
           origin, endaddress, segtype == DSEG ? "DSEG" : "CSEG");
#endif

   if (segtype == DSEG)
      indseg = TRUE;
   else
      indseg = FALSE;
   retpc = origin;
   skip = FALSE;
   for (i = origin; i < endaddress; )
   {
      mptr = indseg ? &dsegctl[i] : &csegctl[i];
      if (mptr->tag)
      {
	 if (skip)
	 {
#ifdef DEBUGPUNCH
	    printf ("   skip end: new pc = >%04X\n", i);
#endif
	    if (mptr->tag == RELORG_TAG)
	    {
	       j = i;
	       i = indseg ? GETDSEGMEM(i) : GETCSEGMEM(i);
	       if (i == j) i = (i + 2) & 0xFFFE;
	    }
	    sprintf (objbuf, OBJFORMAT, RELORG_TAG, i);
	    punchrecord (outfd);
	    skip = FALSE;
	 }
         mptr = indseg ? &dsegctl[i] : &csegctl[i];
#ifdef DEBUGPUNCH
	 printf ("   addr = %04X, tag = %c, data = %04X\n", i, mptr->tag,
	          indseg ? GETDSEGMEM(i) : GETCSEGMEM(i));
#endif
	 sprintf (objbuf, OBJFORMAT, mptr->tag,
	          indseg ? GETDSEGMEM(i) : GETCSEGMEM(i));
	 if (mptr->tag == EXTNDX_TAG)
	 {
	    sprintf (&objbuf[5], "%04X", mptr->value);
	    i = (i + 2) & 0xFFFE;
	 }
	 else if (mptr->tag == RELORG_TAG)
	 {
	    int j = i;
	    i = indseg ? GETDSEGMEM(i) : GETCSEGMEM(i);
#ifdef DEBUGPUNCH
	    printf ("   RELORG: pc = >%04X, new pc = >%04X\n", j, i);
#endif
	    if (i == j) i = (i + 2) & 0xFFFE;
	 }
	 else
	 {
	    i = (i + 2) & 0xFFFE;
	 }
	 punchrecord (outfd);
	 retpc = i;
      }
      else
      {
#ifdef DEBUGPUNCH
	 if (!skip)
	    printf ("   skip start: pc = >%04X\n", i);
#endif
	 i = (i + 2) & 0xFFFE;
	 skip = TRUE;
      }
   }

#ifdef DEBUGPUNCH
   printf ("   skip = %s, retpc = >%04X\n", skip ? "TRUE" : "FALSE", retpc);
#endif
   return (retpc);
}

/***********************************************************************
* punchword - Punch a.out word.
***********************************************************************/

static void
punchword (int j, FILE *outfd)
{
   fputc ((j >> 8) & 0xFF, outfd);
   fputc (j & 0xFF, outfd);
}

/***********************************************************************
* punchaout - Punch a.out format.
***********************************************************************/

static void
punchaout (FILE *outfd)
{
   int i, j;

   punchword (A_FMAGIC, outfd);
   punchword (pgmlen, outfd);
   punchword (0, outfd);
   punchword (dseglen+cseglen, outfd);
   punchword (0, outfd);
   punchword ((relentry >= 0) ? relentry : 0, outfd);
   punchword (0, outfd);
   punchword (1, outfd);

   for (i = 0; i < pc;)
   {
      if (memctl[i].tag)
      {
	 j = GETMEM(i);
	 if (memctl[i].tag == RELORG_TAG)
	 {
	    for (; i < j; i += 2)
	    {
	       punchword (0, outfd);
	    }
	 }
	 else
	 {
	    punchword (j, outfd);
	    i += 2;
	 }
      }
      else 
      {
         i += 2;
      }
   }

#if 0
   if (dseglen)
   {
      for (; i < dsegorg; i += 2)
	 punchword (0, outfd);
      for (; i < dsegorg+dseglen; i += 2)
      {
	 if (dsegctl[i].tag)
	 {
	    j = GETDSEGMEM(i);
	    if (dsegctl[i].tag == RELORG_TAG)
	    {
	       for (; i < j; i += 2)
	       {
		  punchword (0, outfd);
	       }
	    }
	    else
	    {
	       punchword (j, outfd);
	       i += 2;
	    }
	 }
	 else 
	 {
	    punchword (0, outfd);
	    i += 2;
	 }
      }
   }

   if (cseglen)
   {
      for (; i < csegorg; i += 2)
	 punchword (0, outfd);
      for (; i < csegorg+cseglen; i += 2)
      {
	 if (csegctl[i].tag)
	 {
	    j = GETCSEGMEM(i);
	    if (csegctl[i].tag == RELORG_TAG)
	    {
	       for (; i < j; i += 2)
	       {
		  punchword (0, outfd);
	       }
	    }
	    else
	    {
	       punchword (j, outfd);
	       i += 2;
	    }
	 }
	 else 
	 {
	    punchword (0, outfd);
	    i += 2;
	 }
      }
   }
#endif
}

/***********************************************************************
* lnkpunch - Punch out memory.
***********************************************************************/

int
lnkpunch (FILE *outfd, int symtout)
{
   int i, j;
   int skip;
   int emit;
   char lasttag;

#ifdef DEBUGPUNCH
   printf ("lnkpunch: pc = %04X, dsegorg = %04X, dseglen = %04X\n",
	    pc, dsegorg, dseglen);
   printf ("   csegorg = %04X, cseglen = %04X\n",
	    csegorg, cseglen);
#endif

   if (aoutformat)
   {
      punchaout (outfd);
      return (0);
   }

   memset (objrec, ' ', sizeof(objrec));
   if (dsegatend)
      sprintf (objbuf, IDTFORMAT, IDT_TAG, pgmlen+dseglen+cseglen, idtbuf);
   else
      sprintf (objbuf, IDTFORMAT, IDT_TAG, pgmlen, idtbuf);
   punchrecord (outfd);

   if (relentry >= 0)
   {
      sprintf (objbuf, OBJFORMAT, RELENTRY_TAG, relentry);
      punchrecord (outfd);
   }

   sprintf (objbuf, OBJFORMAT, RELORG_TAG, 0);
   punchrecord (outfd);

   skip = FALSE;
   lasttag = RELORG_TAG;
   for (i = 0; i < pc;)
   {
      emit = TRUE;
#ifdef DEBUGPUNCH
         printf ("  addr = %04X, tag = %c, data = %04X\n",
		  i, memctl[i].tag, GETMEM(i));
#endif
      if (memctl[i].tag)
      {
	 if (skip)
	 {
#ifdef DEBUGPUNCH
	    printf ("   skip end: new pc = >%04X\n", i);
#endif
	    if (memctl[i].tag == RELORG_TAG)
	    {
	       j = i;
	       i = GETMEM(i);
	       if (i == j) i = (i + 2) & 0xFFFE;
	    }
	    sprintf (objbuf, OBJFORMAT, RELORG_TAG, i);
	    punchrecord (outfd);
	    skip = FALSE;
	    lasttag = RELORG_TAG;
	 }

         sprintf (objbuf, OBJFORMAT, memctl[i].tag, GETMEM(i));
	 if (memctl[i].tag == EXTNDX_TAG)
	 {
	    sprintf (&objbuf[5], "%04X", memctl[i].value);
	    i = (i + 2) & 0xFFFE;
	 }
	 else if (memctl[i].tag == RELORG_TAG)
	 {
	    j = i;
	    i = GETMEM(i);
#ifdef DEBUGPUNCH
            printf ("   RELORG: pc = >%04X, new pc = >%04X\n", j, i);
#endif
	    if (i == j)
	    {
	       i = (i + 2) & 0xFFFE;
	       emit = FALSE;
	    }
	    if (lasttag == memctl[i].tag)
	       emit = FALSE;
	 }
	 else
	 {
	    i = (i + 2) & 0xFFFE;
	 }
	 if (emit)
	 {
	    punchrecord (outfd);
	 }
      }
      else
      {
#ifdef DEBUGPUNCH
	 if (!skip)
	    printf ("   skip start: pc = >%04X\n", i);
#endif
	 i = (i + 2) & 0xFFFE;
	 skip = TRUE;
      }
      lasttag = memctl[i].tag;
   }

   if (dsegatend)
   {
      int lpc = pc;

#ifdef DEBUGPUNCH
      printf ("DSEG: lpc = %04X, dsegorg = %04X, dseglen = %04X\n",
	       lpc, dsegorg, dseglen);
#endif
      if (lpc != dsegorg)
      {
	 lpc = dsegorg;
	 sprintf (objbuf, OBJFORMAT, RELORG_TAG, lpc);
	 punchrecord (outfd);
      }
      lpc = punchsegment (outfd, lpc, lpc+dseglen, DSEG);
#ifdef DEBUGPUNCH
      printf ("CSEG: lpc = %04X, csegorg = %04X, cseglen = %04X\n",
	       lpc, csegorg, cseglen);
#endif
      if (lpc != csegorg)
      {
	 lpc = csegorg;
	 sprintf (objbuf, OBJFORMAT, RELORG_TAG, lpc);
	 punchrecord (outfd);
      }
      lpc = punchsegment (outfd, lpc, lpc+cseglen, CSEG);
   }

   if (symtout)
   {
      for (j = 0; j < modcount; j++)
      {
	 Module *module;
	 SymNode *sym;

	 module = &modules[j];
	 if (module->dsegmodule) continue;

	 sprintf (objbuf, IDTFORMAT, PGMIDT_TAG,
		  module->origin, module->name);
	 punchrecord (outfd);

	 for (i = 0; i < module->symcount; i++)
	 {
	    sym = module->symbols[i];

	    if (sym->flags & LONGSYM)
	    {
	       sprintf (objbuf, LDEFFORMAT, (sym->flags & RELOCATABLE) ?
				       LRELSYMBOL_TAG : LABSSYMBOL_TAG,
				   sym->value & 0xFFFF, sym->symbol);
	    }
	    else
	    {
	       sprintf (objbuf, DEFFORMAT, (sym->flags & RELOCATABLE) ?
				       RELSYMBOL_TAG : ABSSYMBOL_TAG,
				   sym->value & 0xFFFF, sym->symbol);
	    }
	    punchrecord (outfd);
	 }
      }
   }

   puncheof (outfd);

   return (0);
}
