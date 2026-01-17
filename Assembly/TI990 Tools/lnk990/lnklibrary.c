/************************************************************************
*
* lnklibrary - Loads library objects from asm990 for linking.
*
* Changes:
*   01/22/07   DGP   Original, Hacked from lnkloader.
*   01/24/07   DGP   Added Common REF/DEF and DSEG processing.
*   12/02/10   DGP   Corrected EXTNDX tag support.
*   12/07/10   DGP   Added ABSDEF flag.
*   12/14/10   DGP   Correct module number in undefined reference.
*   01/26/11   DGP   Reread library if we have more to do...
*   02/08/11   DGP   Added PGMIDT and LOAD tag support.
*   02/10/11   DGP   Corrected EXTNDX in dseg and DSEGORG with offset.
*   11/03/11   DGP   Fixed pc usage to always even between modules.
*                    Changed the way uninitalized memory is used.
*   08/21/13   DGP   Revamp library support.
*                    Default D/C SEGS at the end like SDSLNK.
*   02/03/14   DGP   Eat SYMBOL_TAGs.
*   01/11/16   DGP   Added Long COMMON symbol support.
*   09/05/21   DGP   Fixed extern infiles.
*
************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>

#include "lnkdef.h"

extern int pc;
extern int infilecnt;

extern FILENode infiles[MAXFILES];

/************************************************************************
* checkdef - Check if DEF is needed.
************************************************************************/

static int
checkdef (char *item, int relo, int flags)
{
   SymNode *s;
   char *bp;

   bp = item;
   for (; *bp; bp++) if (isspace(*bp)) *bp = '\0';

#ifdef DEBUGLIBRARY
   printf ("checkdef: item = %s", item);
#endif
#if defined(DEBUGLIBRARYLOAD) || defined(DEBUGLIBRARY)
   printf (", %c%s = %s",
	 relo ? 'R' : 'A',
	 (flags & LONGSYM) ? "LDEF" : "DEF",
	 item);
#endif

   if ((s = reflookup (item, NULL, FALSE)) != NULL)
   {
      if (s->flags & UNDEF && !(s->flags & SREF))
      {
#if defined(DEBUGLIBRARYLOAD) || defined(DEBUGLIBRARY)
	 printf ("\n      flags = %04X, value = %04X",
	       s->flags, s->value);
#endif
#ifdef DEBUGLIBRARY
	 putchar('\n');
#endif
         return (1);
      }
   }

#ifdef DEBUGLIBRARY
   putchar('\n');
#endif
   return (0);
}

/************************************************************************
* lnkcheck - Check if file is referenced.
************************************************************************/

