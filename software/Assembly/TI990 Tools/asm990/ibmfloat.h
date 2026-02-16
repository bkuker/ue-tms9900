
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

#ifndef USS

/*
** Exceptions codes
*/

#define PGM_EXPONENT_OVERFLOW_EXCEPTION 12
#define PGM_EXPONENT_UNDERFLOW_EXCEPTION 13
#define PGM_SIGNIFICANCE_EXCEPTION 14
#define PGM_FLOATING_POINT_DIVIDE_EXCEPTION 15

/*
** Condition codes
*/

#define CC_EQ 0
#define CC_LT 1
#define CC_GT 2

/*
** Masks
*/

#if defined(WIN32) && !defined(MINGW)
#define MASK01 0x00FFFFFFFFFFFFFFI64
#define MASK02 0x00FFFFFFFF000000I64
#define MASK03 0x00FFFF0000000000I64
#define MASK04 0x00FF000000000000I64
#define MASK05 0x00F0000000000000I64
#define MASK06 0x0F00000000000000I64
#define MASK07 0xF000000000000000I64
#define MASK08 0x0000F00000000000I64
#define MASK09 0x00000000FFFFFFFFI64
#define MASK10 0x8000000000000000I64
#define MASK11 0x0000000000000001I64
#define MASK12 0x0000000080000000I64
#define MASK13 0xFFFFFFFF00000000I64
#else
#define MASK01 0x00FFFFFFFFFFFFFFULL
#define MASK02 0x00FFFFFFFF000000ULL
#define MASK03 0x00FFFF0000000000ULL
#define MASK04 0x00FF000000000000ULL
#define MASK05 0x00F0000000000000ULL
#define MASK06 0x0F00000000000000ULL
#define MASK07 0xF000000000000000ULL
#define MASK08 0x0000F00000000000ULL
#define MASK09 0x00000000FFFFFFFFULL
#define MASK10 0x8000000000000000ULL
#define MASK11 0x0000000000000001ULL
#define MASK12 0x0000000080000000ULL
#define MASK13 0xFFFFFFFF00000000ULL
#endif

/*
** Functions
*/

extern int ibm_addl (t_uint64 *, t_uint64 *);
extern int ibm_cmpl (t_uint64 *, t_uint64 *);
extern int ibm_divl (t_uint64 *, t_uint64 *);
extern int ibm_fixl (t_uint64 *);
extern int ibm_fltl (t_uint64 *, int32);
extern int ibm_mpyl (t_uint64 *, t_uint64 *);
extern int ibm_negl (t_uint64 *);
extern int ibm_subl (t_uint64 *, t_uint64 *);

extern int ibm_adds (uint32 *, uint32 *);
extern int ibm_cmps (uint32 *, uint32 *);
extern int ibm_divs (uint32 *, uint32 *);
extern int ibm_fixs (uint32 *);
extern int ibm_flts (uint32 *, int32);
extern int ibm_mpys (uint32 *, uint32 *);
extern int ibm_negs (uint32 *);
extern int ibm_subs (uint32 *, uint32 *);

#endif /* USS */
