/************************************************************************
*
* lnkloader - Loads objects from asm990 for linking.
*
* Changes:
*   06/05/03   DGP   Original.
*   06/25/03   DGP   Changed to print listing like TXLINK.
*                    Changed to link undefined external chains.
*   06/27/03   DGP   Added compressed binary support.
*   07/10/03   DGP   Changed to use first global definition and ignore 
*                    absolute externals with a zero value.
*   04/12/04   DGP   Changed to allow non-80 byte records.
*   05/25/04   DGP   Added long ref/def support.
*   05/05/04   DGP   Fixed binary mode reads. Added getnumfield().
*   12/30/05   DGP   Changed SymNode to use flags.
*   01/18/07   DGP   Functionalize REF/DEF processing.
*   01/24/07   DGP   Added Common REF/DEF and DSEG processing.
*   03/17/09   DGP   Allow for back relorg (odd pc bss).
*   06/11/09   DGP   Added CSEG support.
*   12/02/10   DGP   Corrected EXTNDX tag support.
*   12/07/10   DGP   Added ABSDEF flag.
*   12/14/10   DGP   Correct module number in undefined reference.
*   01/07/11   DGP   Allow for EXTNDX to be filled in later. We link the
*                    references when undefined.
*   01/25/11   DGP   Correct dseglen usage.
*   02/08/11   DGP   Added PGMIDT and LOAD tag support.
*   02/10/11   DGP   Corrected EXTNDX in dseg and DSEGORG with offset.
*   03/04/11   DGP   Fixed processing of concatenated object modules.
*   11/03/11   DGP   Don't let LOAD references bump the extndx.
*                    Fixed "back" RORG handling.
*                    Fixed module length return (don't return curraddr).
*   08/20/13   DGP   Many fixes to CSEG/DSEG processing.
*                    Fixed concatenated modules processing.
*   08/21/13   DGP   Revamp library support.
*                    Default D/C SEGS at the end like SDSLNK.
*   01/16/14   DGP   Fixed backward CSEG/DSEG orgs.
*   02/03/14   DGP   Eat SYMBOL_TAGs.
*   10/09/15   DGP   Added Long symbol common tags.
*   11/20/16   DGP   Added punch symbol table (-g) support.
*
************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <memory.h>

#include "lnkdef.h"

extern int absentry;
extern int relentry;
extern int modcount;
extern int modnumber;
extern int dsegorg;
extern int dseglen;
extern int csegorg;
extern int cseglen;
extern int pgmlen;
extern int cmncount;
extern int symbolcount;
extern int curmod;
extern int extndx;
extern int dsegatend;

extern SymNode *symbols[MAXSYMBOLS];
extern char idtbuf[IDTSIZE+2];
extern Module modules[MAXMODULES];
extern CSEGNode cmnnodes[MAXCSEGS];

extern uint8 memory[MEMSIZE];
extern Memory memctl[MEMSIZE];
extern uint8 dsegmem[MEMSIZE];
extern Memory dsegctl[MEMSIZE];
extern uint8 csegmem[MEMSIZE];
extern Memory csegctl[MEMSIZE];

/************************************************************************
* getnumfield - Gets the next numeric field.
************************************************************************/

uint16
getnumfield (unsigned char *op, int binarymode)
{
   uint16 wdata;
   char item[80];

   if (binarymode)
   {
      wdata = (*op << 8) | *(op+1);
   }
   else
   {
      int wd;

      strncpy (item, (char *)op, 4);
      item[4] = '\0';
      sscanf (item, "%4X", &wd);
      wdata = wd & 0xFFFF;
   }
   return (wdata);
}

/************************************************************************
* processdef - Process DEF.
************************************************************************/

void
processdef (char *item, Module *module, int pass, int relo,
	    int flags, uint16 wdata, int libload)
{
   SymNode *s, *r;
   char *bp;

   if (pass == 1)
   {
      bp = item;
      for (; *bp; bp++) if (isspace(*bp)) *bp = '\0';
#ifdef DEBUGLOADER
      printf (", %c%s = %s, libload = %s",
	    relo ? 'R' : 'A',
	    (flags & LONGSYM) ? "LDEF" :
	    (flags & CSEG) ? "CMNDEF" : 
	    (flags & DSEG) ? "DSEGDEF" : "DEF",
	    item, libload ? "TRUE" : "FALSE");
#endif
      if ((s = symlookup (item, module, FALSE)) == NULL)
      {
	 if ((s = symlookup (item, module, TRUE)) == NULL)
	 {
	    fprintf (stderr, "lnk990: Internal error\n");
	    exit (ABORT);
	 }
	 s->flags = relo ? RELOCATABLE : ABSDEF;
	 s->flags |= GLOBAL | flags;
	 s->value = wdata;
	 s->modnum = modnumber;
#ifdef DEBUGLOADER
	 printf ("\n      flags = %04X, value = %04X",
	       s->flags, s->value);
#endif
	 if ((s = reflookup (item, module, FALSE)) != NULL)
	 {
	    s->flags = relo ? RELOCATABLE : ABSDEF;
	    s->flags |= GLOBAL | flags;
	 }
      }
      else
      {
	 if (libload)
	 {
	    s->modnum = modnumber;
	    if (s->flags & UNDEF)
	    {
	       int refaddr;

#ifdef DEBUGLOADER
	       printf ("\n      fill: flags = %04X, value = %04X",
		     s->flags, s->value);
#endif
	       refaddr = s->value;
	       while (refaddr)
	       {
		  int k;
		  k = GETMEM (refaddr);
		  PUTMEM (refaddr, wdata);
		  memctl[refaddr].tag = relo ? RELDATA_TAG : ABSDATA_TAG;
		  refaddr = k;
#ifdef DEBUGLOADER
		  printf ("\n          refaddr = %04X", refaddr);
#endif
	       }
	       s->flags = relo ? RELOCATABLE : ABSDEF;
	       s->flags |= GLOBAL | flags;
	       s->value = wdata;
	    }
	    else
	       s->flags |= MULDEF;
	 }
	 else
	    s->flags |= MULDEF;
      }
   }

}

