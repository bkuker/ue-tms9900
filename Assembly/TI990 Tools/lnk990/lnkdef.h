/***********************************************************************
*
* lnkdef.h - Linker header for the TI 990 computer.
*
* Changes:
*   06/05/03   DGP   Original.
*   06/25/03   DGP   Added Module type and associated formats.
*   05/25/04   DGP   Added Long entry/extern tags.
*   04/27/05   DGP   Put IDT in record sequence column 72.
*   12/30/05   DGP   Changed SymNode to use flags.
*   01/21/07   DGP   Added scansymbols() for non-listmode.
*   01/23/07   DGP   Added library support.
*   12/02/10   DGP   Added EXTNDX support.
*   12/07/10   DGP   Added ABSDEF flag.
*   12/14/10   DGP   Boost MAXFILES and MAXMODULES to 1000.
*   02/10/15   DGP   Remove (DEPRECATE) Partial Link support.
*   10/09/15   DGP   Added Long symbol common tags.
*
***********************************************************************/

#include <stdio.h>

/*
** Definitions
*/

#define NORMAL 0
#define ABORT  12

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define VERSION "2.3.2"

#define EOFSYM ':'

#define MAXLINE 256
#define LINESPAGE 60
#define MAXFILES 1000
#define IDTSIZE 8

#define MAXCSEGS 50
#define MAXMODULES 1000
#define MAXSYMLEN 32
#define MAXSHORTSYMLEN 6
#define MAXSYMBOLS 5000

#define MEMSIZE 65768

#define MAXPATHNAMESIZE 256

#define TEMPSPEC "lnkXXXXXX"

/*
** Object output formats
*/

#define OBJFORMAT "%c%04X"
#define IDTFORMAT "%c%04X%-8.8s"
#define REFFORMAT "%c%4.4X%-6.6s"
#define DEFFORMAT "%c%4.4X%-6.6s"
#define SEQFORMAT "%4.4d\n"
#define LREFFORMAT "%c%4.4X%-32.32s"
#define LDEFFORMAT "%c%4.4X%-32.32s"

/*
** Listing formats
*/

#define H1FORMAT "LNK990 %-8.8s  %-24.24s  %s Page %04d\n\n"

#define MODFORMAT "%-8.8s %3d   %04X    %04X   %s  %s  %s  %s\n"
#define CMNFORMAT "%-8.8s %3d   %04X    %04X\n"
#define SYMFORMAT "%%-%d.%ds %%4.4X%%c%%c %%3d  "

#define UNDEFFORMAT "%%-%d.%ds %%3d  "

/*
** Data type definitions
*/

#define int8            char
#define int16           short
#define int32           int
typedef int             t_stat;                         /* status */
typedef int             t_bool;                         /* boolean */
typedef unsigned int8   uint8;
typedef unsigned int16  uint16;
typedef unsigned int32  uint32, t_addr;                 /* address */

#if defined (WIN32)                                     /* Windows */
#define t_int64 __int64
#elif defined (__ALPHA) && defined (VMS)                /* Alpha VMS */
#define t_int64 __int64
#elif defined (__ALPHA) && defined (__unix__)           /* Alpha UNIX */
#define t_int64 long
#else                                                   /* default GCC */
#define t_int64 long long
#endif                                                  /* end OS's */
typedef unsigned t_int64        t_uint64, t_value;      /* value */
typedef t_int64                 t_svalue;               /* signed value */

#define MSB 0
#define LSB 1

/*
** Memory
*/

typedef struct
{
   char tag;
   int  flags;
   int	value;
} Memory;

#define GETMEM(m) \
(((memory[(m)+MSB] & 0xFF) << 8)|(memory[(m)+LSB] & 0xFF))

#define PUTMEM(m,v) \
{ memory[(m)+MSB] = (v) >> 8 & 0xFF; memory[(m)+LSB] = (v) & 0xFF; }

#define GETDSEGMEM(m) \
(((dsegmem[(m)+MSB] & 0xFF) << 8)|(dsegmem[(m)+LSB] & 0xFF))

#define PUTDSEGMEM(m,v) \
{ dsegmem[(m)+MSB] = (v) >> 8 & 0xFF; dsegmem[(m)+LSB] = (v) & 0xFF; }

#define GETCSEGMEM(m) \
(((csegmem[(m)+MSB] & 0xFF) << 8)|(csegmem[(m)+LSB] & 0xFF))

#define PUTCSEGMEM(m,v) \
{ csegmem[(m)+MSB] = (v) >> 8 & 0xFF; csegmem[(m)+LSB] = (v) & 0xFF; }

/*
** Symbol table
*/

typedef struct EXTndx_t
{
    struct EXTndx_t *next;
    int location;
} EXTndxlist;

