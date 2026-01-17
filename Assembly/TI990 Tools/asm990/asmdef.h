/***********************************************************************
*
* asmdef.h - Assembler header for the TI 990 computer.
*
* Changes:
*   05/21/03   DGP   Original.
*   08/12/03   DGP   Revert print format to one word per line.
*                    Add all operation types.
*   05/25/04   DGP   Added Long REF/DEF pseudo ops.
*   06/07/04   DGP   Added LONG, QUAD, FLOAT and DOUBLE pseudo ops.
*   04/27/05   DGP   Put IDT in record sequence column 72.
*   12/15/08   DGP   Added widelist option.
*   05/28/09   DGP   Added MACRO and CSEG support.
*   11/30/10   DGP   Added QUAL_xx definitions and ATTR_xx.
*   12/14/10   DGP   Added filenum to error processing
*   06/14/12   DGP   Correct checksum for EBCDIC.
*   12/29/14   DGP   Allow for nested $IF/ASMIF statements.
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

#define VERSION "2.4.6"

#define LITERALSYM '='		/* Begins a literal */
#define COMMENTSYM '*'		/* Begins a comment record */
#define MEMSYM '@'		/* Begins a memory reference */
#define STRINGSYM '\''		/* String delimiter */
#define EOFSYM ':'		/* Marks EOF for an object file */

#define MAXSYMLEN 32		/* Length of symbol (LDEF & LREF) */
#define MAXSHORTSYMLEN 6	/* Standard length of Symbol */
#define MAXSYMBOLS 4000		/* The number of Symbols in Symbol Table */

#if defined(USS) || defined(OS390)
#define MAXMACROS 200		/* The number of Macros in an assembly */
#define MAXMACROLINES 200	/* The number of lines in a Macro definition */
#else
#define MAXMACROS 1000		/* The number of Macros in an assembly */
#define MAXMACROLINES 1000	/* The number of lines in a Macro definition */
#endif

#define MAXMACARGS 64		/* The number of arguments in a Macro */
#define DEFMACNEST 16		/* The default Macro nesting level */
#define MAXDEFOPS 100		/* The number of user defined opcodes */
#define MAXIFDEPTH 10		/* The maximum of nested ASMIF statements */
#ifndef MAXPATHNAMESIZE
#define MAXPATHNAMESIZE 256
#endif

#define MAXCSEGS 20		/* The number of CSEGs in an assembly */

#define MAXLINE 256		/* The maximum length of a source line */
#define MAXASMFILES 30		/* The maximum number of COPY/LIBIN files */
#define LINESPAGE 60		/* The number of lines on a listing page */
#define RIGHTMARGIN 60		/* Default right margin for scanner */
#define LINESIZE 80		/* Standard line length also punch record */
#define WLINESIZE 132		/* Wide line length */
#define TTLSIZE 60		/* Length of the TITL */
#define IDTSIZE 8		/* Length of the IDT */
#define PRINTMARGIN 63		/* Source print margin for narrow listing */

#define TEMPSPEC "asXXXXXX"

/*
** Object output formats
*/

#define OBJFORMAT "%c%04X"
#define CMNOBJFORMAT "%c%04X%04X"
#define IDTFORMAT "%c%04X%-8.8s"
#define REFFORMAT "%c%4.4X%-6.6s"
#define DEFFORMAT "%c%4.4X%-6.6s"
#define BYTEFORMAT "%c%02X00"
#define SEQFORMAT "%4.4d\n"
#define CMNFORMAT "%c%4.4X%-6.6s%4.4X"

#define LREFFORMAT "%c%4.4X%-32.32s"
#define LDEFFORMAT "%c%4.4X%-32.32s"
#define LCMNFORMAT "%c%4.4X%-32.32s%4.4X"

/*
** Listing formats
*/

#define H1FORMAT "ASM990 %-8.8s  %-24.24s   %s Page %04d\n"
#define H1WFORMAT "ASM990 %-26.26s  %-58.58s   %s Page %04d\n"
#define H2FORMAT "%s\n\n"

#define L1FORMAT "%s %s %4d%c %s"
#define L2FORMAT "%s %s\n"
#define LITFORMAT "%s %s       %s\n"

#define PCBLANKS "    "
#define PCFORMAT "%04X"

#define OPBLANKS  "     "
#define OP1FORMAT "%04X%c"
#define ADDRFORMAT "%04X%c"
#define CHAR1FORMAT "%02X   "
#define DATA1FORMAT "%04X%c"

#define SYMFORMAT "%-8.8s %4.4X%c  "
#define LONGSYMFORMAT "%%-%d.%ds %%4.4X%%c  "

#define XREFHEADER "SYMBOL  VALUE     DEF            REFERENCES\n\n"
#define LONGXREFHEADER "SYMBOL %sVALUE     DEF            REFERENCES\n\n"

/*
** Expression types
*/

#define MEMEXPR	1
#define REGEXPR	2
#define LITEXPR	3

/*
** Data type definitions
*/

