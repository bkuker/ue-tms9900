/***********************************************************************
*
* asm990 - Assembler for the TI 990 computer.
*
* Changes:
*   05/21/03   DGP   Original.
*   08/14/03   DGP   Added model and define regs options.
*   08/18/03   DGP   Added Include directory option.
*   04/08/04   DGP   Added "Cassette" mode.
*   07/01/04   DGP   Added /10A CPU.
*   03/31/05   DGP   Improve arg parser.
*   12/26/05   DGP   Added DSEG support.
*   12/30/05   DGP   Changes SymNode to use flags.
*   06/28/06   DGP   Check for files with the same names.
*   07/30/07   DGP   Change mktemp to mkstemp.
*   12/15/08   DGP   Added widelist option.
*   05/27/09   DGP   Added pass 0 macro processor.
*   05/30/09   DGP   Added multiple assembly capability.
*   11/30/10   DGP   Changed tabstops to match generated lines.
*   12/16/10   DGP   Added a unified source read function readsource().
*   12/23/10   DGP   Added -a "options".
*   02/09/11   DGP   Unify segments in segvaltype.
*   11/09/11   DGP   Correct missing object file test.
*   02/03/14   DGP   Added SYMT support.
*   12/29/14   DGP   Allow for nested $IF/ASMIF statements.
*   11/18/15   DGP   Added inpseg flag.
*   01/27/16   DGP   Fixed include directory.
*	
***********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include "asmdef.h"

char *pscanbuf;			/* Pointer for tokenizers */
int pc;				/* the assembler pc */
int model;			/* CPU model */
int rightmargin;		/* The right margin to stop scanning */
int genxref;			/* Generate cross reference listing */
int gensymt;			/* Generate symbol table in object */
int inpseg;			/* In a PSEG section */
int indseg;			/* In a DSEG section */
int incseg;			/* In a CSEG section */
int segvaltype;			/* DSEG/CSEG value type*/
int csegcnt;			/* CSEG count */
int csegval;			/* current CSEG number */
int rorgpc;			/* Relocatable PC */
int filenum;			/* COPY file index */
int fileseq;			/* COPY file sequence */
int linenum;			/* Source line number */
int inpass;			/* Current pass */
int absolute;			/* In absolute section */
int cassette;			/* Cassette mode (no sequence numbers) */
int txmiramode;			/* TXMIRA mode */
int indorg;			/* In DORG section */
int widelist;			/* Generate wide listing */
int symbolcount;		/* Number of symbols in symbol table */
int errcount;			/* Number of errors in assembly */
int errnum;			/* Index into errline array */
int p1errcnt;			/* Number of pass 0/1 errors */
int pgmlength;			/* Length of program */
int codelength;			/* Code Length of program */		
int dseglength;			/* DSEG Length of program */
int wpntaddr;			/* Current workspace address */
int libincnt;			/* Number of LIBIN's active */
int libout;			/* LIBOUT active */
int opdefcount;			/* Number of user defined opcodes */
int extvalue;			/* External ref. index value */
int bunlist = FALSE;		/* Byte UNLST option */
int dunlist = FALSE;		/* Data UNLST option */
int munlist = FALSE;		/* Macro UNLST option */
int tunlist = FALSE;		/* Text UNLST option */
char errline[10][120];		/* Pass 2 error lines for current statment */
char inbuf[MAXLINE];		/* The input buffer for the scanners */
char incldir[MAXLINE];		/* -I include directory */
char extname[MAXSYMLEN+2];	/* External ref. index */

COPYfile files[MAXASMFILES];	/* COPY file descriptors */
LIBfile libinfiles[MAXASMFILES];/* LIBIN file descriptors */
LIBfile liboutfile;		/* LIBOUT file descriptor */
CSEGNode csegs[MAXCSEGS];	/* CSEG table */

SymNode *symbols[MAXSYMBOLS];	/* The Symbol table */
ErrTable p1error[MAXERRORS];	/* The pass 0/1 error table */
OpDefCode *opdefcode[MAXDEFOPS];/* The user defined opcode table */

static IFTable iftable[MAXIFDEPTH];/* ASMIF processing table */