typedef struct
{
   EXTndxlist *extlist;
   char symbol[MAXSYMLEN+2];
   char module[MAXSYMLEN+2];
   int flags;
   int value;
   int modnum;
   int extndx;
} SymNode;

#define RELOCATABLE 0x000000001
#define EXTERNAL    0x000000002
#define GLOBAL      0x000000004
#define LONGSYM     0x000000008
#define MULDEF      0x000000010
#define UNDEF       0x000000020
#define DSEG        0x000000040
#define CSEG        0x000000080
#define ABSDEF      0x000000100
#define SREF        0x000000200
#define LOAD        0x000000400
#define SYMTAG      0x000001000

/*
** Module table
*/

typedef struct 
{
   char name[IDTSIZE+2];
   char date[12];
   char time[12];
   char creator[12];
   char objfile[MAXPATHNAMESIZE+2];
   int dsegmodule;
   int origin;
   int length;
   int number;
   int symcount;
   SymNode *symbols[MAXSYMBOLS];
} Module;

/*
** CSEG table
*/

typedef struct
{
   char name[MAXSYMLEN];
   int length;
   int origin;
   int offset;
   int number;
} CSEGNode;

/*
** File node
*/

typedef struct 
{
   char name[MAXPATHNAMESIZE+2];
   int library;
   off_t offset;
} FILENode;

/*
** Object tags
*/

#define BINIDT_TAG	01
#define IDT_TAG		'0'
#define ABSENTRY_TAG	'1'
#define RELENTRY_TAG	'2'
#define RELEXTRN_TAG	'3'
#define ABSEXTRN_TAG	'4'
#define RELGLOBAL_TAG	'5'
#define ABSGLOBAL_TAG	'6'
#define CKSUM_TAG	'7'
#define NOCKSUM_TAG	'8'
#define ABSORG_TAG	'9'
#define RELORG_TAG	'A'
#define ABSDATA_TAG	'B'
#define RELDATA_TAG	'C'
#define LOADBIAS_TAG	'D'
#define EXTNDX_TAG	'E'
#define EOR_TAG		'F'

#define RELSYMBOL_TAG	'G'
#define ABSSYMBOL_TAG	'H'
#define PGMIDT_TAG	'I'
#define CMNSYMBOL_TAG	'J'
#define COMMON_TAG	'M'
#define CMNDATA_TAG	'N'
#define CMNORG_TAG	'P'
#define CBLSEG_TAG	'Q'
#define DSEGORG_TAG	'S'
#define DSEGDATA_TAG	'T'
#define LOAD_TAG	'U'
#define RELSREF_TAG	'V'
#define CMNGLOBAL_TAG	'W'
#define CMNEXTRN_TAG	'X'
#define ABSSREF_TAG	'Y'
#define CMNSREF_TAG	'Z'

#define LRELSYMBOL_TAG	'i'
#define LABSSYMBOL_TAG	'j'
#define LRELEXTRN_TAG	'l'
#define LABSEXTRN_TAG	'm'
#define LRELGLOBAL_TAG	'n'
#define LABSGLOBAL_TAG	'o'
#define LLOAD_TAG	'p'
#define LRELSREF_TAG	'q'
#define LABSSREF_TAG	'r'
#define LCMNGLOBAL_TAG	'w'
#define LCMNEXTRN_TAG	'x'
#define LCMNSREF_TAG	'z'

#define CHARSPERREC	67  /* Chars per object record */
#define CHARSPERCASREC	73  /* Chars per cassette object record */
#define WORDTAGLEN	5   /* Word + Object Tag length */
#define BINWORDTAGLEN	3   /* Word + Binary Object Tag length */
#define EXTRNLEN	11  /* Tag + SYMBOL + addr */
#define GLOBALLEN	11  /* Tag + SYMBOL + addr */
#define IDTLEN		13  /* Tag + IDT + length */
#define BINIDTLEN	11  /* Tag + IDT + binary length */
#define IDTCOLUMN	72  /* Where to put the IDT */
#define SEQUENCENUM	76  /* Where to put the sequence */

#define TIMEOFFSET 17
#define DATEOFFSET 27
#define CREATOROFFSET 38


/*
** External functions.
*/

extern int lnkloader (FILE *, int, int, char *, int);
extern uint16 getnumfield (unsigned char *, int);
extern void fillinextndx (void);
extern void processdef (char *, Module *, int, int, int, uint16, int);

extern int lnklibrary (FILE *, int, int, char *, char *);

extern int lnkpunch (FILE *, int);

extern void printheader (FILE *);

extern SymNode *symlookup (char *, Module *, int);
extern SymNode *reflookup (char *, Module *, int);
extern SymNode *modsymlookup (char *, Module *, int);