#ifndef __TYPE_DEFS__
#define __TYPE_DEFS__
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
#ifndef S_ISDIR
#define S_ISDIR(m) ((m)&S_IFDIR)
#endif
#elif defined (__ALPHA) && defined (VMS)                /* Alpha VMS */
#define t_int64 __int64
#elif defined (__ALPHA) && defined (__unix__)           /* Alpha UNIX */
#define t_int64 long
#else                                                   /* default GCC */
#define t_int64 long long
#endif                                                  /* end OS's */
typedef unsigned t_int64        t_uint64, t_value;      /* value */
typedef t_int64                 t_svalue;               /* signed value */
#ifdef USS
#define DOUBLE double
#else
#define DOUBLE t_uint64
#endif
#if defined(USS) || defined(SOLARIS) || defined(AIX) || defined(__s390__) 
#define ASM_BIG_ENDIAN
#endif
#endif /* __TYPE_DEFS__ */

/*
** OpCode table definition
*/

#define NUMOPS 204

#define MAX_INST_TYPES 21
enum optypes
{
   TYPE_1=0, TYPE_2,  TYPE_3,  TYPE_4,  TYPE_5,  TYPE_6,  TYPE_7,  TYPE_8,
   TYPE_9,   TYPE_10, TYPE_11, TYPE_12, TYPE_13, TYPE_14, TYPE_15, TYPE_16,
   TYPE_17,  TYPE_18, TYPE_19, TYPE_20, TYPE_21, TYPE_P
};

enum psops
{
   AORG_T=1, ASG_T,   ASMIF_T,  ASMELS_T, ASMEND_T, BES_T,    BSS_T,
   BYTE_T,   CALL_T,  CEND_T,   CKPT_T,   COPY_T,   CSEG_T,   DATA_T,
   DEF_T,    DEND_T,  DFOP_T,   DORG_T,   DOUBLE_T, DSEG_T,   DXOP_T,
   ELSE_T,   END_T,   ENDIF_T,  EQU_T,    EVEN_T,   EXIT_T,   FLOAT_T,
   GOTO_T,   IDT_T,   IF_T,     LDEF_T,   LIBIN_T,  LIBOUT_T, LIST_T,
   LOAD_T,   LONG_T,  LREF_T,   MACRO_T,  MEND_T,   NAME_T,   NOP_T,
   OPTION_T, PAGE_T,  PEND_T,   PSEG_T,   QUAD_T,   REF_T,    RORG_T,
   RT_T,     SBSTG_T, SETMNL_T, SETRM_T,  SREF_T,   TEXT_T,   TITL_T,
   UNL_T,    VAR_T,   WPNT_T,   XVEC_T
};

typedef struct
{
   char *opcode;	/* Opcode */
   unsigned opvalue;	/* Opcode value */
   unsigned opmod;	/* Opcode modifier */
   int optype;		/* Opcode type (format) */
   int cputype;		/* Supported Assembler & CPU */
} OpCode;

typedef struct Op_DefCode
{
   struct Op_DefCode *next;
   char opcode[MAXSYMLEN];/* Opcode */
   unsigned opvalue;	/* Opcode value */
   unsigned opmod;	/* Opcode modifier */
   int optype;		/* Opcode type (format) */
   int cputype;		/* Supported Assembler & CPU */
} OpDefCode;

/* cputype mask and mode bits */

#define CPUMASK 0x000F
#define SDSMAC  0x0100

/*
** Symbol table
*/

typedef struct Xref_Node
{
   struct Xref_Node *next;
   int line;
} XrefNode;

typedef struct
{
   XrefNode *xref_head;
   XrefNode *xref_tail;
   char symbol[MAXSYMLEN+2];
   int flags;
   int line;
   int value;
   int csegndx;
   int attribute;
   int extndx;
   int extcnt;
   int refcnt;
} SymNode;

#define RELOCATABLE 0x000000001
#define EXTERNAL    0x000000002
#define GLOBAL      0x000000004
#define LONGSYM     0x000000008
#define LOADSYM     0x000000010
#define SREFSYM     0x000000020
#define XOPSYM      0x000000040
#define PREDEFINE   0x000000080
#define DSEGSYM     0x000000100
#define DSEGEXTERN  0x000000200
#define CSEGSYM     0x000000400
#define CSEGEXTERN  0x000000800
#define P0SYM       0x000001000
#define P0MACNAME   0x000002000
#define EXTERNOUT   0x000004000

/*
** Macro definitions
*/

/* Macro expansion lines */

typedef struct Mac_Lines
{
   struct Mac_Lines *next;
   int ismodel;
   int nestndx;
   int ifindex;
   char label[MAXSYMLEN];
   char opcode[MAXSYMLEN];
   char operand[MAXLINE];
   char source[MAXLINE];
} MacLines;

/* Macro argument template definition */

typedef struct Mac_Arg
{
   struct Mac_Arg *ptr;
   char defsym[MAXSYMLEN];
   char usersym[MAXLINE];
   int length;
   int attribute;
   int uattribute;
   int value;
} MacArg;

/* Macro template definition */