static int verbose;		/* Verbose mode */
static int filecnt;		/* File process count */
static int reccnt;		/* File record count */
static int ifindex;		/* IF table index (nesting level) */

/***********************************************************************
* if_start - Start ASMIF statement.
***********************************************************************/

int
if_start (int val)
{
   int inskip = FALSE;

   if ((ifindex + 1) > MAXIFDEPTH)
   {
      logerror ("ASMIF nesting depth exceeded.");
      return (FALSE);
   }
   ifindex++;
   if (iftable[ifindex-1].skip)
   {
      iftable[ifindex].skip = iftable[ifindex-1].skip;
      inskip = TRUE;
   }
   else
   {
      inskip = iftable[ifindex].skip = val ? FALSE : TRUE;
   }
#ifdef DEBUGASMIF
   printf ("ASMIF: pass = %d, ifindex = %d, val = %d, inskip = %s\n",
	   inpass, ifindex, val, inskip ? "TRUE" : "FALSE");
#endif
   return (inskip);
}

/***********************************************************************
* if_else - Prcoess ASMELS statement.
***********************************************************************/

int
if_else (void)
{
   int inskip = FALSE;

   if (iftable[ifindex-1].skip)
   {
      inskip = TRUE;
   }
   else
   {
      iftable[ifindex].skip = iftable[ifindex].skip ? FALSE : TRUE;
      inskip = iftable[ifindex].skip;
   }
#ifdef DEBUGASMIF
   printf ("ASMELS: pass = %d, ifindex = %d, inskip = %s\n",
	   inpass, ifindex, inskip ? "TRUE" : "FALSE");
#endif
   return (inskip);
}

/***********************************************************************
* if_end - Prcoess ASMEND statement.
***********************************************************************/

int
if_end (void)
{
   int inskip = FALSE;

   if (iftable[ifindex-1].skip)
   {
      inskip = TRUE;
   }
#ifdef DEBUGASMIF
   printf ("ASMEND: pass = %d, ifindex = %d, inskip = %s\n",
	   inpass, ifindex, inskip ? "TRUE" : "FALSE");
#endif
   ifindex--;
   if (ifindex < 0)
      ifindex = 0;
   return (inskip);
}

/***********************************************************************
* tabpos - Return TRUE if col is a tab stop.
***********************************************************************/

static int
tabpos (int col, int tabs[]) 
{
   if (col < BUFSIZ)
      return (tabs[col]);

    return (TRUE); 

} /* tabpos */

/***********************************************************************
* detabit - Convert tabs to equivalent number of blanks.
***********************************************************************/

static void
detabit (int tabs[], FILE *ifd, FILE *ofd)
         
{
   int c, col;
 
   col = 0;
   while ((c = fgetc (ifd)) != EOF)
   {

      switch (c)
      {

         case '\t' :
            while (TRUE)
	    { 
               fputc (' ', ofd);
               if (tabpos (++col, tabs) == TRUE) 
                  break ;
            } 
            break;

         case '\n' :
            fputc (c, ofd);
            col = 0;
	    reccnt++;
            break;

         default: 
            fputc (c, ofd);
            col++;

      } /* switch */

   } /* while */

} /* detab */

/***********************************************************************
* alldig - Check if all digits.
***********************************************************************/

static int
alldig (char *digs)
{
   while (*digs)
   {
      if (!isdigit (*digs))
      {
         return (FALSE);
      }
      digs++;
   }

   return (TRUE);

} /* alldig */

/***********************************************************************
* settab - Set initial tab stops.
***********************************************************************/

static void
settab (int tabs[], int argc, char *argv[])
{
   int m, p, i, j, l;
   char *bp;
  
   p = 0;

   for (i = 0; i < BUFSIZ; i++)
       tabs[i] = FALSE;

   for (j = 1; j < argc; j++)
   {

      bp = argv[j];

      if (*bp == '+')
         bp++;

      if (alldig (bp))
      {

         l = atoi (bp) ;

         if (l < 0 || l >= BUFSIZ)
             continue;

         if (*argv[j] != '+')
         { 
            p = l;
            tabs[p] = TRUE;
         } 

         else
         { 
            if (p == 0) 
               p = l;
            for (m = p; m < BUFSIZ; m += l) 
            {
               tabs[m] = TRUE;
            }
         }

      }

   } 

   if (p == 0)
   {
      for (i = 8; i < BUFSIZ; i += 8) 
      {
         tabs[i] = TRUE;
      }
   }

} /* settab */