/************************************************************************
* processref - Process REF.
************************************************************************/

static void
processref (char *item, Module *module, int pass, int relo,
	    int flags, uint16 wdata, int libload)
{
   SymNode *s;
   char *bp;
   char otag;

   bp = item;
   for (; *bp; bp++) if (isspace(*bp)) *bp = '\0';
#ifdef DEBUGLOADER
   printf (", %c%s = %s",
	    relo ? 'R' : 'A',
	    (flags & LONGSYM) ? "LREF" :
	    (flags & SREF) ? "SREF" :
	    (flags & LOAD) ? "LOAD" : "REF",
	    item);
#endif


   if (pass == 1)
   {
      if ((s = symlookup (item, module, FALSE)) == NULL)
      {
	 if ((s = reflookup (item, module, FALSE)) == NULL)
	 {
	    if ((s = reflookup (item, module, TRUE)) == NULL)
	    {
	       fprintf (stderr, "lnk990: Internal error\n");
	       exit (ABORT);
	    }
	    s->flags = relo ? RELOCATABLE : 0;
	    s->flags |= EXTERNAL | UNDEF | flags;
	 }
	 else
	 {
	    if ((flags & LOAD) && (s->flags & SREF))
	    {
	       s->flags &= ~(LOAD|SREF);
	    }
	 }
      }
#ifdef DEBUGLOADER
      printf (", flags = %04X, value = %04X",
	    s->flags, s->value);
#endif
   }
   else if (pass == 2)
   {
      if ((s = symlookup (item, module, FALSE)) == NULL)
      {
	 if ((s = symlookup (item, module, TRUE)) == NULL)
	 {
	    fprintf (stderr, "lnk990: Internal error\n");
	    exit (ABORT);
	 }
#ifdef DEBUGLOADER
         printf (", module = %d", curmod);
#endif
	 s->flags = relo ? RELOCATABLE : 0;
	 s->flags |= EXTERNAL | UNDEF | flags;
	 s->value = wdata;
	 if (!(flags & LOAD))
	    s->extndx = extndx++;
	 s->modnum = curmod;
      }
      else
      {
	 int refaddr;

         if ((s->extndx < 0) && !(flags & LOAD))
	    s->extndx = extndx++;
	 if (relo && !(s->flags & ABSDEF))
	    s->flags |= RELOCATABLE;
	 refaddr = wdata;
	 if (flags & LOAD)
	 {
	    if (s->flags & SREF)
	       s->flags &= ~(LOAD|SREF);
	    return;
	 }
#ifdef DEBUGLOADER
	 printf ("\n      flags = %04X, value = %04X",
	       s->flags, s->value);
	 printf ("\n          refaddr = %04X", refaddr);
#endif
	 /*
	 ** If defined, fill in back chain
	 */

	 if (!(s->flags & UNDEF))
	 {
	    while (refaddr)
	    {
	       int k;

	       if (dsegatend && (flags & DSEG))
	       {
		  k = GETDSEGMEM (refaddr);
		  PUTDSEGMEM (refaddr, s->value);
		  dsegctl[refaddr].tag =
		       (s->flags & RELOCATABLE) ? RELDATA_TAG : ABSDATA_TAG;
	       }
	       else if (dsegatend && (flags & CSEG))
	       {
		  k = GETCSEGMEM (refaddr);
		  PUTCSEGMEM (refaddr, s->value);
		  csegctl[refaddr].tag =
		       (s->flags & RELOCATABLE) ? RELDATA_TAG : ABSDATA_TAG;
	       }
	       else
	       {
		  k = GETMEM (refaddr);
		  PUTMEM (refaddr, s->value);
		  memctl[refaddr].tag =
		       (s->flags & RELOCATABLE) ? RELDATA_TAG : ABSDATA_TAG;
	       }
	       refaddr = k;
#ifdef DEBUGLOADER
	       printf ("\n          refaddr = %04X", refaddr);
#endif
	    }
	 }

	 /*
	 ** If undefined, connect back chain
	 */

	 else
	 {
	    while (refaddr)
	    {
	       int k;

	       if (dsegatend && (flags & DSEG))
	       {
		  k = GETDSEGMEM (refaddr);
		  if (k == 0)
		  {
		     dsegctl[refaddr].tag = RELDATA_TAG;
		     PUTDSEGMEM(refaddr, s->value);
		     s->value = wdata;
		  }
	       }
	       else if (dsegatend && (flags & CSEG))
	       {
		  k = GETCSEGMEM (refaddr);
		  if (k == 0)
		  {
		     csegctl[refaddr].tag = RELDATA_TAG;
		     PUTCSEGMEM(refaddr, s->value);
		     s->value = wdata;
		  }
	       }
	       else
	       {
		  k = GETMEM (refaddr);
		  if (k == 0)
		  {
		     memctl[refaddr].tag = RELDATA_TAG;
		     PUTMEM(refaddr, s->value);
		     s->value = wdata;
		  }
	       }
	       refaddr = k;
	    }
	 }
      }
   }
}