static int
lnkcheck (FILE *fd)
{
   int status;
   int binarymode = FALSE;
   int reclen;
   int relo;
   int done;
   int i;
   int wordlen = WORDTAGLEN-1;
   unsigned char inbuf[82];

#ifdef DEBUGLIBRARY
   printf ("lnkcheck: \n");
#endif

   status = 0;
   i = fgetc (fd);
   ungetc (i, fd);
   if (i == BINIDT_TAG)
   {
      binarymode = TRUE;
   }

   done = FALSE;
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

#ifdef DEBUGLIBRARYLOAD
      printf ("reclen = %d\n", reclen);
#endif
      if (*op == EOFSYM)
      {
	 done = TRUE;
	 break;
      }

      for (i = 0; i < reclen; i++)
      {
	 char otag;
	 char item[80];
	 uint16 wdata;

	 otag = *op++;

	 wdata = getnumfield (op, binarymode);

#ifdef DEBUGLIBRARYLOAD
	 printf ("   otag = %c", isprint(otag) ? otag : '0');
	 printf (", data = %04X", wdata & 0xFFFF);
#endif

	 relo = FALSE;
	 switch (otag)
	 {
	 case BINIDT_TAG: /* Binary IDT */
#ifdef DEBUGLIBRARYLOAD
	    printf (", Binary mode");
#endif
	    wordlen = BINWORDTAGLEN-1;
	 case IDT_TAG:
	 case PGMIDT_TAG:
	    op += wordlen;
	    strncpy (item, (char *)op, IDTSIZE);
	    item[IDTSIZE] = '\0';
#ifdef DEBUGLIBRARY
	    printf ("   IDT = %s\n", item);
#endif
#ifdef DEBUGLIBRARYLOAD
	    printf (", IDT = %s", item);
#endif
	    op += IDTSIZE;
	    break;

	 case LRELEXTRN_TAG:
	 case LABSEXTRN_TAG:
	 case LABSSYMBOL_TAG:
	 case LRELSYMBOL_TAG:
	    op += (MAXSYMLEN - MAXSHORTSYMLEN);

	 case RELEXTRN_TAG:
	 case ABSEXTRN_TAG:
	 case RELSREF_TAG:
	 case LOAD_TAG:
	 case ABSSYMBOL_TAG:
	 case RELSYMBOL_TAG:
	    op += wordlen;
	    op += MAXSHORTSYMLEN;
	    break;

	 case RELGLOBAL_TAG:
	    relo = TRUE;
	 case ABSGLOBAL_TAG:
	    op += wordlen;
	    strncpy (item, (char *)op, MAXSHORTSYMLEN);
	    item[MAXSHORTSYMLEN] = '\0';
	    status += checkdef (item, relo, 0);
	    op += MAXSHORTSYMLEN;
	    break;

	 case COMMON_TAG:
	 case CMNEXTRN_TAG:
	    op += wordlen;
            op += MAXSHORTSYMLEN;
            op += wordlen;
            break;
	    
         case CMNORG_TAG:
         case CMNDATA_TAG:
         case EXTNDX_TAG:
            op += wordlen;
            op += wordlen;
            break;

	 case CMNGLOBAL_TAG:
            op += wordlen;
	    strncpy (item, (char *)op, MAXSHORTSYMLEN);
	    item[MAXSHORTSYMLEN] = '\0';
	    status += checkdef (item, TRUE, 0);
            op += MAXSHORTSYMLEN;
            op += wordlen;
            break;

	 case LRELGLOBAL_TAG:
	    relo = TRUE;
	 case LABSGLOBAL_TAG:
	    op += wordlen;
	    strncpy (item, (char *)op, MAXSYMLEN);
	    item[MAXSYMLEN] = '\0';
	    status += checkdef (item, relo, LONGSYM);
	    op += MAXSYMLEN;
	    break;

	 case LCMNGLOBAL_TAG:
	    relo = TRUE;
	    op += wordlen;
	    strncpy (item, (char *)op, MAXSYMLEN);
	    item[MAXSYMLEN] = '\0';
	    status += checkdef (item, relo, LONGSYM);
	    op += MAXSYMLEN;
	    op += wordlen;
	    break;

	 case LCMNEXTRN_TAG:
	 case LCMNSREF_TAG:
	    op += wordlen;
	    op += MAXSYMLEN;
	    op += wordlen;
	    break;

	 case EOR_TAG:
	    i = 81;
	    break;

	 case NOCKSUM_TAG:
	 case CKSUM_TAG:
	    op += wordlen;
	    break; 

	 default:
	    op += wordlen;
	 }
#ifdef DEBUGLIBRARYLOAD
	 printf ("\n");
#endif
      }
   }

#ifdef DEBUGLIBRARY
   if (status)
      printf ("   LOAD module\n");
#endif
   return (status);

}

/************************************************************************
* lnklibrary - Object library loader for lnk990.
************************************************************************/

int
lnklibrary (FILE *fd, int loadpt, int pass, char *file, char *fullfile)
{
   off_t modstart;
   int more;
   int pcstart;
   int curraddr;

   curraddr = loadpt;

#ifdef DEBUGLIBRARY
   printf ("lnklibrary: loadpt = %04X, pass = %d, file = %s\n",
           loadpt, pass, fullfile);
#endif

   /*
   ** Process the entire library
   */

   do
   {
      fseek (fd, 0, SEEK_SET);
      clearerr(fd);
      more = FALSE;

      while (!feof(fd))
      {
	 if ((modstart = ftell(fd)) < 0)
	 {
	    char errbuf[256];
	    sprintf (errbuf, "lnk990: Can't ftell input file %s",
		     fullfile);
	    perror (errbuf);
	    exit (ABORT);
	 }
	 pcstart = curraddr;

	 /*
	 ** Check if current module has DEFs that are needed for an
	 ** unresolved reference.
	 */

	 if (lnkcheck (fd))
	 {
	    /*
	    ** Yes, load module.
	    */

	    /* Save file info for pass 2 */

	    if (infilecnt >= MAXFILES)
	    {
	       fprintf (stderr, "lnk990: Too many files: %d\n", MAXFILES);
	       exit (ABORT);
	    }
	    strcpy (infiles[infilecnt].name, fullfile);
	    infiles[infilecnt].library = TRUE;
	    infiles[infilecnt].offset = modstart;
	    infilecnt++;

	    /* Go load it */

	    more = TRUE;
	    pcstart = (pcstart + 1) & 0xFFFE;
	    if (fseek (fd, modstart, SEEK_SET) < 0)
	    {
	       char errbuf[256];
	       sprintf (errbuf, "lnk990: Can't fseek input file %s",
			fullfile);
	       perror (errbuf);
	       exit (ABORT);
	    }
	    curraddr = lnkloader (fd, pcstart, pass, file, TRUE);
	 }
      }
   } while (more);
   pc = curraddr;
   return (0);
}