typedef struct
{
   char macname[MAXSYMLEN];
   int macstartline;
   int macargcount;
   int macvarcount;
   int maclinecount;
   int maclinemunge;
   MacArg macargs[MAXMACARGS];
   MacLines *maclines[MAXMACROLINES];
} MacDef;


/*
** Macro variable attributes
*/

#define ATTR_PCALL	0x00000001
#define ATTR_PIND	0x00000002
#define ATTR_POPL	0x00000004
#define ATTR_PNDX	0x00000008
#define ATTR_PATO	0x00000010
#define ATTR_PSYM	0x00000020
#define ATTR_PLIT	0x00000040
#define ATTR_VAL	0x00000080
#define ATTR_UNDEF	0x00000100
#define ATTR_MAC	0x00000200
#define ATTR_RELO	0x00001000
#define ATTR_REF	0x00002000
#define ATTR_DEF	0x00004000
#define ATTR_STR	0x00008000

/*
** Macro variable qualifiers
*/

#define QUAL_A		0
#define QUAL_L		1
#define QUAL_S		2
#define QUAL_V		3
#define QUAL_NS		4
#define QUAL_SA		5
#define QUAL_SL		6
#define QUAL_SS		7
#define QUAL_SV		8
#define QUAL_SU		9
#define QUAL_ALL	10

/*
** Modes for processing
*/

#define GENINST   0x00000001
#define INSERT    0x00000004
#define SKPINST   0x00000010
#define MACDEF    0x00001000
#define MACEXP    0x00002000
#define MACCALL   0x00004000
#define NESTMASK  0x00FF0000
#define MACRMASK  0xFF000000
#define NESTSHFT  16
#define MACRSHFT  24

/*
** $IF/ASMIF nesting definitions.
*/

typedef struct
{
   int skip;
   int level;
} IFTable;

/*
** CSEG table definitions.
*/

typedef struct
{
   char name[MAXSYMLEN];
   int length;
   int number;
} CSEGNode;

/*
** Object tags
*/

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
#define CMNEXTERN_TAG	'X'
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
#define LCMNEXTERN_TAG	'x'
#define LCMNSREF_TAG	'z'

#define CHARSPERREC	66  /* Chars per object record */
#define CHARSPERCASREC	73  /* Chars per cassette object record */
#define WORDTAGLEN	5   /* Word + Object Tag length */
#define EXTRNLEN	11  /* Tag + SYMBOL + addr */
#define GLOBALLEN	11  /* Tag + SYMBOL + addr */
#define IDTLEN		13  /* Tag + IDT + length */
#define IDTCOLUMN       72  /* Where to put the IDT */
#define SEQUENCENUM	76  /* Where to put the sequence */

/*
** Pass 0/1 error table - pass 2 does the printing.
*/

#define MAXERRORS 1000

typedef struct
{
   char *errortext;
   int errorline;
   int errorfile;
} ErrTable;

/*
** COPY files data
*/

typedef struct
{
   FILE *fd;
   int line;
   char tmpnam[1024];
} COPYfile;

/*
** LIBIN/OUT files data
*/

typedef struct
{
   char dirname[MAXPATHNAMESIZE];
   int line;
} LIBfile;

/*
** Parser definitions
*/

#include "asm990.tok"

#define MEMREF	1
#define FLTNUM	2
#define MACSYM  SYM+1
#define IS_RELOP(v) (((v) >= OR) && ((v) < HEXNUM))

typedef uint8  tchar;  /* 0..255 */
typedef uint8  pstate; /* 0..255 */
typedef uint16 toktyp; /* 0..0x7F */
typedef uint16 tokval;

#define TOKENLEN  80 /* symbol and string length */
#define VALUEZERO 0

/*
** External definitions
*/

extern int if_start (int);
extern int if_else (void);
extern int if_end (void);

extern int detab (FILE *, FILE *, char *);
extern int asmpass0 (FILE *, FILE *);
extern int asmpass1 (FILE *);
extern int asmpass2 (FILE *, FILE *, int, FILE *);

extern OpCode *oplookup (char *, int);
extern void opadd (OpCode *, char *);
extern void opdel (char *);

extern int readsource (FILE *, int *, int *, int *, char *);
extern void writesource (FILE *, int, int, char *, char *);

extern char *tokscan (char *, char **, int *, int *, char *);
extern SymNode *symlookup (char *, int, int);
extern char *exprscan (char *, int *, char *, int *, int, int, int);
extern unsigned long cvtfloat (char *);
extern t_uint64 cvtdouble (char *);
extern DOUBLE ibm_strtod (const char *, char **);
extern void logerror (char *);
extern char *checkexpr (char *, int *);

extern FILE *opencopy (char *, FILE *);
extern FILE *closecopy (FILE *);
#ifdef WIN32
extern int mkstemp (char *);
#endif

extern void Parse_Error (int, int, int);
extern tokval Parser (char *, int *, int *, int *, int, int, int);
extern toktyp Scanner (char *, int *, tokval *, char *, char *, int);

extern int processoption (char *, int);

#if defined(USS) || defined(OS390)
extern unsigned char ebcasc[256];
#define TOASCII(a) ebcasc[(a)]
#else
#define TOASCII(a) (a)
#endif