/************************************************************************
* fillinextndx - Process EXTNDX back fills.
************************************************************************/

void
fillinextndx (void)
{
   int i;

#ifdef DEBUGLOADER
   printf ("\n\nfillinextndx: entered\n");
#endif

   for (i = 0; i < symbolcount; i++)
   {
      SymNode *s;

      s = symbols[i];
      if (s->extlist)
      {
	 EXTndxlist *ref;
	 int curraddr;

	 ref = s->extlist;
	 while (ref)
	 {
	    curraddr = ref->location;
	    if (s->flags & UNDEF)
	    {
	       memctl[curraddr].tag = ABSDATA_TAG;
	       PUTMEM (curraddr, 0);
#ifdef DEBUGLOADER
	       printf (
		     "   sym = %s-U, flags = %04X, addr = %04X, value = %04X\n",
		     s->symbol, s->flags, curraddr, GETMEM(curraddr));
#endif
	    }
	    else
	    {
	       memctl[curraddr].tag =
		      (s->flags & RELOCATABLE) ? RELDATA_TAG : ABSDATA_TAG;
	       PUTMEM (curraddr, memctl[curraddr].value + s->value);
#ifdef DEBUGLOADER
	       printf (
	 "   sym = %s, flags = %04X, addr = %04X, value = %04X (%04X + %04X)\n",
		     s->symbol, s->flags, curraddr, GETMEM(curraddr),
		     memctl[curraddr].value, s->value);
#endif
	    }
	    ref = ref->next;
	 }
      }
   }
}

/************************************************************************
* processextndx - Process EXTNDX tag.
************************************************************************/

static void
processextndx (int curraddr, int seg)
{
   int i;
   uint16 wdata;

   if (dsegatend && (seg == DSEG))
      wdata = GETDSEGMEM (curraddr);
   else if (dsegatend && (seg == CSEG))
      wdata = GETCSEGMEM (curraddr);
   else
      wdata = GETMEM (curraddr);

   for (i = 0; i < symbolcount; i++)
   {
      SymNode *s;

      s = symbols[i];
      if (wdata == s->extndx)
      {
	 if (s->flags & UNDEF)
	 {
	    EXTndxlist *new;

	    if ((new = malloc (sizeof (EXTndxlist))) == NULL)
	    {
	       fprintf (stderr, "lnk990: Unable to allocate memory\n");
	       exit (ABORT);
	    }
#ifdef DEBUGLOADER
	    printf (
		 "\n       sym = %s-U, flags = %04X, value = %04X, addr = %04X",
		  s->symbol, s->flags, GETMEM(curraddr), curraddr);
#endif
	    new->next = s->extlist;
	    new->location = curraddr;
	    s->extlist = new;
	 }
	 else
	 {
	    if (dsegatend && (seg == DSEG))
	    {
	       dsegctl[curraddr].tag =
		      (s->flags & RELOCATABLE) ? RELDATA_TAG : ABSDATA_TAG;
	       PUTDSEGMEM (curraddr, dsegctl[curraddr].value + s->value);
#ifdef DEBUGLOADER
	       printf (
	        "\n       sym-D = %s, flags = %04X, value = %04X (%04X + %04X)",
		     s->symbol, s->flags, GETDSEGMEM(curraddr),
		     dsegctl[curraddr].value, s->value);
#endif
	    }
	    else if (dsegatend && (seg == CSEG))
	    {
	       csegctl[curraddr].tag =
		      (s->flags & RELOCATABLE) ? RELDATA_TAG : ABSDATA_TAG;
	       PUTCSEGMEM (curraddr, csegctl[curraddr].value + s->value);
#ifdef DEBUGLOADER
	       printf (
		"\n       sym-C = %s, flags = %04X, value = %04X (%04X + %04X)",
		     s->symbol, s->flags, GETCSEGMEM(curraddr),
		     csegctl[curraddr].value, s->value);
#endif
	    }
	    else
	    {
	       memctl[curraddr].tag =
		      (s->flags & RELOCATABLE) ? RELDATA_TAG : ABSDATA_TAG;
	       PUTMEM (curraddr, memctl[curraddr].value + s->value);
#ifdef DEBUGLOADER
	       printf (
		"\n       sym-F = %s, flags = %04X, value = %04X (%04X + %04X)",
		  s->symbol, s->flags, GETMEM(curraddr),
		  memctl[curraddr].value, s->value);
#endif
            }
	 }
	 break;
      }
   }

}

/************************************************************************
* resetextndx - Reset EXTNDX tag.
************************************************************************/

static void
resetextndx (void)
{
   int i;

   extndx = 0;
   for (i = 0; i < symbolcount; i++)
      symbols[i]->extndx = -1;
}

/************************************************************************
* lnkloader - Object loader for lnk990.
************************************************************************/

