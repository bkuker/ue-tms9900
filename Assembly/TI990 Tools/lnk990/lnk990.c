/************************************************************************
*
* lnk990 - Links objects from asm990
*
* Changes:
*   06/05/03   DGP   Original.
*   06/25/03   DGP   Changed to print listing like TXLINK.
*                    Added printsymbols function.
*                    Added partial link.
*                    Added multiply defined and undefined symbols.
*   07/08/03   DGP   Moved get date/time to always get it.
*   04/08/04   DGP   Added "Cassette" mode.
*   05/25/04   DGP   Added long ref/def support.
*   12/30/05   DGP   Changed SymNode to use flags.
*   01/21/07   DGP   Added scansymbols() for non-listmode.
*   01/23/07   DGP   Added library support.
*   06/20/07   DGP   Adjust number of symbols per line in the map.
*   12/12/07   DGP   Fold IDT to upper case.
*   06/11/09   DGP   Added CSEG support.
*   12/02/10   DGP   Generate UNDEF symbol error if value is non-zero.
*   12/24/10   DGP   Correct page count to eject new page.
*   01/07/11   DGP   Allow for EXTNDX to be filled in later. We link the
*                    references when undefined.
*   11/03/11   DGP   Fixed pc usage to always even between modules.
*                    Changed the way uninitalized memory is used.
*   08/20/13   DGP   Many fixes to CSEG/DSEG processing.
*                    Fixed concatenated modules processing.
*   08/21/13   DGP   Revamp library support.
*                    Default D/C SEGS at the end like SDSLNK.
*   02/10/15   DGP   Remove (DEPRECATE) Partial Link support.
*   10/09/15   DGP   Added Long symbol common tags.
*                    Added printundefs() function.
*   10/20/15   DGP   Added a.out format.
*   11/20/16   DGP   Added punch symbol table (-g) support.
*   12/13/16   DGP   Added program too big detection.
*	
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

#include "lnkdef.h"

FILE *lstfd = NULL;
int listmode = FALSE;
int pc; /* the linker pc */
int absolute = FALSE;
int dsegatend = TRUE;
int symbolcount = 0;
int refsymbolcount = 0;
int errcount = 0;
int pgmlength = 0;
int absentry = -1;
int relentry = -1;
int linecnt = MAXLINE;
int modcount = 0;
int modnumber = 0;
int undefs = FALSE;
int muldefs = FALSE;
int toobig = FALSE;
int cassette = FALSE;
int aoutformat = FALSE;
int symtout = FALSE;
int dseguserorg = -1;
int cseguserorg = -1;
int pseguserorg = -1;
int dsegorg = 0;
int dseglen = 0;
int csegorg = 0;
int cseglen = 0;
int pgmlen = 0;
int cmncount = 0;
int curmod = 0;
int extndx = 0;
int infilecnt = 0;
char errline[120];
char inbuf[MAXLINE];
char idtbuf[IDTSIZE+2];
struct tm *timeblk;

SymNode *symbols[MAXSYMBOLS];
SymNode *refsymbols[MAXSYMBOLS];

Module modules[MAXMODULES];
CSEGNode cmnnodes[MAXCSEGS];
FILENode infiles[MAXFILES];

uint8 memory[MEMSIZE];
Memory memctl[MEMSIZE];
uint8 dsegmem[MEMSIZE];
Memory dsegctl[MEMSIZE];
uint8 csegmem[MEMSIZE];
Memory csegctl[MEMSIZE];

static int pagenum = 0;
static char datebuf[48];

/***********************************************************************
* printheader - Print header on listing.
***********************************************************************/

void
printheader (FILE *lstfd)
{
   if (linecnt >= LINESPAGE)
   {
      linecnt = 0;
      if (pagenum) fputc ('\f', lstfd);
      fprintf (lstfd, H1FORMAT, VERSION, idtbuf, datebuf, ++pagenum);
   }
}

/***********************************************************************
* printsymbols - Print the symbol table.
***********************************************************************/

