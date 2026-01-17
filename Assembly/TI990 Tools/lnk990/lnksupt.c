/***********************************************************************
*
* lnksupt.c - Support routines for the TI 990 linker.
*
* Changes:
*   06/05/03   DGP   Original.
*   10/27/04   DGP   Set longsym to FALSE.
*   12/30/05   DGP   Changed SymNode to use flags.
*   03/31/10   DGP   Added module numbering.
*   08/21/13   DGP   Revamp library support.
*                    Default D/C SEGS at the end like SDSLNK.
*   11/22/16   DGP   Added modsymlookup().
*
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "lnkdef.h"

extern int pc;
extern int symbolcount;
extern int refsymbolcount;
extern int absolute;
extern int pgmlength;
extern int modnumber;

extern char inbuf[MAXLINE];
extern SymNode *symbols[MAXSYMBOLS];
extern SymNode *refsymbols[MAXSYMBOLS];


/***********************************************************************
* lookupsym - Lookup a symbol in the symbol table.
***********************************************************************/

static SymNode *
lookupsym (SymNode *symtbl[MAXSYMBOLS], int *symcount, char *sym,
	   Module *module, int add)
{
   SymNode *ret = NULL;
   int done = FALSE;
   int count;
   int mid;
   int last = 0;
   int lo;
   int up;
   int r;

   count = *symcount;
#ifdef DEBUGSYM
   printf ("lookupsym: Entered: count = %d, sym = %s, module = %s\n",
	   count, sym, module ? module->name : "");
#endif

   /*
   ** Empty symbol table.
   */

   if (count == 0)
   {
      if (!add) return (NULL);

#ifdef DEBUGSYM
      printf ("add symbol at top\n");
#endif
      if ((symtbl[0] = (SymNode *)malloc (sizeof (SymNode))) == NULL)
      {
         fprintf (stderr, "lnk990: Unable to allocate memory\n");
	 exit (ABORT);
      }
      memset (symtbl[0], '\0', sizeof (SymNode));
      strcpy (symtbl[0]->symbol, sym);
      strcpy (symtbl[0]->module, module ? module->name : "");
      symtbl[0]->value = pc;
      symtbl[0]->modnum = modnumber;
      if (!absolute)
	 symtbl[0]->flags = RELOCATABLE;
      count++;
      *symcount = count;
      return (symtbl[0]);
   }

   /*
   ** Locate using binary search
   */

   lo = 0;
   up = count;
   last = -1;
   
   while (!done)
   {
      mid = (up - lo) / 2 + lo;
#ifdef DEBUGSYM
      printf (" mid = %d, last = %d\n", mid, last);
#endif
      if (last == mid) break;
      r = strcmp (symtbl[mid]->symbol, sym);
      if (r == 0)
      {
	 if (add) return (NULL); /* must be a duplicate */
         return (symtbl[mid]);
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
      if (count+1 > MAXSYMBOLS)
      {
         fprintf (stderr, "lnk990: Symbol table exceeded\n");
	 exit (ABORT);
      }

      if ((new = (SymNode *)malloc (sizeof (SymNode))) == NULL)
      {
         fprintf (stderr, "lnk990: Unable to allocate memory\n");
	 exit (ABORT);
      }

      memset (new, '\0', sizeof (SymNode));
      strcpy (new->symbol, sym);
      strcpy (new->module, module ? module->name : "");
      new->value = pc;
      new->modnum = modnumber;
      if (!absolute)
	 new->flags = RELOCATABLE;

      /*
      ** Insert pointer in sort order.
      */

      for (lo = 0; lo < count; lo++)
      {
         if (strcmp (symtbl[lo]->symbol, sym) > 0)
	 {
	    for (up = count + 1; up > lo; up--)
	    {
	       symtbl[up] = symtbl[up-1];
	    }
	    symtbl[lo] = new;
	    count++;
	    *symcount = count;
	    return (symtbl[lo]);
	 }
      }
      symtbl[count] = new;
      ret = symtbl[count];
      count++;
      *symcount = count;
   }
   return (ret);
}

/***********************************************************************
* symlookup - Lookup a symbol in the symbol table.
***********************************************************************/

SymNode *
symlookup (char *sym, Module *module, int add)
{
#ifdef DEBUGSYM
   printf ("symlookup: Entered: sym = %s, module = %s\n",
	   sym, module ? module->name : "");
#endif
   return (lookupsym (symbols, &symbolcount, sym, module, add));
}

/***********************************************************************
* reflookup - Lookup a symbol in the ref symbol table.
***********************************************************************/

SymNode *
reflookup (char *sym, Module *module, int add)
{
#ifdef DEBUGSYM
   printf ("reflookup: Entered: sym = %s, module = %s\n",
	   sym, module ? module->name : "");
#endif
   return (lookupsym (refsymbols, &refsymbolcount, sym, module, add));
}

/***********************************************************************
* modsymlookup - Lookup a symbol in the module symbol table.
***********************************************************************/

SymNode *
modsymlookup (char *sym, Module *module, int add)
{
#ifdef DEBUGSYM
   printf ("modsymlookup: Entered: sym = %s, module = %s\n",
	   sym, module ? module->name : "");
#endif
   return (lookupsym (module->symbols, &module->symcount, sym, module, add));
}