int
lnkloader (FILE *fd, int loadpt, int pass, char *file, int libload)
{
   Module *newmodule;
   SymNode *sym;
   int binarymode = FALSE;
   int reclen;
   int curraddr;
   int modendaddr;
   int relo;
   int done;
   int i;
   int dsegpc;
   int csegpc;
   int indseg;
   int incseg;
   int wordlen = WORDTAGLEN-1;
   int cdseglen;
   int ccseglen;
   int lastaddr;
   uint16 pseglen = 0;
   unsigned char inbuf[82];
   char module[MAXSYMLEN+2];

#ifdef DEBUGLOADER
   printf ("lnkloader: loadpt = %04X, pass = %d, libload = %s, file = %s\n",
	 loadpt, pass, libload ? "TRUE" : "FALSE", file);
#endif

   indseg = FALSE;
   incseg = FALSE;
   curraddr = loadpt;
   modendaddr = loadpt;
   if (dsegatend)
   {
      dsegpc = dsegorg;
      cdseglen = 0;
      csegpc = csegorg;
      ccseglen = 0;
   }
   else
   {
      dseglen = dsegpc = 0;
      cseglen = csegpc = 0;
   }

   resetextndx();

   i = fgetc (fd);
   if (i == BINIDT_TAG)
      binarymode = TRUE;
   ungetc (i, fd);

   done = FALSE;
   pseglen = 0;
   while (!done)
   {
      unsigned char *op = inbuf;

      if (binarymode)
      {
	 reclen = fread (inbuf, 1, 81, fd);
	 if (feof(fd))
	 {
	    done = TRUE;
	    break;
	 }
      }
      else
      {
	 if (fgets ((char *)inbuf, 82, fd) == NULL)
	 {
	    done = TRUE;
	    break;
	 }
	 reclen = strlen ((char *)inbuf);
      }

#ifdef DEBUGLOADER
      printf ("reclen = %d\n", reclen);
#endif
      if (*op == EOFSYM)
      {
	 if (pass == 1)
	 {
	    if (reclen > 60)
	    {
	       strncpy (newmodule->date, (char *)&inbuf[TIMEOFFSET], 8);
	       newmodule->date[9] = '\0';
	       strncpy (newmodule->time, (char *)&inbuf[DATEOFFSET], 8);
	       newmodule->time[9] = '\0';
	       strncpy (newmodule->creator,
		        (char *)&inbuf[CREATOROFFSET], 8);
	       newmodule->creator[9] = '\0';
	    }
	    else
	    {
	       strncpy (newmodule->date, "        ", 8);
	       strncpy (newmodule->time, "        ", 8);
	       strncpy (newmodule->creator, "        ", 8);
	    }
	 }
	 if (dsegatend)
	 {
	    if (cdseglen)
	    {
	       dsegorg += cdseglen;
	       dseglen += cdseglen;
	    }
	    cdseglen = 0;
	    dsegpc = dsegorg;
	    if (ccseglen)
	    {
	       csegorg += ccseglen;
	       cseglen += ccseglen;
	    }
	    ccseglen = 0;
	    csegpc = csegorg;
	 }
	 else
	 {
	    if (dseglen)
	       curraddr += dseglen;
	    dseglen = 0;
	    dsegpc = 0;
	    if (cseglen)
	       curraddr += cseglen;
	    cseglen = 0;
	    csegpc = 0;
	 }
	 incseg = FALSE;
	 indseg = FALSE;
	 binarymode = FALSE;
	 resetextndx();
	 loadpt += pseglen;
	 loadpt = (loadpt + 1) & 0xFFFE;
	 if (libload)
	    break;
	 continue;
      }

      for (i = 0; i < reclen; i++)
      {
	 char *bp;
	 char otag, ltag;
	 char item[80];
	 uint16 wdata;
	 uint16 cmnndx;

	 relo = FALSE;
	 otag = *op++;

	 wdata = getnumfield (op, binarymode);

#ifdef DEBUGLOADER
	 printf ("   otag = %c, loadpt = %04X, curraddr = %04X",
		  isprint(otag) ? otag : '0', loadpt, curraddr);
	 printf (", dsegpc = %04X, csegpc = %04X, data = %04X",
	         dsegpc, csegpc, wdata & 0xFFFF);
#endif

         switch (otag)
	 {
	 case BINIDT_TAG: /* Binary IDT */
	    binarymode = TRUE;
#ifdef DEBUGLOADER
	    printf (", Binary mode");
#endif
	    wdata = (*op << 8) | *(op+1);
	    wordlen = BINWORDTAGLEN-1;
	 case IDT_TAG:
	    op += wordlen;
	    strncpy (item, (char *)op, IDTSIZE);
	    item[IDTSIZE] = '\0';
#ifdef DEBUGLOADER
	    printf (", IDT = %s", item);
#endif
	    bp = item;
	    for (; *bp; bp++) if (isspace(*bp)) *bp = '\0';
	    strcpy (module, item);
	    pseglen = ((wdata + 1) & 0xFFFE);
	    pgmlen += pseglen;
	    modendaddr += ((wdata + 1) & 0xFFFE);
	    if (pass == 1)
	    {
	       modnumber++;
	       if (modnumber >= MAXMODULES)
	       {
		  fprintf (stderr, "lnk990: Too many Modules: %d\n",
			   MAXMODULES);
		  exit (ABORT);
	       }
	       newmodule = &modules[modcount];
	       strcpy (newmodule->objfile, file);
	       strcpy (newmodule->name, item);
	       newmodule->length = wdata;
	       newmodule->origin = loadpt;
	       newmodule->number = modnumber;
	       newmodule->creator[0] = '\0';
	       newmodule->date[0] = '\0';
	       newmodule->time[0] = '\0';
	       curmod = modcount;
	       modcount++;
	    }
	    else
	    {
	       newmodule = &modules[curmod];
	       curmod++;
	    }
#ifdef DEBUGLOADER
            printf (", module = %s, curmod = %d", module, curmod);
#endif
	    if (idtbuf[0] == '\0')
	       strcpy (idtbuf, item);
	    if (!dsegatend && (pass == 1))
	    {
	       dsegorg = loadpt + wdata;
	       csegorg = loadpt + wdata;
	    }
	    op += IDTSIZE;
	    break;

	 case PGMIDT_TAG:
	    op += wordlen;
	    strncpy (item, (char *)op, IDTSIZE);
	    item[IDTSIZE] = '\0';
#ifdef DEBUGLOADER
	    printf (", IDT = %s", item);
#endif
	    op += IDTSIZE;
	    break;

         case RELORG_TAG:
	    wdata += loadpt;
         case ABSORG_TAG:
	    if (!indseg && !incseg && pseglen)
	    {
	       if (wdata >= curraddr)
	       {
#ifdef DEBUGLOADER
		  printf (", ORGTAG = %04X, memtag = %c", wdata,
			  memctl[curraddr].tag ? memctl[curraddr].tag : '0');
#endif
		  lastaddr = curraddr;
		  if (!memctl[curraddr].tag || (memctl[curraddr].tag == otag))
		  {
		     PUTMEM (curraddr, wdata);
		     memctl[curraddr].tag = otag;
		  }
	       }
	       else
	       {
#ifdef DEBUGLOADER
		  printf (", BACK ORGTAG = %04X, ltag = %c:%c, lastaddr = %04X",
			  wdata, ltag, memctl[lastaddr].tag, lastaddr);
#endif
		  if ((ltag == otag) && (memctl[lastaddr].tag == otag))
		  {
		     PUTMEM (lastaddr, wdata);
		  }
	       }
	    }
	    curraddr = wdata & 0xFFFF;
	    indseg = FALSE;
	    incseg = FALSE;
	    op += wordlen;
	    break;

	 case RELDATA_TAG:
	    wdata += loadpt;
	    relo = TRUE;
	 case ABSDATA_TAG:
	    if (indseg)
	    {
	       if (dsegatend)
	       {
		  PUTDSEGMEM (dsegpc, wdata);
		  dsegctl[dsegpc].tag = otag;
	       }
	       else
	       {
		  PUTMEM (dsegpc, wdata);
		  memctl[dsegpc].tag = otag;
	       }
#ifdef DEBUGLOADER
	       printf (", DSEGDATA");
#endif
	       dsegpc += 2;
	    }
	    else if (incseg)
	    {
	       if (dsegatend)
	       {
		  PUTCSEGMEM (csegpc, wdata);
		  csegctl[csegpc].tag = otag;
	       }
	       else
	       {
		  PUTMEM (csegpc, wdata);
		  memctl[csegpc].tag = otag;
	       }
#ifdef DEBUGLOADER
	       printf (", CSEGDATA");
#endif
	       csegpc += 2;
	    }
	    else
	    {
	       PUTMEM (curraddr, wdata);
	       memctl[curraddr].flags = relo ? RELOCATABLE : 0;
	       memctl[curraddr].tag = otag;
	       curraddr += 2;
	    }
	    op += wordlen;
	    break;

	 case RELENTRY_TAG:
	    relentry = (wdata + loadpt) & 0xFFFF;
	    op += wordlen;
	    break;

	 case ABSENTRY_TAG:
	    absentry = wdata & 0xFFFF;
	    op += wordlen;
	    break;

	 case RELEXTRN_TAG:
	    wdata += loadpt;
	    relo = TRUE;
	 case ABSEXTRN_TAG:
	    op += wordlen;
	    strncpy (item, (char *)op, MAXSHORTSYMLEN);
	    item[MAXSHORTSYMLEN] = '\0';
	    processref (item, newmodule, pass, relo, 0, wdata, libload);
	    op += MAXSHORTSYMLEN;
	    break;

	 case RELGLOBAL_TAG:
	    wdata += loadpt;
	    relo = TRUE;
	 case ABSGLOBAL_TAG:
	    op += wordlen;
	    strncpy (item, (char *)op, MAXSHORTSYMLEN);
	    item[MAXSHORTSYMLEN] = '\0';
	    processdef (item, newmodule, pass, relo, 0, wdata, libload);
	    op += MAXSHORTSYMLEN;
	    break;

	 case RELSREF_TAG:
	    if (wdata)
	    {
	       wdata += loadpt;
	       relo = TRUE;
	    }
	    op += wordlen;
	    strncpy (item, (char *)op, MAXSHORTSYMLEN);
	    item[MAXSHORTSYMLEN] = '\0';
	    processref (item, newmodule, pass, TRUE, SREF, wdata, libload);
	    op += MAXSHORTSYMLEN;
	    break;

	 case COMMON_TAG:
	    op += wordlen;
	    strncpy (item, (char *)op, MAXSHORTSYMLEN);
	    item[MAXSHORTSYMLEN] = '\0';
            op += MAXSHORTSYMLEN;
            cmnndx = getnumfield (op, binarymode);
            op += wordlen;
#ifdef DEBUGLOADER
            printf (", CMN = %s, NDX = %04X", item, cmnndx);
#endif

            if (!strncmp (item, "$DATA", 5))
	    {
	       /* $DATA sections are always index 0 */
	       strcpy (cmnnodes[cmnndx].name, item);
	       cmnnodes[cmnndx].length = wdata;
	       cmnnodes[cmnndx].offset = 0;
	       if (dsegatend)
	       {
		  cdseglen = wdata;
		  cmnnodes[cmnndx].origin = dsegorg;
	       }
	       else
	       {
		  dseglen = wdata;
		  cmnnodes[cmnndx].origin = loadpt + pseglen;
		  pseglen += wdata;
		  modendaddr += wdata;
	       }
#ifdef DEBUGLOADER
	       printf (", DSEG length = %04X, origin = %04X",
			cmnnodes[cmnndx].length, cmnnodes[cmnndx].origin);
	    	    
#endif
	       if (pass == 1 && wdata)
	       {
		  /*
		  ** A dseg gets a dummy module entry.
		  */

		  strcpy (modules[modcount].objfile, "");
		  strcpy (modules[modcount].name, item);
		  modules[modcount].length = cmnnodes[cmnndx].length;
		  modules[modcount].origin = cmnnodes[cmnndx].origin;
		  modules[modcount].number = modnumber;
		  modules[modcount].dsegmodule = TRUE;
		  strncpy (modules[modcount].date, "        ", 8);
		  strncpy (modules[modcount].time, "        ", 8);
		  strncpy (modules[modcount].creator, "        ", 8);
		  modcount++;
	       }
	    }
	    else
	    {
	       int j;
	       int found;

	       cseglen = 0;
	       found = FALSE;
#ifdef DEBUGCOMMON
               printf ("COMMON: item = '%s', cmnndx = %d, pass = %d\n",
		       item, cmnndx, pass);
#endif
	       for (j = 0; j < cmncount; j++)
	       {
		  csegorg = cmnnodes[j+1].origin;
		  cseglen = cmnnodes[j+1].length;
#ifdef DEBUGCOMMON
		  printf (
		     "   j = %d, name = '%s', origin = %04X, length = %04X\n",
		     j, cmnnodes[j+1].name, csegorg, cseglen);
#endif
	          if (!strcmp (cmnnodes[j+1].name, item))
		  {
#ifdef DEBUGCOMMON
		     printf ("   FOUND\n");
#endif
		     found = TRUE;
		     cmnnodes[cmnndx].offset = j+1;
		     cmnndx = j+1;
		     break;
		  }
	       }
	       if (!found)
	       {
#ifdef DEBUGCOMMON
		     printf ("   NOT FOUND\n");
#endif
		  cmncount++;
		  if (cmncount >= MAXCSEGS)
		  {
		     fprintf (stderr, "lnk990: Too many Common segments: %d\n",
			      MAXCSEGS);
		     exit (ABORT);
		  }
	          cmnnodes[cmnndx].offset = cmncount;
		  if (pass == 1)
		  {
		     strcpy (cmnnodes[cmncount].name, item);
		     cmnnodes[cmncount].number = modnumber;
		     cmnnodes[cmncount].length = wdata;
		     if (dsegatend)
			cmnnodes[cmncount].origin = csegorg + cseglen;
		     else
		        cmnnodes[cmncount].origin = loadpt + pseglen;
		  }
		  cmnndx = cmncount;
	       }

	       if (dsegatend)
	       {
		  ccseglen = wdata;
		  csegorg = cmnnodes[cmnnodes[cmnndx].offset].origin;
		  //csegorg = cmnnodes[cmnndx].origin;
	       }
	       else
	       {
		  cseglen = wdata;
		  pseglen += wdata;
		  modendaddr += wdata;
	       }
#ifdef DEBUGLOADER
	       printf (", CSEG length = %04X, origin = %04X",
			cmnnodes[cmnndx].length, cmnnodes[cmnndx].origin);
	    	    
#endif
	    }
            break;
	    
         case EXTNDX_TAG:
            op += wordlen;
            cmnndx = getnumfield (op, binarymode);
            op += wordlen;
#ifdef DEBUGLOADER
            printf (", wdata = %04X, OFFSET = %04X", wdata, cmnndx);
#endif      
	    if (indseg)
	    {
	       if (dsegatend)
	       {
		  PUTDSEGMEM (dsegpc, wdata);
		  dsegctl[dsegpc].tag = otag;
		  dsegctl[dsegpc].value = cmnndx;
	       }
	       else
	       {
		  PUTMEM (dsegpc, wdata);
		  memctl[dsegpc].tag = otag;
		  memctl[dsegpc].value = cmnndx;
	       }
	       if (pass == 2)
		  processextndx (dsegpc, DSEG);
	       dsegpc += 2;
	    }
	    else if (incseg)
	    {
	       if (dsegatend)
	       {
		  PUTCSEGMEM (csegpc, wdata);
		  csegctl[csegpc].tag = otag;
		  csegctl[csegpc].value = cmnndx;
	       }
	       else
	       {
		  PUTMEM (csegpc, wdata);
		  memctl[csegpc].tag = otag;
		  memctl[csegpc].value = cmnndx;
	       }
	       if (pass == 2)
		  processextndx (csegpc, CSEG);
	       csegpc += 2;
	    }
	    else
	    {
	       PUTMEM (curraddr, wdata);
	       memctl[curraddr].tag = otag;
	       memctl[curraddr].value = cmnndx;
	       if (pass == 2)
		  processextndx (curraddr, 0);
	       curraddr += 2;
	    }
            break;

         case DSEGORG_TAG:
            op += wordlen; 
	    dsegorg = cmnnodes[0].origin;

	    if (wdata >= dsegpc)
	    {
#ifdef DEBUGLOADER
	       printf (", DSEGORG = %04X, dsegpc = %04X, dsegorg = %04X",
		       wdata, dsegpc, dsegorg);
#endif      
	       if (dsegatend)
	       {
		  PUTDSEGMEM (dsegpc, dsegorg+wdata);
		  dsegctl[dsegpc].tag = RELORG_TAG;
	       }
	       else
	       {
		  if (dsegpc)
		  {
		     PUTMEM (dsegpc, dsegorg+wdata);
		     memctl[dsegpc].tag = RELORG_TAG;
		  }
		  else
		  {
		     PUTMEM (dsegorg, dsegorg+wdata);
		     memctl[dsegorg].tag = RELORG_TAG;
		  }
	       }
	    }
	    else
	    {
#ifdef DEBUGLOADER
	       printf (", BACK DSEGORG = %04X, dsegpc = %04X, dsegorg = %04X",
		       wdata, dsegpc, dsegorg);
#endif      
	    }
            dsegpc = dsegorg + wdata;
            indseg = TRUE;
	    incseg = FALSE;
            break;
         
         case DSEGDATA_TAG:
            op += wordlen;
	    wdata += cmnnodes[0].origin;
#ifdef DEBUGLOADER
	    printf (", DSEGREF = %04X", wdata);
#endif
	    if (indseg)
	    {
	       if (dsegatend)
	       {
		  PUTDSEGMEM (dsegpc, wdata);
		  if (pass == 2)
		     dsegctl[dsegpc].tag = RELDATA_TAG;
	       }
	       else
	       {
		  PUTMEM (dsegpc, wdata);
		  if (pass == 2)
		     memctl[dsegpc].tag = RELDATA_TAG;
	       }
	       dsegpc += 2;
	    }
	    else
	    {
#ifdef DEBUGLOADER
	       printf (", DSEGORG = %04X", dsegorg);
#endif
	       PUTMEM (curraddr, wdata);
	       if (pass == 2)
		  memctl[curraddr].tag = RELDATA_TAG;
	       curraddr += 2;
	    }
            break;
         
         case CMNORG_TAG:
            op += wordlen; 
            cmnndx = getnumfield (op, binarymode);
            op += wordlen;
	    csegorg = cmnnodes[cmnnodes[cmnndx].offset].origin;
	    if (wdata >= csegpc)
	    {
#ifdef DEBUGLOADER
	       printf (", CSEGORG = %04X, NDX = %04X", wdata, cmnndx);
	       printf (", csegpc = %04X, csegorg = %04X", csegpc, csegorg);
#endif
	       if (dsegatend)
	       {
		  PUTCSEGMEM (csegpc, csegorg+wdata);
		  csegctl[csegpc].tag = RELORG_TAG;
	       }
	       else
	       {
		  if (csegpc)
		  {
		     PUTMEM (csegpc, csegorg+wdata);
		     memctl[csegpc].tag = RELORG_TAG;
		  }
		  else
		  {
		     PUTMEM (csegorg, csegorg+wdata);
		     memctl[csegorg].tag = RELORG_TAG;
		  }
	       }
	    }
	    else
	    {
#ifdef DEBUGLOADER
	       printf (", BACK CSEGORG = %04X, NDX = %04X", wdata, cmnndx);
	       printf (", csegpc = %04X, csegorg = %04X",
	               csegpc, csegorg);
#endif
	    }
            csegpc = csegorg + wdata;
            incseg = TRUE;
	    indseg = FALSE;
            break;
         
         case CMNDATA_TAG:
            op += wordlen;
            cmnndx = getnumfield (op, binarymode);
            op += wordlen;
	    wdata += cmnnodes[cmnnodes[cmnndx].offset].origin;
#ifdef DEBUGLOADER
	    printf (", CSEGREF = %04X, NDX = %04X", wdata, cmnndx);
#endif
	    if (incseg)
	    {
	       if (dsegatend)
	       {
		  PUTCSEGMEM (csegpc, wdata);
		  if (pass == 2)
		     csegctl[csegpc].tag = RELDATA_TAG;
	       }
	       else
	       {
		  PUTMEM (csegpc, wdata);
		  if (pass == 2)
		     memctl[csegpc].tag = RELDATA_TAG;
	       }
	       csegpc += 2;
	    }
	    else
	    {
#ifdef DEBUGLOADER
               printf (", CSEGORG = %04X", csegorg);
#endif
	       PUTMEM (curraddr, wdata);
	       if (pass == 2)
		  memctl[curraddr].tag = RELDATA_TAG;
	       curraddr += 2;
	    }
            break;

	 case CMNEXTRN_TAG:
            op += wordlen;
	    strncpy (item, (char *)op, MAXSHORTSYMLEN);
	    item[MAXSHORTSYMLEN] = '\0';
            op += MAXSHORTSYMLEN;
            cmnndx = getnumfield (op, binarymode);
            op += wordlen;
	    wdata += cmnnodes[cmnnodes[cmnndx].offset].origin;
#ifdef DEBUGLOADER
            printf (", NDX = %04X", cmnndx);
#endif
	    processref (item, newmodule, pass, TRUE, cmnndx ? CSEG : DSEG,
			wdata, libload);
            break;

	 case CMNGLOBAL_TAG:
            op += wordlen;
	    strncpy (item, (char *)op, MAXSHORTSYMLEN);
	    item[MAXSHORTSYMLEN] = '\0';
            op += MAXSHORTSYMLEN;
            cmnndx = getnumfield (op, binarymode);
            op += wordlen;
	    wdata += cmnnodes[cmnnodes[cmnndx].offset].origin;
#ifdef DEBUGLOADER
            printf (", NDX = %04X", cmnndx);
#endif
	    processdef (item, newmodule, pass, TRUE, (cmnndx ? CSEG : DSEG),
	    		wdata, libload);
            break;

	 case LCMNEXTRN_TAG:
            op += wordlen;
	    strncpy (item, (char *)op, MAXSYMLEN);
	    item[MAXSHORTSYMLEN] = '\0';
            op += MAXSHORTSYMLEN;
            cmnndx = getnumfield (op, binarymode);
            op += wordlen;
	    wdata += cmnnodes[cmnnodes[cmnndx].offset].origin;
#ifdef DEBUGLOADER
            printf (", NDX = %04X", cmnndx);
#endif
	    processref (item, newmodule, pass, TRUE,
			LONGSYM | (cmnndx ? CSEG : DSEG), wdata, libload);
            break;

	 case LCMNGLOBAL_TAG:
            op += wordlen;
	    strncpy (item, (char *)op, MAXSYMLEN);
	    item[MAXSYMLEN] = '\0';
            op += MAXSYMLEN;
            cmnndx = getnumfield (op, binarymode);
            op += wordlen;
	    wdata += cmnnodes[cmnnodes[cmnndx].offset].origin;
#ifdef DEBUGLOADER
            printf (", NDX = %04X", cmnndx);
#endif
	    processdef (item, newmodule, pass, TRUE,
			LONGSYM | (cmnndx ? CSEG : DSEG), wdata, libload);
            break;

	 case LRELEXTRN_TAG:
	    wdata += loadpt;
	    relo = TRUE;
	 case LABSEXTRN_TAG:
	    op += wordlen;
	    strncpy (item, (char *)op, MAXSYMLEN);
	    item[MAXSYMLEN] = '\0';
	    processref (item, newmodule, pass, relo, LONGSYM, wdata, libload);
	    op += MAXSYMLEN;
	    break;

	 case LRELGLOBAL_TAG:
	    wdata += loadpt;
	    relo = TRUE;
	 case LABSGLOBAL_TAG:
	    op += wordlen;
	    strncpy (item, (char *)op, MAXSYMLEN);
	    item[MAXSYMLEN] = '\0';
	    processdef (item, newmodule, pass, relo, LONGSYM, wdata, libload);
	    op += MAXSYMLEN;
	    break;

	 case LOAD_TAG:
	    relo = TRUE;
	    op += wordlen;
	    strncpy (item, (char *)op, MAXSHORTSYMLEN);
	    item[MAXSHORTSYMLEN] = '\0';
	    processref (item, newmodule, pass, relo, LOAD, wdata, libload);
	    op += MAXSHORTSYMLEN;
	    break;

	 case RELSYMBOL_TAG:
	    relo = TRUE;
	    wdata += loadpt;
	 case ABSSYMBOL_TAG:
	    op += wordlen;
	    strncpy (item, (char *)op, MAXSHORTSYMLEN);
	    item[MAXSHORTSYMLEN] = '\0';
	    bp = item;
	    for (; *bp; bp++) if (isspace(*bp)) *bp = '\0';
	    op += MAXSHORTSYMLEN;
	    if ((sym = modsymlookup (item, newmodule, FALSE)) == NULL)
	    {
	       if ((sym = modsymlookup (item, newmodule, TRUE)) == NULL)
	       {
		  fprintf (stderr, "lnk990: Internal error\n");
		  exit (ABORT);
	       }
	       sym->flags = relo ? RELOCATABLE : ABSDEF;
	       sym->flags |= SYMTAG;
	       sym->value = wdata;
	       sym->modnum = modnumber;
	    }
	    break;

	 case LRELSYMBOL_TAG:
	    relo = TRUE;
	    wdata += loadpt;
	 case LABSSYMBOL_TAG:
	    op += wordlen;
	    strncpy (item, (char *)op, MAXSYMLEN);
	    item[MAXSYMLEN] = '\0';
	    bp = item;
	    for (; *bp; bp++) if (isspace(*bp)) *bp = '\0';
	    op += MAXSYMLEN;
	    if ((sym = modsymlookup (item, newmodule, FALSE)) == NULL)
	    {
	       if ((sym = modsymlookup (item, newmodule, TRUE)) == NULL)
	       {
		  fprintf (stderr, "lnk990: Internal error\n");
		  exit (ABORT);
	       }
	       sym->flags = relo ? RELOCATABLE : ABSDEF;
	       sym->flags |= SYMTAG | LONGSYM;
	       sym->value = wdata;
	       sym->modnum = modnumber;
	    }
	    break;

	 case EOR_TAG:
	    i = 81;
	    break;

	 case NOCKSUM_TAG:
	 case CKSUM_TAG:
	    op += wordlen;
	    break; 

	 default:
	    fprintf (stderr, "lnk990: UNSUPPORTED TAG, tag = %c, %s\n",
	    	otag, &inbuf[71]);
	    exit (ABORT);
	 }
#ifdef DEBUGLOADER
	 printf ("\n");
#endif
         ltag = otag;
      }
   }

#ifdef DEBUGLOADER
   printf ("   end modendaddr = %04X\n", modendaddr);
#endif
   return (modendaddr);
}