static void
printsymbols (FILE *lstfd)
{
   SymNode *sym;
   int i, j;
   int longformat;
   int maxsymlen;
   int symperline;
   char type;
   char type1;
   char format[40];

   longformat = FALSE;
   maxsymlen = 6;

   if ((linecnt + 4) >= LINESPAGE)
      printheader(lstfd);

   fprintf (lstfd, "\n                        D E F I N I T I O N S \n\n");
   linecnt += 3;

   for (i = 0; i < symbolcount; i++) {
      if (symbols[i]->flags & LONGSYM) longformat = TRUE;
      if ((j = strlen (symbols[i]->symbol)) > maxsymlen) maxsymlen = j;
   }
   maxsymlen += 1;
   symperline = 80 / (12 + maxsymlen);
   sprintf (format, SYMFORMAT, maxsymlen, maxsymlen);

   j = 0;
   for (i = 0; i < symbolcount; i++)
   {
      sym = symbols[i];
      printheader (lstfd);

      if (sym->flags & RELOCATABLE) type = '\'';
      else type = ' ';

      if (sym->flags & MULDEF)
      {
	 muldefs = TRUE;
	 type1 = 'M';
      }
      else if (sym->flags & UNDEF && sym->value != 0)
      {
	 if (!(sym->flags & SREF))
	 {
	    continue; /* process it in printundefs */
	 }
      }
      else type1 = ' ';

      fprintf (lstfd, format,
	       sym->symbol, sym->value,
	       type, type1, sym->modnum);
      j++;

      if (j == symperline)
      {
	 fprintf (lstfd, "\n");
	 linecnt++;
	 printheader(lstfd);
	 j = 0;
      }
   }
   fprintf (lstfd, "\n");
}

/***********************************************************************
* printundefs - Print the udefined symbols.
***********************************************************************/

static void
printundefs (FILE *lstfd)
{
   SymNode *sym;
   int i, j;
   int hdrout = FALSE;
   int maxsymlen;
   int symperline;
   char format[40];

   maxsymlen = 6;
   for (i = 0; i < symbolcount; i++)
   {
      sym = symbols[i];
      if (sym->flags & LONGSYM)
      {
	 if ((j = strlen (sym->symbol)) > maxsymlen)
	    maxsymlen = j;
      }
   }
   maxsymlen += 1;
   symperline = 80 / (5 + maxsymlen);
   sprintf (format, UNDEFFORMAT, maxsymlen, maxsymlen);

   j = 0;
   for (i = 0; i < symbolcount; i++)
   {
      sym = symbols[i];
      if (sym->flags & UNDEF && (sym->value != 0))
      {
	 if (!(sym->flags & SREF))
	 {
	    if (!hdrout)
	    {
	       printheader(lstfd);
	       fputs (
	   "\n                   U N R E S O L V E D   R E F E R E N C E S\n\n",
		       lstfd);
	       linecnt += 3;
	       hdrout = TRUE;
	    }
	    fprintf (lstfd, format, sym->symbol, sym->modnum);
	    undefs = TRUE;
	    errcount++;
	    if (++j == symperline)
	    {
	       fputc ('\n', lstfd);
	       linecnt++;
	       printheader(lstfd);
	       j = 0;
	    }
	 }
      }
   }
   if (hdrout && j)
   {
      fputc ('\n', lstfd);
      linecnt++;
   }
}

/***********************************************************************
* scansymbol - Scan the symbol table for undefined symbols.
***********************************************************************/

static void
scansymbols (void)
{
   SymNode *sym;
   int i;

   for (i = 0; i < symbolcount; i++)
   {
      sym = symbols[i];
      if (sym->flags & UNDEF && (sym->value != 0))
      {
	 if (!(sym->flags & SREF))
	 {
	    fprintf (stderr, "Undefined symbol: %s\n", sym->symbol);
	    undefs = TRUE;
	    errcount++;
	 }
      }
   }
}

/***********************************************************************
* updatesegs - Scan and update the C/DSEG symbol definitions.
***********************************************************************/

static void
updatesegs (int adjust, int seg)
{
   SymNode *sym;
   int i;

#ifdef DEBUG
   printf ("updatesegs: symbolcount = %d, adjust = %04X\n",
   	    symbolcount, adjust);
#endif

   for (i = 0; i < symbolcount; i++)
   {
      sym = symbols[i];
#ifdef DEBUG
      printf ("   flags = %04X, value = %04X, sym = %s\n",
         sym->flags, sym->value, sym->symbol);
#endif
      if ((sym->flags & seg) && (sym->flags & RELOCATABLE))
      {
	 sym->value += adjust;
#ifdef DEBUG
	 printf ("      update: value = %04X\n", sym->value);
#endif
      }
   }

   if (seg == CSEG)
   {
      for (i = 0; i < cmncount; i++)
      {
         cmnnodes[i+1].origin += adjust;
      }
   }
}