/***********************************************************************
* detab - Detab source.
***********************************************************************/

int
detab (FILE *ifd, FILE *ofd, char *type) 
{
   int tabs[BUFSIZ];
   char *tabstops[] = { "as", "7", "15", "+8", "\0" };
  
#ifdef DEBUG
   fprintf (stderr, "detab: Entered: infile = %s, outfile = %s\n",
	    infile, outfile);
#endif

   if (verbose)
      fprintf (stderr, "asm990: Detab %s file \n", type);
   reccnt = 0;

   /* set initial tab stops */

   settab (tabs, 4, tabstops);

   detabit (tabs, ifd, ofd);

   if (verbose)
      fprintf (stderr, "asm990: File contains %d records\n", reccnt);

   return (0);

} /* detab */

/***********************************************************************
* defineregs - Define registers.
***********************************************************************/

static void
defineregs (void)
{
   int i;

   for (i = 0; i < 16; i++)
   {
      SymNode *sym;
      char reg[8];

      sprintf (reg, "R%d", i);
      if ((sym = symlookup (reg, TRUE, FALSE)) != NULL)
      {
	 sym->value = i;
	 sym->flags = PREDEFINE;
      }
   }
}

/***********************************************************************
* Main procedure
***********************************************************************/

int
main (int argc, char **argv)
{
   FILE *infd = NULL;
   FILE *outfd = NULL;
   FILE *lstfd = NULL;
   FILE *tmpfd0 = NULL;
   FILE *tmpfd1 = NULL;
   char *infile = NULL;
   char *outfile = NULL;
   char *lstfile = NULL;
   char *bp;
   char *pp;
   int i;
   int done;
   int tmpdes0, tmpdes1;
   int listmode = FALSE;
   int lclxref;
   int lclsymt;
   int status = 0;
   char tname0[64];
   char tname1[64];
  
#ifdef DEBUG
   printf ("asm990: Entered:\n");
   printf ("args:\n");
   for (i = 1; i < argc; i++)
   {
      printf ("   arg[%2d] = %s\n", i, argv[i]);
   }
#endif

   /*
   ** Process command line arguments
   */

   verbose = FALSE;
   lclxref = FALSE;
   lclsymt = FALSE;
   cassette = FALSE;
   txmiramode = FALSE;
   widelist = FALSE;
   bunlist = FALSE;
   dunlist = FALSE;
   munlist = FALSE;
   tunlist = FALSE;
   filecnt = 0;
   model = 12;

   for (i = 1; i < argc; i++)
   {
      bp = argv[i];

      if (*bp == '-')
      {
         for (bp++; *bp; bp++) switch (*bp)
         {
	 case 'a':
	    i++;
	    if (i >= argc) goto USAGE;
	    if (processoption (argv[i], FALSE) < 0) goto USAGE;
	    lclsymt = gensymt;
	    lclxref = genxref;
	    break;

	 case 'c':
	    cassette = TRUE;
	    break;

	 case 'g':
	    lclsymt = TRUE;
	    break;

	 case 'I':
	    i++;
	    if (i >= argc) goto USAGE;
	    strcpy (incldir, argv[i]);
	    break;

	 case 'l':
	    i++;
	    if (i >= argc) goto USAGE;
	    lstfile = argv[i];
	    listmode = TRUE;
	    break;

         case 'm':
            i++;
	    if (i >= argc) goto USAGE;
	    model = 0;
	    for (pp = argv[i]; *pp; pp++)
	    {
	       if (isdigit(*pp))
	       {
		  model = (model * 10) + (*pp - '0');
	       }
	       else
	       {
	          if (*pp == 'a' || *pp == 'A')
		  {
		     if (model == 10) model = 11;
		     else goto BAD_MODEL;
		     break;
		  }
		  else goto BAD_MODEL;
	       }
	    }
	    if (!(model ==  4 || model ==  5 || model == 9 ||
		  model == 10 || model == 11 || model == 12))
	    {
	    BAD_MODEL:
	       printf ("asm990: Invalid CPU model specification %s\n", argv[i]);
	       goto USAGE;
	    }
            break;

         case 'o':
            i++;
	    if (i >= argc) goto USAGE;
            outfile = argv[i];
            break;

	 case 't':
	    txmiramode = TRUE;
	    break;

	 case 'w':
	    widelist = TRUE;
	    break;

	 case 'x':
	    lclxref = TRUE;
	    break;

	 case 'V': /* Verbose mode */
	    verbose = TRUE;
	    break;

         default:
      USAGE:
	    printf ("asm990 - version %s\n", VERSION);
	    printf ("usage: asm990 [-options] -o file.obj file.asm\n");
            printf (" options:\n");
	    printf ("    -a \"OP1,OP2\"  - Assembly options\n");
            printf ("    -c            - Cassette mode\n");
            printf ("    -g            - Generate symbol table in object\n");
	    printf ("    -I includedir - Include directory for COPY\n");
	    printf ("    -l file.lst   - Generate listing to file.lst\n");
	    printf ("    -m model      - CPU model = 4, 5, 9, 10, 10A, 12\n");
	    printf ("    -o file.obj   - Generate object to file.obj\n");
            printf ("    -t            - TXMIRA mode\n");
            printf ("    -w            - Wide listing\n");
	    printf ("    -x            - Generate cross reference\n");
	    return (ABORT);
         }
      }

      else
      {
         if (infile) goto USAGE;
         infile = argv[i];
      }

   }

   if (!(infile && outfile)) goto USAGE;

   if (!strcmp (infile, outfile))
   {
      fprintf (stderr, "asm990: Identical source and object file names\n");
      exit (ABORT);
   }
   if (listmode)
   {
      if (!strcmp (outfile, lstfile))
      {
	 fprintf (stderr, "asm990: Identical object and listing file names\n");
	 exit (ABORT);
      }
      if (!strcmp (infile, lstfile))
      {
	 fprintf (stderr, "asm990: Identical source and listing file names\n");
	 exit (ABORT);
      }
   }

#ifdef DEBUG
   printf (" infile = %s\n", infile);
   printf (" outfile = %s\n", outfile);
   if (listmode)
      printf (" lstfile = %s\n", lstfile);
#endif

   /*
   ** Create temp files for intermediate use.
   */

#ifdef WIN32
   sprintf (tname0, "a0%s", TEMPSPEC);
#else
   sprintf (tname0, "/tmp/a0%s", TEMPSPEC);
#endif

   if ((tmpdes0 = mkstemp (tname0)) < 0)
   {
      perror ("asm990: Can't mkstemp");
      return (ABORT);
   }

   /*
   ** Open the files.
   */

   if ((infd = fopen (infile, "r")) == NULL)
   {
      perror ("asm990: Can't open input file");
      exit (ABORT);
   }
   if ((outfd = fopen (outfile, "w")) == NULL)
   {
      perror ("asm990: Can't open output file");
      exit (ABORT);
   }
   if (listmode) {
      if ((lstfd = fopen (lstfile, "w")) == NULL)
      {
	 perror ("asm990: Can't open listing file");
	 exit (ABORT);
      }
   }

   if ((tmpfd0 = fdopen (tmpdes0, "w+")) == NULL)
   {
      perror ("asm990: Can't open temporary file");
      exit (ABORT);
   }

   /*
   ** Mark tmp files for deletion
   */

   unlink (tname0);

   /*
   ** Detab the source, so we know where we are on a line.
   */

   if (verbose) fprintf (stderr, "\n");
   status = detab (infd, tmpfd0, "source");
   fclose (infd);

   /*
   ** Rewind the input.
   */

   if (fseek (tmpfd0, 0, SEEK_SET) < 0)
   {
      perror ("asm990: Can't rewind input temp file");
      exit (ABORT);
   }

   done = FALSE;
   while (!done)
   {
      /*
      ** Clear tables
      */

      for (i = 0; i < MAXSYMBOLS; i++)
      {
	 symbols[i] = NULL;
      }
      symbolcount = 0;

      memset (csegs, 0, sizeof (csegs));
      memset (iftable, 0, sizeof(iftable));

      rightmargin = RIGHTMARGIN;
      inpseg = TRUE;
      indseg = FALSE;
      incseg = FALSE;
      segvaltype = 0;
      csegcnt = 0;
      rorgpc = 0;
      filenum = 0;
      fileseq = 0;
      linenum = 0;
      absolute = FALSE;
      indorg = FALSE;
      symbolcount = 0;
      errcount = 0;
      errnum = 0;
      p1errcnt = 0;
      pgmlength = 0;
      codelength = 0;
      dseglength = -1;
      wpntaddr = -1;
      pscanbuf = inbuf;
      genxref = lclxref;
      gensymt = lclsymt;

      if (!txmiramode)
	 defineregs();

      if (verbose)
	 fprintf (stderr, "\nasm990: Processing file %d\n", filecnt+1);

#ifdef WIN32
      sprintf (tname1, "a1%s", TEMPSPEC);
#else
      sprintf (tname1, "/tmp/a1%s", TEMPSPEC);
#endif

      if ((tmpdes1 = mkstemp (tname1)) < 0)
      {
	 perror ("asm990: Can't mkstemp");
	 return (ABORT);
      }

      if ((tmpfd1 = fdopen (tmpdes1, "w+b")) == NULL)
      {
	 perror ("asm990: Can't open temporary file");
	 exit (ABORT);
      }

      unlink (tname1);

      /* 
      ** Call pass 0 to scan the macros.
      */

      if (verbose)
	 fprintf (stderr, "asm990: Calling pass 0\n");

      ifindex = 0;
      inpass = 0;
      status = asmpass0 (tmpfd0, tmpfd1);

      if (status)
      {
	 if (verbose)
	    fprintf  (stderr, "asm990: File %d: EOF\n",
		     filecnt+1);
	 if (tmpfd1)
	 {
	    fclose (tmpfd1);
	    unlink (tname1);
	 }
	 tmpfd1 = NULL;
         done = TRUE;
	 if (status == 2)
	 {
	    if (listmode)
	       fprintf (lstfd, "\nNo END record\n");
	    filecnt++;
	    goto PRINT_ERROR;
	 }
	 else
	 {
	    status = 0;
	 }
	 break;
      }
      if (filecnt++ > 0 && listmode)
         fputc ('\f', lstfd);


      /* 
      ** Call pass 1 to scan the source and get labels.
      */

      if (verbose)
	 fprintf (stderr, "asm990: Calling pass 1\n");

      ifindex = 0;
      inpass = 1;
      status = asmpass1 (tmpfd1);

      /*
      ** Call pass 2 to generate object and optional listing
      */

      if (verbose)
	 fprintf (stderr, "asm990: Calling pass 2\n");

      ifindex = 0;
      inpass = 2;
      status = asmpass2 (tmpfd1, outfd, listmode, lstfd);

      /*
      ** Close the intermediate file
      */

      if (tmpfd1)
      {
	 fclose (tmpfd1);
	 unlink (tname1);
      }
      tmpfd1 = NULL;

      /*
      ** Free symbols, etc.
      */

      for (i = 0; i < symbolcount; i++)
      {
	 if (symbols[i])
	 {
	    XrefNode *xr, *nxr;

	    xr = symbols[i]->xref_head;
	    while (xr)
	    {
	       nxr = xr->next;
	       free(xr);
	       xr = nxr;
	    }
	    free(symbols[i]);
	 }
      }
      for (i = 0; i < p1errcnt; i++)
      {
         free (p1error[i].errortext);
      }

      for (i = 0; i < opdefcount; i++)
      {
         if (opdefcode[i])
            free (opdefcode[i]);
      }

   PRINT_ERROR:
      if (listmode)
      {
	 fprintf (lstfd, "\n%d Errors in assembly\n", errcount);
      }

      if (errcount || verbose)
      {
	 fprintf (stderr, "asm990: File %d: %d Errors in assembly\n",
		  filecnt, errcount);
      }
   }
   fclose (tmpfd0);
   unlink (tname0);
   fclose (outfd);

   return (status == 0 ? NORMAL : ABORT);
}