/***********************************************************************
* getnum - Get a number from the command line.
***********************************************************************/

int
getnum (char *bp)
{
   char *cp;
   long i = 0;

   while (*bp && isspace (*bp)) bp++;
   cp = bp;
   if (*bp == '>')
   {
      i = strtol (++bp, &cp, 16);
   }
   else if (*bp == '0')
   {
      int base = 8;
      bp++;
      if (*bp == 'x' || *bp == 'X')
      {
	 bp++;
	 base = 16;
      }
      i = strtol (bp, &cp, base);
   }
   else 
   {
      i = strtol (bp, &cp, 10);
   }
   return ((int)i & 0xFFFF);
}

/***********************************************************************
* Main procedure
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *infd = NULL;
   FILE *outfd = NULL;
   char *libs[MAXFILES];
   char *libdirs[MAXFILES];
   char *outfile = NULL;
   char *lstfile = NULL;
   char *bp;
   int i;
   int listMode = FALSE;
   int libcnt = 0;
   int libdircnt = 0;
   int status = 0;
   time_t curtime;
   char temp[256];
  
#ifdef DEBUG
   printf ("lnk990: Entered:\n");
   printf ("args:\n");
   for (i = 1; i < argc; i++)
      printf ("   arg[%2d] = %s\n", i, argv[i]);
#endif

   /*
   ** Clear files.
   */

   memset (&infiles, 0, sizeof (infiles));
   for (i = 0; i < MAXFILES; i++)
   {
      libs[i] = NULL;
      libdirs[i] = NULL;
   }

   /*
   ** Clear symboltable
   */

   memset (&symbols, 0, sizeof (symbols));
   memset (&refsymbols, 0, sizeof (refsymbols));

   /*
   ** Clear Common table
   */

   memset (&cmnnodes, 0, sizeof(cmnnodes));

   /*
   ** Scan off the the args.
   */

   idtbuf[0] = '\0';

   for (i = 1; i < argc; i++)
   {
      bp = argv[i];

      if (*bp == '-')
      {
	 bp++;
         while (*bp) switch (*bp)
         {
	 case 'a':
	    aoutformat = TRUE;
	    bp++;
	    break;

	 case 'C':
	    i++;
	    if (i >= argc) goto USAGE;
	    cseguserorg = getnum (argv[i]);
	    bp++;
	    break;

	 case 'D':
	    i++;
	    if (i >= argc) goto USAGE;
	    dseguserorg = getnum (argv[i]);
	    bp++;
	    break;

	 case 'P':
	    i++;
	    if (i >= argc) goto USAGE;
	    pseguserorg = getnum (argv[i]);
	    bp++;
	    break;

	 case 'c':
	    cassette = TRUE;
	    bp++;
	    break;

	 case 'd':
	    dsegatend = FALSE;
	    bp++;
	    break;

	 case 'g':
	    symtout = TRUE;
	    bp++;
	    break;

	 case 'i':
	    i++;
	    if (i >= argc) goto USAGE;
	    if (strlen(argv[i]) >= IDTSIZE) argv[i][IDTSIZE] = '\0';
	    strcpy (idtbuf, argv[i]);
	    bp++;
	    break;

	 case 'L':
	    if (libdircnt >= MAXFILES)
	    {
	       fprintf (stderr, "Maximum library directories exceeded: %d\n",
	       		MAXFILES);
	       exit (ABORT);
	    }
	    libdirs[libdircnt++] = bp+1;
	    while (*bp) bp++;
	    break;

	 case 'l':
	    if (libcnt >= MAXFILES)
	    {
	       fprintf (stderr, "Maximum libraries exceeded: %d\n", MAXFILES);
	       exit (ABORT);
	    }
	    libs[libcnt++] = bp+1;
	    while (*bp) bp++;
	    break;

	 case 'M':
	    listMode = TRUE;
	    bp++;
	    break;

	 case 'm':
	    i++;
	    if (i >= argc) goto USAGE;
	    lstfile = argv[i];
	    listmode = TRUE;
	    bp++;
	    break;

         case 'o':
            i++;
	    if (i >= argc) goto USAGE;
            outfile = argv[i];
	    bp++;
            break;

	 case 'p':
	    printf ("Partial linking has been DEPRECATED\n");

         default:
      USAGE:
	    printf ("lnk990 - version %s\n", VERSION);
	    printf ("usage: lnk990 [-options] -o file.obj infile.obj...\n");
            printf (" options:\n");
	    printf ("    -a           - Generate a.out format\n");
	    printf ("    -c           - Cassette mode\n");
	    printf ("    -C address   - CSEG origin\n");
	    printf ("    -D address   - DSEG origin\n");
            printf ("    -g           - Generate symbol table in object\n");
            printf ("    -i IDT       - Define IDT name for link\n");
            printf ("    -llib        - Library name\n");
            printf ("    -Llibdir     - Library directory\n");
	    printf ("    -m file.map  - Generate map listing to file.map\n");
	    printf ("    -o file.obj  - Linked output file to file.obj\n");
	    printf ("    -P address   - PSEG origin\n");
	    return (ABORT);
         }
      }

      else
      {
	 if (infilecnt >= MAXFILES)
	 {
	    fprintf (stderr, "Maximum input files exceeded: %d\n", MAXFILES);
	    exit (ABORT);
	 }
         strcpy (infiles[infilecnt++].name, argv[i]);
      }

   }

   if (!(infilecnt && outfile)) goto USAGE;

#ifdef DEBUG
   printf (" outfile = %s\n", outfile);
   for (i = 0; i < infilecnt; i++)
      printf (" infiles[%d].name = %s\n", i, infiles[i].name);
   for (i = 0; i < libcnt; i++)
      printf (" libs[%d] = %s\n", i, libs[i]);
   for (i = 0; i < libdircnt; i++)
      printf (" libdirs[%d] = %s\n", i, libdirs[i]);
   if (listmode)
      printf (" lstfile = %s\n", lstfile);
#endif

   /*
   ** Open the files.
   */

   if ((outfd = fopen (outfile, "w")) == NULL)
   {
      perror ("lnk990: Can't open output file");
      exit (ABORT);
   }
   if (listMode)
   {
      listmode = TRUE;
      sprintf (temp, "%s.map", outfile);
      lstfile = temp;
   }

   if (listmode)
   {
      if ((lstfd = fopen (lstfile, "w")) == NULL)
      {
	 perror ("lnk990: Can't open listing file");
	 exit (ABORT);
      }

   }

   /*
   ** Get current date/time.
   */

   curtime = time(NULL);
   timeblk = localtime (&curtime);
   strcpy (datebuf, ctime(&curtime));
   *strchr (datebuf, '\n') = '\0';

   /*
   ** If IDT not specified, use output file name
   */

   if (!idtbuf[0])
   {
      char *sp, *ep;
      char temp[256];

      strcpy (temp, outfile);
      if ((sp = strrchr (temp, '/')) == NULL)
         sp = temp;
      else
         sp++;
      if ((ep = strchr (sp, '.')) != NULL)
         *ep = '\0';
      if (strlen(sp) >= IDTSIZE) sp[IDTSIZE] = '\0';
      strcpy (idtbuf, sp);
#ifdef DEBUG
      printf (" idtbuf = %s\n", idtbuf);
#endif
   }
   for (i = 0; idtbuf[i]; i++)
   {
      if (islower (idtbuf[i])) 
         idtbuf[i] = toupper (idtbuf[i]);
   }

   /*
   ** Clear memory.
   */

   memset (&memory, 0, sizeof(memory));
   memset (&dsegmem, 0, sizeof(dsegmem));
   memset (&csegmem, 0, sizeof(csegmem));
   memset (&memctl, 0, sizeof (memctl));
   memset (&dsegctl, 0, sizeof (dsegctl));
   memset (&csegctl, 0, sizeof (csegctl));

   /*
   ** Load the objects for pass 1.
   */

   dsegorg = dseguserorg > 0 ? dseguserorg : 0;
   csegorg = cseguserorg > 0 ? cseguserorg : 0;
   pc = pseguserorg > 0 ? pseguserorg : 0;
   curmod = 0;
   for (i = 0; i < infilecnt; i++)
   {
      if ((infd = fopen (infiles[i].name, "rb")) == NULL)
      {
	 sprintf (inbuf, "lnk990: Can't open input file %s", infiles[i].name);
	 perror (inbuf);
	 exit (ABORT);
      }

      pc = (pc + 1) & 0xFFFE;
      pc = lnkloader (infd, pc, 1, infiles[i].name, FALSE);

      fclose (infd);
   }

   /*
   ** If libraries, search them.
   */

   if (libcnt)
   {
      for (i = 0; i < libcnt; i++)
      {
	 int j;
	 char libfile[1024];
	 char libname[256];

	 sprintf (libname, "lib%s", libs[i]);
	 if (libdircnt)
	 {
	    for (j = 0; j < libdircnt; j++)
	    {
	       sprintf (libfile, "%s/lib%s.a", libdirs[j], libs[i]);

	       if ((infd = fopen (libfile, "rb")) != NULL)
	       {
#ifdef DEBUGLIBRARY
		  printf (" Library: %s\n", libfile);
#endif
		  pc = (pc + 1) & 0xFFFE;
		  status = lnklibrary (infd, pc, 1, libname, libfile);
		  fclose (infd);
	       }
	    }
	 }
	 else
	 {
	    sprintf (libfile, "lib%s.a", libs[i]);

	    if ((infd = fopen (libfile, "rb")) != NULL)
	    {
#ifdef DEBUGLIBRARY
	       printf (" Library: %s\n", libfile);
#endif
	       pc = (pc + 1) & 0xFFFE;
	       status = lnklibrary (infd, pc, 1, libname, libfile);
	       fclose (infd);
	    }
	 }
      }
   }

   /*
   ** Clear memory.
   */

   memset (&memory, 0, sizeof(memory));
   memset (&dsegmem, 0, sizeof(dsegmem));
   memset (&csegmem, 0, sizeof(csegmem));
   memset (&memctl, 0, sizeof (memctl));
   memset (&dsegctl, 0, sizeof (dsegctl));
   memset (&csegctl, 0, sizeof (csegctl));

   /*
   ** reLoad the objects for pass 2.
   */

   if (dsegatend)
   {
      pc = (pc + 1) & 0xFFFE;
      dsegorg = dseguserorg > 0 ? dseguserorg : pc;
      if (dseguserorg < 0)
      {
	 updatesegs (dsegorg, DSEG);
	 pc += dseglen;
      }
#ifdef DEBUGLOADER
      printf ("main-1: pc = %04X, dsegorg = %04X, dseglen = %04X\n",
	      pc, dsegorg, dseglen);
#endif
      pc = (pc + 1) & 0xFFFE;
      csegorg = cseguserorg > 0 ? cseguserorg : pc;
      if (cseguserorg < 0)
      {
	 updatesegs (csegorg, CSEG);
      }
#ifdef DEBUGLOADER
      printf ("   csegorg = %04X, cseglen = %04X\n",
	      csegorg, cseglen);
#endif
   }
   else
   {
      dsegorg = 0;
      csegorg = 0;
   }

   /*
   ** If a.out mode add _etext, _edata, _end symbols.
   */
   if (aoutformat)
   {
      cseglen = 0;
      pgmlen += pseguserorg > 0 ? pseguserorg : 0;
      for (i = 0; i < cmncount; i++)
      {
	 cseglen += cmnnodes[i+1].length;
      }
      processdef ("_etext", NULL, 1, 1, LONGSYM, pgmlen, 0);
      processdef ("_edata", NULL, 1, 1, LONGSYM, pgmlen, 0);
      processdef ("_end",   NULL, 1, 1, LONGSYM, pgmlen+dseglen+cseglen, 0);
   }

   dseglen = 0;
   cseglen = 0;
   pgmlen = 0;

   pc = pseguserorg > 0 ? pseguserorg : 0;
   curmod = 0;
   for (i = 0; i < infilecnt; i++)
   {
      int libload;

      libload = FALSE;
#ifdef DEBUGLIBRARY
      printf ("lnk990: infile[%d].name = %s, library = %s\n",
	       i, infiles[i].name, infiles[i].library ? "TRUE" : "FALSE");
#endif
      if ((infd = fopen (infiles[i].name, "rb")) == NULL)
      {
	 sprintf (inbuf, "lnk990: Can't open input file %s", infiles[i].name);
	 perror (inbuf);
	 exit (ABORT);
      }
      if (infiles[i].library)
      {
         if (fseek (infd, infiles[i].offset, SEEK_SET) < 0)
	 {
	    sprintf (inbuf, "lnk990: Can't fseek input file %s",
		     infiles[i].name);
	    perror (inbuf);
	    exit (ABORT);
	 }
	 libload = TRUE;
      }

      pc = (pc + 1) & 0xFFFE;
      pc = lnkloader (infd, pc, 2, infiles[i].name, libload);

      fclose (infd);
   }

   if (dsegatend)
   {
      pc = (pc + 1) & 0xFFFE;
      dsegorg = pc;
      dseglen = 0;
      for (i = 0; i < modcount; i++)
      {
         if (modules[i].dsegmodule)
	    dseglen += modules[i].length;
      }
      csegorg = pc + dseglen;
      pc = (pc + 1) & 0xFFFE;
      cseglen = 0;
      for (i = 0; i < cmncount; i++)
      {
	 cseglen += cmnnodes[i+1].length;
      }
#ifdef DEBUGLOADER
      printf ("main-2: pc = %04X, dsegorg = %04X, dseglen = %04X\n",
	      pc, dsegorg, dseglen);
      printf ("   csegorg = %04X, cseglen = %04X\n",
	      csegorg, cseglen);
#endif
      if ((pgmlen + dseglen + cseglen) > 0xFFFF)
      {
	 errcount++;
         toobig = TRUE;
      }
   }
   else
   {
      cseglen = 0;
      for (i = 0; i < cmncount; i++)
      {
	 cseglen += cmnnodes[i+1].length;
      }
      if ((pgmlen + cseglen) > 0xFFFF)
      {
	 errcount++;
         toobig = TRUE;
      }
   }

   /*
   ** Go back and fill in all defined EXTNDX records.
   */

   fillinextndx();

   /*
   ** Generate link map.
   */

   if (listmode)
   {

      /*
      ** Print the modules
      */

      printheader (lstfd);
      fprintf (lstfd,
	"MODULE    NO  ORIGIN  LENGTH    DATE      TIME    CREATOR   FILE\n\n");
      linecnt += 2;
      for (i = 0; i < modcount; i++)
      {
	 printheader (lstfd);
	 if (dsegatend)
	 {
	    if (modules[i].dsegmodule && dseguserorg < 0)
	       modules[i].origin += dsegorg;
	 }
	 if ((strlen (modules[i].objfile) > 18) && 
	     (modules[i].objfile[0] == '/'))
	 {
	    char *ep = strrchr (modules[i].objfile, '/');
	    if (ep)
	       strcpy (modules[i].objfile, ep+1);
	 }
         fprintf (lstfd, MODFORMAT, modules[i].name, modules[i].number,
		  modules[i].origin, modules[i].length, modules[i].date,
		  modules[i].time, modules[i].creator, modules[i].objfile);
	 linecnt ++;
	 printheader(lstfd);
      }

      if (cmncount)
      {
	 if ((linecnt + 4) >= LINESPAGE)
	    printheader(lstfd);

	 fprintf (lstfd, "\nCOMMON    NO  ORIGIN  LENGTH\n\n");
	 linecnt += 3;
	 for (i = 0; i < cmncount; i++)
	 {
	    fprintf (lstfd, CMNFORMAT, cmnnodes[i+1].name, cmnnodes[i+1].number,
		     cmnnodes[i+1].origin, cmnnodes[i+1].length);
	    linecnt ++;
	    printheader(lstfd);
	 }
      }

      /*
      ** Print the symbol table
      */

      printsymbols (lstfd);
      printundefs (lstfd);

      printheader (lstfd);
      fprintf (lstfd, "\n");
      linecnt ++;
      printheader(lstfd);

      if (muldefs)
      {
	 fprintf (lstfd, "** There are multiply defined symbols - M\n");
	 linecnt ++;
	 printheader(lstfd);
      }
      if (toobig)
      {
	 fprintf (lstfd, "** Program is too big.\n");
	 linecnt ++;
	 printheader(lstfd);
      }

      i = pgmlen;
      if (dsegatend)
         i += dseglen + cseglen;
      else
         i += cseglen;
      fprintf (lstfd, "\n%04X Length of program \n", i);
      linecnt ++;
      printheader(lstfd);
      if (relentry >= 0)
      {
	 fprintf (lstfd, "%04X Program entry \n", relentry);
	 linecnt ++;
	 printheader(lstfd);
      }
      fprintf (lstfd, "%d Errors\n", errcount);
   }
   else
   {
      scansymbols();
      if (toobig)
      {
	 printf ("lnk990: Program is too big.\n");
      }
   }

   if (errcount)
   {
      printf ("lnk990: %d errors\n", errcount);
      status = 1;
   }

   /*
   ** Punch out linked object
   */

   if (!errcount)
      lnkpunch (outfd, symtout);

   /*
   ** Close the files
   */

   fclose (outfd);
   if (listmode)
      fclose (lstfd);

   return (status == 0 ? NORMAL : ABORT);
}
