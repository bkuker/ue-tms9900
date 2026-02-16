/*-
 * Copyright (c) 1993
 *   The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *   This product includes software developed by the University of
 *   California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strtod.c   8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

/****************************************************************
 *
 * The author of this software is David M. Gay.
 *
 * Copyright (c) 1991 by AT&T.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose without fee is hereby granted, provided that this entire notice
 * is included in all copies of any software which is or includes a copy
 * or modification of this software and in all copies of the supporting
 * documentation for such software.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTY.  IN PARTICULAR, NEITHER THE AUTHOR NOR AT&T MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY
 * OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 ***************************************************************/

/* Please send bug reports to
   David M. Gay
   AT&T Bell Laboratories, Room 2C-463
   600 Mountain Avenue
   Murray Hill, NJ 07974-2070
   U.S.A.
   dmg@research.att.com or research!dmg
 */

/* strtod for IEEE-, VAX-, and IBM-arithmetic machines.
 *
 * This strtod returns a nearest machine number to the input decimal
 * string (or sets errno to ERANGE).  With IEEE arithmetic, ties are
 * broken by the IEEE round-even rule.  Otherwise ties are broken by
 * biased rounding (add half and chop).
 *
 * Inspired loosely by William D. Clinger's paper "How to Read Floating
 * Point Numbers Accurately" [Proc. ACM SIGPLAN '90, pp. 92-101].
 *
 * Modifications:
 *
 *   1. We only require IEEE, IBM, or VAX double-precision
 *      arithmetic (not IEEE double-extended).
 *   2. We get by with floating-point arithmetic in a case that
 *      Clinger missed -- when we're computing d * 10^n
 *      for a small integer d and the integer n is not too
 *      much larger than 22 (the maximum integer k for which
 *      we can represent 10^k exactly), we may be able to
 *      compute (d*10^k) * 10^(e-k) with just one roundoff.
 *   3. Rather than a bit-at-a-time adjustment of the binary
 *      result in the hard case, we use floating-point
 *      arithmetic to determine the adjustment to within
 *      one bit; only in really hard cases do we need to
 *      compute a second residual.
 *   4. Because of 3., we don't need a large table of powers of 10
 *      for ten-to-e (just some small tables, e.g. of 10^k
 *      for 0 <= k <= 22).
 */

/*
 * #define IEEE_8087 for IEEE-arithmetic machines where the least
 *   significant byte has the lowest address.
 * #define IEEE_MC68k for IEEE-arithmetic machines where the most
 *   significant byte has the lowest address.
 * #define Sudden_Underflow for IEEE-format machines without gradual
 *   underflow (i.e., that flush to zero on underflow).
 * #define IBM for IBM mainframe-style floating-point arithmetic.
 * #define VAX for VAX-style floating-point arithmetic.
 * #define Unsigned_Shifts if >> does treats its left operand as unsigned.
 * #define No_leftright to omit left-right logic in fast floating-point
 *   computation of dtoa.
 * #define Check_FLT_ROUNDS if FLT_ROUNDS can assume the values 2 or 3.
 * #define RND_PRODQUOT to use rnd_prod and rnd_quot (assembly routines
 *   that use extended-precision instructions to compute rounded
 *   products and quotients) with IBM.
 * #define ROUND_BIASED for IEEE-format with biased rounding.
 * #define Inaccurate_Divide for IEEE-format with correctly rounded
 *   products but inaccurate quotients, e.g., for Intel i860.
 * #define Just_16 to store 16 bits per 32-bit long when doing high-precision
 *   integer arithmetic.  Whether this speeds things up or slows things
 *   down depends on the machine and the number being converted.
 * #define KR_headers for old-style C function headers.
 * #define Bad_float_h if your system lacks a float.h or if it does not
 *   define some or all of DBL_DIG, DBL_MAX_10_EXP, DBL_MAX_EXP,
 *   FLT_RADIX, FLT_ROUNDS, and DBL_MAX.
 */

#define IBM
#define Sudden_Underflow

#ifdef DEBUG_STRTOD
#include <stdio.h>
#define Bug(x) {fprintf(stderr, "%s\n", x); exit(ABORT);}
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <math.h>

#include "ibmfloat.h"

#define DBL_DIG 16
#define DBL_MAX_10_EXP 75
#define DBL_MAX_EXP 63
#define FLT_RADIX 16
#define FLT_ROUNDS 0

#ifdef USS
#define DBL_MAX 7.2370055773322621e+75
#define ZERO 0.0
#define ONE  1.0
#define TWO  2.0
#define MONE -1.0
#define P5   0.5
#define P4999999 0.4999999
#define P5000001 0.5000001
#else
#if defined(WIN32) && !defined(MINGW)
#define DBL_MAX 0x7FFFFFFFFFFFFFFFI64
static DOUBLE ZERO = 0x000000000000000I64;
static DOUBLE ONE = 0x4110000000000000I64;
static DOUBLE TWO = 0x4120000000000000I64;
static DOUBLE MONE = 0xC110000000000000I64;
static DOUBLE P5 = 0x4080000000000000I64;
static DOUBLE P4999999 = 0x407FFFFE5280D654I64;
static DOUBLE P5000001 = 0x40800001AD7F29ACI64;
#else
#define DBL_MAX 0x7FFFFFFFFFFFFFFFULL
static DOUBLE ZERO = 0x0000000000000000ULL;
static DOUBLE ONE = 0x4110000000000000ULL;
static DOUBLE TWO = 0x4120000000000000ULL;
static DOUBLE MONE = 0xC110000000000000ULL;
static DOUBLE P5 = 0x4080000000000000ULL;
static DOUBLE P4999999 = 0x407FFFFE5280D654ULL;
static DOUBLE P5000001 = 0x40800001AD7F29ACULL;
#endif
#endif

#ifndef LONG_MAX
#define LONG_MAX 2147483647
#endif

#define CONST const

#ifdef Unsigned_Shifts
#define Sign_Extend(a,b) if (b < 0) a |= 0xffff0000;
#else
#define Sign_Extend(a,b)	/*no-op */
#endif

#ifdef ASM_BIG_ENDIAN
#define getword0(x) ((uint32 *)&x)[0]
#define getword1(x) ((uint32 *)&x)[1]
#define putword0(x,y) ((uint32 *)&x)[0]=(y)
#define putword1(x,y) ((uint32 *)&x)[1]=(y)
#else
#define getword0(x) ((uint32 *)&x)[1]
#define getword1(x) ((uint32 *)&x)[0]
#define putword0(x,y) ((uint32 *)&x)[1]=(y)
#define putword1(x,y) ((uint32 *)&x)[0]=(y)
#endif

/* The following definition of Storeinc is appropriate for MIPS processors.
 * An alternative that might be better on some machines is
 * #define Storeinc(a,b,c) (*a++ = b << 16 | c & 0xffff)
 */
#ifdef ASM_BIG_ENDIAN
#define Storeinc(a,b,c) (((unsigned short *)a)[0] = (unsigned short)b, \
((unsigned short *)a)[1] = (unsigned short)c, a++)
#else
#define Storeinc(a,b,c) (((unsigned short *)a)[1] = (unsigned short)b, \
((unsigned short *)a)[0] = (unsigned short)c, a++)
#endif

/* #define P DBL_MANT_DIG */
/* Ten_pmax = floor(P*log(2)/log(5)) */
/* Bletch = (highest power of 2 < DBL_MAX_10_EXP) / 16 */
/* Quick_max = floor((P-1)*log(FLT_RADIX)/log(10) - 1) */
/* Int_max = floor(P*log(FLT_RADIX)/log(10) - 1) */
#define Exp_shift  24
#define Exp_shift1 24
#define Exp_msk1   0x1000000
#define Exp_msk11  0x1000000
#define Exp_mask  0x7f000000
#define P 14
#define Emin (-75)
#define Bias 65
#define Exp_1  0x41000000
#define Exp_11 0x41000000
#define Ebits 8	/* exponent has 7 bits, but 8 is the right value in b2d */
#define Frac_mask  0xffffff
#define Frac_mask1 0xffffff
#define Bletch 4
#define Ten_pmax 22
#define Bndry_mask  0xefffff
#define Bndry_mask1 0xffffff
#define LSB 1
#define Sign_bit 0x80000000
#define Log2P 4
#define Tiny0 0x100000
#define Tiny1 0
#define Quick_max 14
#define Int_max 15
#define ROUND_BIASED
#ifdef RND_PRODQUOT
#define rounded_product(a,b) a = rnd_prod(a, b)
#define rounded_quotient(a,b) a = rnd_quot(a, b)
     extern DOUBLE rnd_prod (DOUBLE, DOUBLE),
           rnd_quot (DOUBLE, DOUBLE);
#else
#ifdef USS
#define rounded_product(a,b) a *= b
#define rounded_quotient(a,b) a /= b
#else
#define rounded_product(a,b) ibm_mpyl(&a,&b)
#define rounded_quotient(a,b) ibm_divl(&a,&b)
#endif
#endif
#ifdef USS
#define Big0 (Frac_mask1 | Exp_msk1*(DBL_MAX_EXP+Bias-1))
#else
#define Big0 0x7fffffff
#endif
#define Big1 0xffffffff

#ifndef Just_16
/* When Pack_32 is not defined, we store 16 bits per 32-bit long.
 * This makes some inner loops simpler and sometimes saves work
 * during multiplications, but it often seems to make things slightly
 * slower.  Hence the default is now to store 32 bits per long.
 */
#ifndef Pack_32
#define Pack_32
#endif
#endif
#define Kmax 15

struct Bigint
{
  struct Bigint *next;
  int   k,
	maxwds,
	sign,
	wds;
  uint32 x[1];
};

typedef struct Bigint Bigint;

static Bigint *freelist[Kmax + 1];

static Bigint *
Balloc (int k)
{
   int   x;
   Bigint *rv;

   rv = freelist[k];
   if (rv)
   {
      freelist[k] = rv->next;
   }
   else
   {
      x = 1 << k;
      rv = (Bigint *) malloc (sizeof (Bigint) + (x - 1) * sizeof (long));
      rv->k = k;
      rv->maxwds = x;
   }
   rv->sign = rv->wds = 0;
   return rv;
}

static void
Bfree (Bigint * v)
{
   if (v)
   {
      v->next = freelist[v->k];
      freelist[v->k] = v;
   }
}

#define Bcopy(x,y) memcpy((char *)&x->sign, (char *)&y->sign, \
y->wds*sizeof(long) + 2*sizeof(int))

static Bigint *
multadd (Bigint * b, int m, int a)	/* multiply by m and add a */
{
   int   i,
         wds;
   uint32 *x,
         y;
#ifdef Pack_32
   uint32 xi,
         z;
#endif
   Bigint *b1;

#ifdef DEBUG_STRTOD
   printf ("multadd: wds = %d, m = %d, a = %d\n", b->wds, m, a);
   printf ("   x = ");
   for (i = 0; i < b->wds; i++)
      printf ("%lx ", b->x[i]);
   printf ("\n");
#endif
   wds = b->wds;
   x = b->x;
   i = 0;
   do
   {
#ifdef Pack_32
      xi = *x;
      y = (xi & 0xffff) * m + a;
      z = (xi >> 16) * m + (y >> 16);
      a = (int) (z >> 16);
      *x++ = (z << 16) + (y & 0xffff);
#else
      y = *x * m + a;
      a = (int) (y >> 16);
      *x++ = y & 0xffff;
#endif
   }
   while (++i < wds);

   if (a)
   {
      if (wds >= b->maxwds)
      {
	 b1 = Balloc (b->k + 1);
	 Bcopy (b1, b);
	 Bfree (b);
	 b = b1;
      }
      b->x[wds++] = a;
      b->wds = wds;
   }
#ifdef DEBUG_STRTOD
   printf ("   mx = ");
   for (i = 0; i < b->wds; i++)
      printf ("%lx ", b->x[i]);
   printf ("\n");
#endif
   return b;
}

static Bigint *
s2b (CONST char *s, int nd0, int nd, uint32 y9)
{
   Bigint *b;
   int   i,
         k;
   int32 x,
         y;

#ifdef DEBUG_STRTOD
   printf ("s2b: s = %s, nd0 = %d, nd = %d, y9 = %d\n", s, nd0, nd, y9);
#endif
   x = (nd + 8) / 9;
   for (k = 0, y = 1; x > y; y <<= 1, k++);
#ifdef Pack_32
   b = Balloc (k);
   b->x[0] = y9;
   b->wds = 1;
#else
   b = Balloc (k + 1);
   b->x[0] = y9 & 0xffff;
   b->wds = (b->x[1] = y9 >> 16) ? 2 : 1;
#endif

   i = 9;
   if (9 < nd0)
   {
      s += 9;
      do
	 b = multadd (b, 10, *s++ - '0');
      while (++i < nd0);
      s++;
   }
   else
      s += 10;

   for (; i < nd; i++)
      b = multadd (b, 10, *s++ - '0');
#ifdef DEBUG_STRTOD
   printf ("   sx = ");
   for (i = 0; i < b->wds; i++)
      printf ("%lx ", b->x[i]);
   printf ("\n");
#endif
   return b;
}

static int
hi0bits (register uint32 x)
{
   register int k = 0;

#ifdef DEBUG_STRTOD
   printf ("hi0bits: x = >%08.8x\n", x);
#endif
   if    (!(x & 0xffff0000))
   {
      k = 16;
      x <<= 16;
   }
   if    (!(x & 0xff000000))
   {
      k += 8;
      x <<= 8;
   }
   if (!(x & 0xf0000000))
   {
      k += 4;
      x <<= 4;
   }
   if (!(x & 0xc0000000))
   {
      k += 2;
      x <<= 2;
   }
   if (!(x & 0x80000000))
   {
      k++;
      if (!(x & 0x40000000))
	 return 32;
   }
#ifdef DEBUG_STRTOD
   printf ("   k = %d\n", k);
#endif
   return k;
}

static int
lo0bits (uint32 *y)
{
   register int k;
   register uint32 x = *y;

#ifdef DEBUG_STRTOD
   printf ("lo0bits: x = %d\n", x);
#endif
   if (x & 7)
   {
      if (x & 1)
	 return 0;
      if (x & 2)
      {
	 *y = x >> 1;
	 return 1;
      }
      *y = x >> 2;

      return 2;
   }
   k = 0;
   if (!(x & 0xffff))
   {
      k = 16;
      x >>= 16;
   }
   if (!(x & 0xff))
   {
      k += 8;
      x >>= 8;
   }
   if (!(x & 0xf))
   {
      k += 4;
      x >>= 4;
   }
   if (!(x & 0x3))
   {
      k += 2;
      x >>= 2;
   }
   if (!(x & 1))
   {
      k++;
      x >>= 1;
      if ((!x) & 1)
	 return 32;
   }
   *y = x;
#ifdef DEBUG_STRTOD
   printf ("   k = %d\n", k);
#endif
   return k;
}

static Bigint *
i2b (int i)
{
   Bigint *b;

#ifdef DEBUG_STRTOD
   printf ("i2b: i = %d\n", i);
#endif
   b = Balloc (1);
   b->x[0] = i;
   b->wds = 1;
#ifdef DEBUG_STRTOD
   printf ("   b = ");
   for (i = 0; i < b->wds; i++)
      printf ("%lx ", b->x[i]);
   printf ("\n");
#endif
   return b;
}

static Bigint *
mult (Bigint * a, Bigint * b)
{
   Bigint *c;
   int   k,
         wa,
         wb,
         wc;
   uint32 carry,
         y,
         z;
   uint32 *x,
	 *xa,
	 *xae,
	 *xb,
	 *xbe,
	 *xc,
	 *xc0;

#ifdef Pack_32
   uint32 z2;
#endif

#ifdef DEBUG_STRTOD
   printf ("mult:\n");
   printf ("   a = ");
   for (k = 0; k < a->wds; k++)
      printf ("%lx ", a->x[k]);
   printf ("\n");
   printf ("   b = ");
   for (k = 0; k < b->wds; k++)
      printf ("%lx ", b->x[k]);
   printf ("\n");
#endif
   if (a->wds < b->wds)
   {
      c = a;
      a = b;
      b = c;
   }
   k = a->k;
   wa = a->wds;
   wb = b->wds;
   wc = wa + wb;
   if (wc > a->maxwds)
      k++;
   c = Balloc (k);
   for (x = c->x, xa = x + wc; x < xa; x++)
      *x = 0;
   xa = a->x;
   xae = xa + wa;
   xb = b->x;
   xbe = xb + wb;
   xc0 = c->x;
#ifdef Pack_32
   for (; xb < xbe; xb++, xc0++)
   {
      y = *xb & 0xffff;
      if (y)
      {
	 x = xa;
	 xc = xc0;
	 carry = 0;
	 do
	 {
	    z = (*x & 0xffff) * y + (*xc & 0xffff) + carry;
	    carry = z >> 16;
	    z2 = (*x++ >> 16) * y + (*xc >> 16) + carry;
	    carry = z2 >> 16;
	    Storeinc (xc, z2, z);
	 }
	 while (x < xae);
	 *xc = carry;
      }
      y = *xb >> 16;
      if (y)
      {
	 x = xa;
	 xc = xc0;
	 carry = 0;
	 z2 = *xc;
	 do
	 {
	    z = (*x & 0xffff) * y + (*xc >> 16) + carry;
	    carry = z >> 16;
	    Storeinc (xc, z, z2);
	    z2 = (*x++ >> 16) * y + (*xc & 0xffff) + carry;
	    carry = z2 >> 16;
	 }
	 while (x < xae);
	 *xc = z2;
      }
   }
#else
   for (; xb < xbe; xc0++)
   {
      if (y = *xb++)
      {
	 x = xa;
	 xc = xc0;
	 carry = 0;
	 do
	 {
	    z = *x++ * y + *xc + carry;
	    carry = z >> 16;
	    *xc++ = z & 0xffff;
	 }
	 while (x < xae);
	 *xc = carry;
      }
   }
#endif
   for (xc0 = c->x, xc = xc0 + wc; wc > 0 && !*--xc; --wc);
   c->wds = wc;
#ifdef DEBUG_STRTOD
   printf ("   c = ");
   for (k = 0; k < c->wds; k++)
      printf ("%lx ", c->x[k]);
   printf ("\n");
#endif
   return c;
}

static Bigint *p5s;

static Bigint *
pow5mult (Bigint * b, int k)
{
   Bigint *b1,
	 *p5,
	 *p51;
   int   i;
   static int p05[3] = {5, 25, 125};

#ifdef DEBUG_STRTOD
   printf ("pow5mult: k = %d\n", k);
   printf ("   b = ");
   for (i = 0; i < b->wds; i++)
      printf ("%lx ", b->x[i]);
   printf ("\n");
#endif
   i = k & 3;
   if (i)
      b = multadd (b, p05[i - 1], 0);

   if (!(k >>= 2))
      return b;

   if (!(p5 = p5s))
   {
      /*
       * first time 
       */
      p5 = p5s = i2b (625);
      p5->next = 0;
   }
   for (;;)
   {
      if (k & 1)
      {
	 b1 = mult (b, p5);
	 Bfree (b);
	 b = b1;
      }
      if (!(k >>= 1))
	 break;
      if (!(p51 = p5->next))
      {
	 p51 = p5->next = mult (p5, p5);
	 p51->next = 0;
      }
      p5 = p51;
   }
#ifdef DEBUG_STRTOD
   printf ("   rb = ");
   for (i = 0; i < b->wds; i++)
      printf ("%lx ", b->x[i]);
   printf ("\n");
#endif
   return b;
}

static Bigint *
lshift (Bigint * b, int k)
{
   int   i,
         k1,
         n,
         n1;
   Bigint *b1;
   uint32 *x,
	 *x1,
	 *xe,
         z;

#ifdef DEBUG_STRTOD
   printf ("lshift: k = %d\n", k);
   printf ("   b = ");
   for (i = 0; i < b->wds; i++)
      printf ("%lx ", b->x[i]);
   printf ("\n");
#endif
#ifdef Pack_32
   n = k >> 5;
#else
   n = k >> 4;
#endif
   k1 = b->k;
   n1 = n + b->wds + 1;
   for (i = b->maxwds; n1 > i; i <<= 1)
      k1++;
   b1 = Balloc (k1);
   x1 = b1->x;
   for (i = 0; i < n; i++)
      *x1++ = 0;
   x = b->x;
   xe = x + b->wds;
#ifdef Pack_32
   if (k &= 0x1f)
   {
      k1 = 32 - k;
      z = 0;
      do
      {
	 *x1++ = *x << k | z;
	 z = *x++ >> k1;
      }
      while (x < xe);

      *x1 = z;
      if (*x1)
	 ++n1;
   }
#else
   if (k &= 0xf)
   {
      k1 = 16 - k;
      z = 0;
      do
      {
	 *x1++ = *x << k & 0xffff | z;
	 z = *x++ >> k1;
      }
      while (x < xe);

      if (*x1 = z)
	 ++n1;
   }
#endif
   else
      do
	 *x1++ = *x++;
      while (x < xe);
   b1->wds = n1 - 1;
   Bfree (b);
#ifdef DEBUG_STRTOD
   printf ("   b1 = ");
   for (i = 0; i < b1->wds; i++)
      printf ("%lx ", b1->x[i]);
   printf ("\n");
#endif
   return b1;
}

static int
cmp (Bigint * a, Bigint * b)
{
   uint32 *xa,
	 *xa0,
	 *xb,
	 *xb0;
   int   i,
         j;

#ifdef DEBUG_STRTOD
   printf ("cmp: \n");
   printf ("   a = ");
   for (i = 0; i < a->wds; i++)
      printf ("%lx ", a->x[i]);
   printf ("\n");
   printf ("   b = ");
   for (i = 0; i < b->wds; i++)
      printf ("%lx ", b->x[i]);
   printf ("\n");
#endif
   i = a->wds;
   j = b->wds;
#ifdef DEBUG_STRTOD
   if (i > 1 && !a->x[i - 1])
      Bug ("cmp called with a->x[a->wds-1] == 0");
   if (j > 1 && !b->x[j - 1])
      Bug ("cmp called with b->x[b->wds-1] == 0");
#endif
   if (i -= j)
   {
#ifdef DEBUG_STRTOD
      printf ("   return %d\n", i);
#endif
      return i;
   }
   xa0 = a->x;
   xa = xa0 + j;
   xb0 = b->x;
   xb = xb0 + j;
   for (;;)
   {
      if (*--xa != *--xb)
      {
#ifdef DEBUG_STRTOD
	 printf ("   return %d\n", *xa < *xb ? -1 : 1);
#endif
	 return *xa < *xb ? -1 : 1;
      }
      if (xa <= xa0)
	 break;
   }
#ifdef DEBUG_STRTOD
   printf ("   return 0\n");
#endif
   return 0;
}

static Bigint *
diff (Bigint * a, Bigint * b)
{
   Bigint *c;
   int   i,
         wa,
         wb;
   int32 borrow,
         y;			/* We need signed shifts here. */
   uint32 *xa,
	 *xae,
	 *xb,
	 *xbe,
	 *xc;

#ifdef Pack_32
   int32 z;
#endif

#ifdef DEBUG_STRTOD
   printf ("diff: \n");
   printf ("   a = ");
   for (i = 0; i < a->wds; i++)
      printf ("%lx ", a->x[i]);
   printf ("\n");
   printf ("   b = ");
   for (i = 0; i < b->wds; i++)
      printf ("%lx ", b->x[i]);
   printf ("\n");
#endif
   i = cmp (a, b);
   if (!i)
   {
      c = Balloc (0);
      c->wds = 1;
      c->x[0] = 0;
#ifdef DEBUG_STRTOD
      printf ("   c = ");
      for (i = 0; i < c->wds; i++)
	 printf ("%lx ", c->x[i]);
      printf ("\n");
#endif
      return c;
   }
   if (i < 0)
   {
      c = a;
      a = b;
      b = c;
      i = 1;
   }
   else
      i = 0;
   c = Balloc (a->k);
   c->sign = i;
   wa = a->wds;
   xa = a->x;
   xae = xa + wa;
   wb = b->wds;
   xb = b->x;
   xbe = xb + wb;
   xc = c->x;
   borrow = 0;
#ifdef Pack_32
   do
   {
      y = (*xa & 0xffff) - (*xb & 0xffff) + borrow;
      borrow = y >> 16;
      Sign_Extend (borrow, y);
      z = (*xa++ >> 16) - (*xb++ >> 16) + borrow;
      borrow = z >> 16;
      Sign_Extend (borrow, z);
      Storeinc (xc, z, y);
   }
   while (xb < xbe);
   while (xa < xae)
   {
      y = (*xa & 0xffff) + borrow;
      borrow = y >> 16;
      Sign_Extend (borrow, y);
      z = (*xa++ >> 16) + borrow;
      borrow = z >> 16;
      Sign_Extend (borrow, z);
      Storeinc (xc, z, y);
   }
#else
   do
   {
      y = *xa++ - *xb++ + borrow;
      borrow = y >> 16;
      Sign_Extend (borrow, y);
      *xc++ = y & 0xffff;
   }
   while (xb < xbe);
   while (xa < xae)
   {
      y = *xa++ + borrow;
      borrow = y >> 16;
      Sign_Extend (borrow, y);
      *xc++ = y & 0xffff;
   }
#endif
   while (!*--xc)
      wa--;
   c->wds = wa;
#ifdef DEBUG_STRTOD
   printf ("   c = ");
   for (i = 0; i < c->wds; i++)
      printf ("%lx ", c->x[i]);
   printf ("\n");
#endif
   return c;
}

static DOUBLE
ulp (DOUBLE x)
{
   register int32 L;
   DOUBLE a;

#ifdef DEBUG_STRTOD
   union
   {
      DOUBLE xxd;
      uint32 xxl[2];
   } xx;
#endif

#ifdef DEBUG_STRTOD
   xx.xxd = x;
#ifdef ASM_BIG_ENDIAN
   printf ("ulp: x = >%08.8X%08.8X\n", xx.xxl[0], xx.xxl[1]);
#else
   printf ("ulp: x = >%08.8X%08.8X\n", xx.xxl[1], xx.xxl[0]);
#endif
#endif
   a = ZERO;
   L = (getword0 (x) & Exp_mask) - (P - 1) * Exp_msk1;
#ifndef Sudden_Underflow
   if (L > 0)
   {
#endif
      L |= Exp_msk1 >> 4;
      putword0 (a, L);
      putword1 (a, 0);

#ifndef Sudden_Underflow
   }
   else
   {
      L = -L >> Exp_shift;
      if (L < Exp_shift)
      {
	 putword0 (a, 0x80000 >> L);
	 putword1 (a, 0);
      }
      else
      {
	 putword0 (a, 0);
	 L -= Exp_shift;
	 putword1 (a, L >= 31 ? 1 : 1 << (31 - L));
      }
   }
#endif
#ifdef DEBUG_STRTOD
   xx.xxd = a;
#ifdef ASM_BIG_ENDIAN
   printf ("   a = >%08.8X%08.8X\n", a, xx.xxl[0], xx.xxl[1]);
#else
   printf ("   a = >%08.8X%08.8X\n", a, xx.xxl[1], xx.xxl[0]);
#endif
#endif
   return a;
}

static DOUBLE
b2d (Bigint * a, int *e)
{
   uint32 *xa,
	 *xa0,
         w,
         y,
         z;
   int   k;
   DOUBLE d;
#ifdef DEBUG_STRTOD
   union
   {
      DOUBLE xxd;
      uint32 xxl[2];
   } xx;
#endif

#ifdef DEBUG_STRTOD
   printf ("b2d: \n");
   printf ("   a = ");
   for (k = 0; k < a->wds; k++)
      printf ("%lx ", a->x[k]);
   printf ("\n");
#endif
   d = ZERO;
   xa0 = a->x;
   xa = xa0 + a->wds;
   y = *--xa;
#ifdef DEBUG_STRTOD
   if (!y)
      Bug ("zero y in b2d");
#endif
   k = hi0bits (y);
   *e = 32 - k;
#ifdef Pack_32
   if (k < Ebits)
   {
      putword0 (d, Exp_1 | y >> (Ebits - k));
      w = xa > xa0 ? *--xa : 0;
      putword1 (d, y << (32 - Ebits + k) | w >> (Ebits - k));
      goto ret_d;
   }
   z = xa > xa0 ? *--xa : 0;
   if (k -= Ebits)
   {
      putword0 (d, Exp_1 | y << k | z >> (32 - k));
      y = xa > xa0 ? *--xa : 0;
      putword1 (d, z << k | y >> (32 - k));
   }
   else
   {
      putword0 (d, Exp_1 | y);
      putword1 (d, z);
   }
#else
   if (k < Ebits + 16)
   {
      z = xa > xa0 ? *--xa : 0;
      putword0 (d, Exp_1 | y << (k - Ebits) | z >> (Ebits + 16 - k));
      w = xa > xa0 ? *--xa : 0;
      y = xa > xa0 ? *--xa : 0;
      putword1 (d, z << (k + 16 - Ebits) | w << (k - Ebits) |
		   y >> (16 + Ebits - k));
      goto ret_d;
   }
   z = xa > xa0 ? *--xa : 0;
   w = xa > xa0 ? *--xa : 0;
   k -= Ebits + 16;
   putword0 (d, Exp_1 | y << k + 16 | z << k | w >> 16 - k);
   y = xa > xa0 ? *--xa : 0;
   putword1 (d, w << k + 16 | y << k);
#endif
 ret_d:
#ifdef DEBUG_STRTOD
   printf ("   e = %d\n", *e);
   xx.xxd = d;
#ifdef ASM_BIG_ENDIAN
   printf ("   d = >%08.8X%08.8X\n", xx.xxl[0], xx.xxl[1]);
#else
   printf ("   d = >%08.8X%08.8X\n", xx.xxl[1], xx.xxl[0]);
#endif
#endif
   return d;
}

static Bigint *
d2b (DOUBLE d, int *e, int *bits)
{
   Bigint *b;
   int   de,
         i,
         k;
   uint32 *x,
         y,
         z;
#ifdef DEBUG_STRTOD
   union
   {
      DOUBLE xxd;
      uint32 xxl[2];
   } xx;
#endif

#ifdef DEBUG_STRTOD
   xx.xxd = d;
#ifdef ASM_BIG_ENDIAN
   printf ("d2b: d = >%08.8X%08.8X\n", xx.xxl[0], xx.xxl[1]);
#else
   printf ("d2b: d = >%08.8X%08.8X\n", xx.xxl[1], xx.xxl[0]);
#endif
#endif
#ifdef Pack_32
   b = Balloc (1);
#else
   b = Balloc (2);
#endif
   x = b->x;

   z = getword0 (d) & Frac_mask;
   putword0 (d, getword0 (d) & 0x7fffffff);/* clear sign bit, which we ignore */
   de = (int) (getword0 (d) >> Exp_shift);
#ifndef Sudden_Underflow
   if (de != 0)
      z |= Exp_msk1;
#endif
#ifdef Pack_32
   y = getword1 (d);
   if (y)
   {
      k = lo0bits (&y);
      if (k)
      {
	 x[0] = y | z << (32 - k);
	 z >>= k;
      }
      else
	 x[0] = y;
      i = b->wds = (x[1] = z) ? 2 : 1;
   }
   else
   {
#ifdef DEBUG_STRTOD
      if (!z)
	 Bug ("Zero passed to d2b");
#endif
      k = lo0bits (&z);
      x[0] = z;
      i = b->wds = 1;
      k += 32;
   }
#else
   if (y = getword1 (d))
   {
      if (k = lo0bits (&y))
      {
	 if (k >= 16)
	 {
	    x[0] = y | z << 32 - k & 0xffff;
	    x[1] = z >> k - 16 & 0xffff;
	    x[2] = z >> k;
	    i = 2;
	 }
	 else
	 {
	    x[0] = y & 0xffff;
	    x[1] = y >> 16 | z << 16 - k & 0xffff;
	    x[2] = z >> k & 0xffff;
	    x[3] = z >> k + 16;
	    i = 3;
	 }
      }
      else
      {
	 x[0] = y & 0xffff;
	 x[1] = y >> 16;
	 x[2] = z & 0xffff;
	 x[3] = z >> 16;
	 i = 3;
      }
   }
   else
   {
#ifdef DEBUG_STRTOD
      if (!z)
	 Bug ("Zero passed to d2b");
#endif
      k = lo0bits (&z);
      if (k >= 16)
      {
	 x[0] = z;
	 i = 0;
      }
      else
      {
	 x[0] = z & 0xffff;
	 x[1] = z >> 16;
	 i = 1;
      }
      k += 32;
   }
   while (!x[i])
      --i;
   b->wds = i + 1;
#endif
#ifndef Sudden_Underflow
   if (de)
   {
#endif
      *e = ((de - Bias - (P - 1)) << 2) + k;
      *bits = 4 * P + 8 - k - hi0bits (getword0 (d) & Frac_mask);
#ifndef Sudden_Underflow
   }
   else
   {
      *e = de - Bias - (P - 1) + 1 + k;
#ifdef Pack_32
      *bits = 32 * i - hi0bits (x[i - 1]);
#else
      *bits = (i + 2) * 16 - hi0bits (x[i]);
#endif
   }
#endif
#ifdef DEBUG_STRTOD
   printf ("   e = %d, bits = %d\n", *e, *bits);
   printf ("   b = ");
   for (i = 0; i < b->wds; i++)
      printf ("%lx ", b->x[i]);
   printf ("\n");
#endif
   return b;
}

static DOUBLE
ratio (Bigint * a, Bigint * b)
{
   DOUBLE da,
         db;
   DOUBLE dk;
   int   k,
         ka,
         kb;

#ifdef DEBUG_STRTOD
   union
   {
      DOUBLE xxd;
      uint32 xxl[2];
   } xx;
#endif

#ifdef DEBUG_STRTOD
   printf ("ratio: \n");
   printf ("   s = ");
   for (i = 0; i < a->wds; i++)
      printf ("%lx ", a->x[i]);
   printf ("\n");
   printf ("   b = ");
   for (i = 0; i < b->wds; i++)
      printf ("%lx ", b->x[i]);
   printf ("\n");
#endif
   da = b2d (a, &ka);
   db = b2d (b, &kb);
#ifdef Pack_32
   k = ka - kb + 32 * (a->wds - b->wds);
#else
   k = ka - kb + 16 * (a->wds - b->wds);
#endif
   if (k > 0)
   {
      putword0 (da, getword0 (da) + (k >> 2) * Exp_msk1);
      if (k &= 3)
      {
#ifdef USS
	 da *= 1 << k;
#else
	 ibm_fltl (&dk, 1 << k);
	 ibm_mpyl (&da, &dk);
#endif
      }
   }
   else
   {
      k = -k;
      putword0 (db, getword0 (db) + (k >> 2) * Exp_msk1);
      if (k &= 3)
      {
#ifdef USS
	 db *= 1 << k;
#else
	 ibm_fltl (&dk, 1 << k);
	 ibm_mpyl (&db, &dk);
#endif
      }
   }
#ifdef DEBUG_STRTOD
   xx.xxd = da;
#ifdef ASM_BIG_ENDIAN
   printf ("   da = >%08.8X%08.8X\n", xx.xxl[0], xx.xxl[1]);
#else
   printf ("   da = >%08.8X%08.8X\n", xx.xxl[1], xx.xxl[0]);
#endif
   xx.xxd = db;
#ifdef ASM_BIG_ENDIAN
   printf ("   db = >%08.8X%08.8X\n", xx.xxl[0], xx.xxl[1]);
#else
   printf ("   db = >%08.8X%08.8X\n", xx.xxl[1], xx.xxl[0]);
#endif
#ifdef USS
   dk = da / db;
#else
   dk = da;
   ibm_divl (&dk, &db);
#endif
   xx.xxd = dk;
#ifdef ASM_BIG_ENDIAN
   printf ("   dk = >%08.8X%08.8X\n", xx.xxl[0], xx.xxl[1]);
#else
   printf ("   dk = >%08.8X%08.8X\n", xx.xxl[1], xx.xxl[0]);
#endif
#endif
#ifdef USS
   return da / db;
#else
   ibm_divl (&da, &db);
   return da;
#endif
}

static DOUBLE tens[] = {
#ifdef USS
   1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
   1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
   1e20, 1e21, 1e22
#else
#if defined(WIN32) && !defined(MINGW)
   0x4110000000000000I64, 0x41A0000000000000I64, 0x4264000000000000I64,
   0x433E800000000000I64, 0x4427100000000000I64, 0x45186A0000000000I64,
   0x45F4240000000000I64, 0x4698968000000000I64, 0x475F5E1000000000I64,
   0x483B9ACA00000000I64, 0x492540BE40000000I64, 0x4A174876E8000000I64,
   0x4AE8D4A510000000I64, 0x4B9184E72A000000I64, 0x4C5AF3107A400000I64,
   0x4D38D7EA4C680000I64, 0x4E2386F26FC10000I64, 0x4F16345785D8A000I64,
   0x4FDE0B6B3A764000I64, 0x508AC7230489E800I64, 0x5156BC75E2D63100I64,
   0x523635C9ADC5DEA0I64, 0x5321E19E0C9BAB24I64
#else
   0x4110000000000000ULL, 0x41A0000000000000ULL, 0x4264000000000000ULL,
   0x433E800000000000ULL, 0x4427100000000000ULL, 0x45186A0000000000ULL,
   0x45F4240000000000ULL, 0x4698968000000000ULL, 0x475F5E1000000000ULL,
   0x483B9ACA00000000ULL, 0x492540BE40000000ULL, 0x4A174876E8000000ULL,
   0x4AE8D4A510000000ULL, 0x4B9184E72A000000ULL, 0x4C5AF3107A400000ULL,
   0x4D38D7EA4C680000ULL, 0x4E2386F26FC10000ULL, 0x4F16345785D8A000ULL,
   0x4FDE0B6B3A764000ULL, 0x508AC7230489E800ULL, 0x5156BC75E2D63100ULL,
   0x523635C9ADC5DEA0ULL, 0x5321E19E0C9BAB24ULL
#endif
#endif
};

#ifdef USS
static DOUBLE bigtens[] = {
   1e16, 1e32, 1e64
};
static DOUBLE tinytens[] = {
   1e-16, 1e-32, 1e-64
};
#else
#if defined(WIN32) && !defined(MINGW)
static DOUBLE bigtens[] = {
   0x4E2386F26FC10000I64, 0x5B4EE2D6D415B85BI64, 0x76184F03E93FF9F5I64
};
static DOUBLE tinytens[] = {
   0x33734ACA5F6226F1I64, 0x2633EC47AB514E65I64, 0x0BA87FEA27A539EAI64
};
#else
static DOUBLE bigtens[] = {
   0x4E2386F26FC10000ULL, 0x5B4EE2D6D415B85BULL, 0x76184F03E93FF9F5ULL
};
static DOUBLE tinytens[] = {
   0x33734ACA5F6226F1ULL, 0x2633EC47AB514E65ULL, 0x0BA87FEA27A539EAULL
};
#endif
#endif

#define n_bigtens 3

DOUBLE
ibm_strtod (CONST char *s00, char **se)
{
   int   bb2,
         bb5,
         bbe,
         bd2,
         bd5,
         bbbits,
         bs2,
         c,
         dsign,
         e,
         e1,
         esign,
         i,
         j,
         k,
         nd,
         nd0,
         nf,
         nz,
         nz0,
         sign;
   int   cc,
         cc1;
   CONST char *s,
	 *s0,
	 *s1;
   DOUBLE aadj,
         aadj1,
         adj,
         rv,
         rv0;
#ifndef USS
   DOUBLE xval,
         yval;
#ifndef Sudden_Underflow
   int   xint;
#endif
#endif
#ifdef DEBUG_STRTOD
   union
   {
      DOUBLE xxd;
      uint32 xxl[2];
   } xx;
#endif
   int32  L;
   uint32 y,
         z;
   Bigint *bb,
	 *bb1,
	 *bd,
	 *bd0,
	 *bs,
	 *delta;

#ifdef DEBUG_STRTOD
   printf ("ibm_strtod: entered: s00 = %s\n", s00);
#endif
   e = sign = nz0 = nz = 0;
   rv = ZERO;
   for (s = s00;; s++)
      switch (*s)
      {
      case '-':
	 sign = 1;
	 /*
	  * no break 
	  */
      case '+':
	 if (*++s)
	    goto break2;
	 /*
	  * no break 
	  */
      case 0:
	 s = s00;
	 goto ret;
      default:
	 if (isspace ((unsigned char) *s))
	    continue;
	 goto break2;
      }
 break2:
   if (*s == '0')
   {
      nz0 = 1;
      while (*++s == '0');
      if (!*s)
	 goto ret;
   }
   s0 = s;
   y = z = 0;
   for (nd = nf = 0; (c = *s) >= '0' && c <= '9'; nd++, s++)
      if (nd < 9)
	 y = 10 * y + c - '0';
      else if (nd < 16)
	 z = 10 * z + c - '0';
   nd0 = nd;
   if (c == '.')
   {
      c = *++s;
      if (!nd)
      {
	 for (; c == '0'; c = *++s)
	    nz++;
	 if (c > '0' && c <= '9')
	 {
	    s0 = s;
	    nf += nz;
	    nz = 0;
	    goto have_dig;
	 }
	 goto dig_done;
      }
      for (; c >= '0' && c <= '9'; c = *++s)
      {
       have_dig:
	 nz++;
	 if (c -= '0')
	 {
	    nf += nz;
	    for (i = 1; i < nz; i++)
	       if (nd++ < 9)
		  y *= 10;
	       else if (nd <= DBL_DIG + 1)
		  z *= 10;
	    if (nd++ < 9)
	       y = 10 * y + c;
	    else if (nd <= DBL_DIG + 1)
	       z = 10 * z + c;
	    nz = 0;
	 }
      }
   }
 dig_done:
   e = 0;
   if (c == 'e' || c == 'E')
   {
      if (!nd && !nz && !nz0)
      {
	 s = s00;
	 goto ret;
      }
      s00 = s;
      esign = 0;
      switch (c = *++s)
      {
      case '-':
	 esign = 1;
      case '+':
	 c = *++s;
      }
      if (c >= '0' && c <= '9')
      {
	 while (c == '0')
	    c = *++s;
	 if (c > '0' && c <= '9')
	 {
	    L = c - '0';
	    s1 = s;
	    while ((c = *++s) >= '0' && c <= '9')
	       L = 10 * L + c - '0';
	    if (s - s1 > 8 || L > 19999)
	       /*
	        * Avoid confusion from exponents
	        * * so large that e might overflow.
	        */
	       e = 19999;	/* safe for 16 bit ints */
	    else
	       e = (int) L;
	    if (esign)
	       e = -e;
	 }
	 else
	    e = 0;
      }
      else
	 s = s00;
   }
#ifdef DEBUG_STRTOD
   printf (" sign = %d\n", sign);
   printf (" nd  = %d:>%x\n", nd, nd);
   printf (" nd0 = %d:>%x\n", nd0, nd0);
   printf (" nf  = %d:>%x\n", nf, nf);
   printf (" nz  = %d:>%x\n", nz, nz);
   printf (" nz0 = %d:>%x\n", nz0, nz0);
   printf (" e   = %d:>%x\n", e, e);
   printf (" y   = %d:>%x\n", y, y);
   printf (" z   = %d:>%x\n", z, z);
#endif
   if (!nd)
   {
      if (!nz && !nz0)
	 s = s00;
      goto ret;
   }
   e1 = e -= nf;

   /*
    * Now we have nd0 digits, starting at s0, followed by a
    * * decimal point, followed by nd-nd0 digits.  The number we're
    * * after is the integer represented by those digits times
    * * 10**e 
    */

   if (!nd0)
      nd0 = nd;
   k = nd < DBL_DIG + 1 ? nd : DBL_DIG + 1;
#ifdef USS
   rv = y;
#else
   ibm_fltl (&rv, y);
#endif
#ifdef DEBUG_STRTOD
   printf (" e = %d, e1 = %d, k = %d\n", e, e1, k);
#endif
   if (k > 9)
   {
#ifdef DEBUG_STRTOD
      printf (" tens[%d] = >%llx\n", k - 9, tens[k - 9]);
#endif
#ifdef USS
      rv = tens[k - 9] * rv + z;
#else
      ibm_mpyl (&rv, &tens[k - 9]);
      ibm_fltl (&xval, z);
      ibm_addl (&rv, &xval);
#endif
   }
#ifdef DEBUG_STRTOD
   xx.xxd = rv;
#ifdef ASM_BIG_ENDIAN
   printf (" rv = >%08.8X%08.8X\n", xx.xxl[0], xx.xxl[1]);
#else
   printf (" rv = >%08.8X%08.8X\n", xx.xxl[1], xx.xxl[0]);
#endif
#endif
   if (nd <= DBL_DIG
#ifndef RND_PRODQUOT
       && FLT_ROUNDS == 1
#endif
      )
   {
      if (!e)
	 goto ret;
      if (e > 0)
      {
	 if (e <= Ten_pmax)
	 {
#ifdef DEBUG_STRTOD
	    printf (" rv*tens[%d]:tens = %llx\n", e, tens[e]);
#endif
#ifdef USS
	    /*
	     * rv = 
	     */ rounded_product (rv, tens[e]);
#else
	    ibm_mpyl (&rv, &tens[e]);
#endif
	    goto ret;
	 }
	 i = DBL_DIG - nd;
	 if (e <= Ten_pmax + i)
	 {
	    /*
	     * A fancier test would sometimes let us do
	     * * this for larger i values.
	     */
	    e -= i;
#ifdef DEBUG_STRTOD
	    printf (" rv*tens[%d]:tens = %llx\n", i, tens[i]);
#endif
#ifdef USS
	    rv *= tens[i];
#else
	    ibm_mpyl (&rv, &tens[i]);
#endif
#ifdef DEBUG_STRTOD
	    printf (" rv*tens[%d]:tens = %llx\n", e, tens[e]);
#endif
#ifdef USS
	    /*
	     * rv = 
	     */ rounded_product (rv, tens[e]);
#else
	    ibm_mpyl (&rv, &tens[e]);
#endif
	    goto ret;
	 }
      }
#ifndef Inaccurate_Divide
      else if (e >= -Ten_pmax)
      {
#ifdef DEBUG_STRTOD
	 printf (" rv/tens[%d]:tens = %llx\n", -e, tens[-e]);
#endif
#ifdef USS
	 /*
	  * rv = 
	  */ rounded_quotient (rv, tens[-e]);
#else
	 ibm_divl (&rv, &tens[-e]);
#endif
	 goto ret;
      }
#endif
   }
   e1 += nd - k;

   /*
    * Get starting approximation = rv * 10**e1 
    */

   if (e1 > 0)
   {
      i = e1 & 15;
      if (i)
#ifdef USS
	 rv *= tens[i];
#else
	 ibm_mpyl (&rv, &tens[i]);
#endif
      if (e1 &= ~15)
      {
	 if (e1 > DBL_MAX_10_EXP)
	 {
	  ovfl:
	    errno = ERANGE;
	    putword0 (rv, Big0);
	    putword1 (rv, Big1);
	    goto ret;
	 }
	 if (e1 >>= 4)
	 {
	    for (j = 0; e1 > 1; j++, e1 >>= 1)
	       if (e1 & 1)
#ifdef USS
		  rv *= bigtens[j];
#else
		  ibm_mpyl (&rv, &bigtens[j]);
#endif
	    /*
	     * The last multiplication could overflow. 
	     */
	    putword0 (rv, getword0 (rv) - P * Exp_msk1);
#ifdef USS
	    rv *= bigtens[j];
#else
	    ibm_mpyl (&rv, &bigtens[j]);
#endif
	    if ((z =
		 getword0 (rv) & Exp_mask) >
		Exp_msk1 * (DBL_MAX_EXP + Bias - P))
	       goto ovfl;
	    if (z > Exp_msk1 * (DBL_MAX_EXP + Bias - 1 - P))
	    {
	       /*
	        * set to largest number 
	        */
	       /*
	        * (Can't trust DBL_MAX) 
	        */
	       putword0 (rv, Big0);
	       putword1 (rv, Big1);
	    }
	    else
	       putword0 (rv, getword0 (rv) + P * Exp_msk1);
	 }
      }
   }
   else if (e1 < 0)
   {
      e1 = -e1;
      i = e1 & 15;
      if (i)
      {
#ifdef DEBUG_STRTOD
	 printf (" rv/tens[%d]:tens = %llx\n", i, tens[i]);
#endif
#ifdef USS
	 rv /= tens[i];
#else
	 ibm_divl (&rv, &tens[i]);
#endif
      }
      if (e1 &= ~15)
      {
	 e1 >>= 4;
	 for (j = 0; e1 > 1; j++, e1 >>= 1)
	    if (e1 & 1)
#ifdef USS
	       rv *= tinytens[j];
#else
	       ibm_mpyl (&rv, &tinytens[j]);
#endif
	 /*
	  * The last multiplication could underflow. 
	  */
	 rv0 = rv;
#ifdef USS
	 rv *= tinytens[j];
#else
	 ibm_mpyl (&rv, &tinytens[j]);
#endif
	 if (!rv)
	 {
#ifdef USS
	    rv = 2. * rv0;
	    rv *= tinytens[j];
#else
	    rv = rv0;
	    ibm_mpyl (&rv, &TWO);
	    ibm_mpyl (&rv, &tinytens[j]);
#endif
	    if (!rv)
	    {
	     undfl:
	       rv = ZERO;
	       errno = ERANGE;
	       goto ret;
	    }
	    putword0 (rv, Tiny0);
	    putword1 (rv, Tiny1);
	    /*
	     * The refinement below will clean
	     * * this approximation up.
	     */
	 }
      }
   }

   /*
    * Now the hard part -- adjusting rv to the correct value.
    */

   /*
    * Put digits into bd: true value = bd * 10^e 
    */

   bd0 = s2b (s0, nd0, nd, y);

   for (;;)
   {
      bd = Balloc (bd0->k);
      Bcopy (bd, bd0);
      bb = d2b (rv, &bbe, &bbbits);	/* rv = bb * 2^bbe */
      bs = i2b (1);

      if (e >= 0)
      {
	 bb2 = bb5 = 0;
	 bd2 = bd5 = e;
      }
      else
      {
	 bb2 = bb5 = -e;
	 bd2 = bd5 = 0;
      }
      if (bbe >= 0)
	 bb2 += bbe;
      else
	 bd2 -= bbe;
      bs2 = bb2;
#ifdef Sudden_Underflow
      j = 1 + 4 * P - 3 - bbbits + ((bbe + bbbits - 1) & 3);
#else
      i = bbe + bbbits - 1;	/* logb(rv) */
      if (i < Emin)		/* denormal */
	 j = bbe + (P - Emin);
      else
	 j = P + 1 - bbbits;
#endif
      bb2 += j;
      bd2 += j;
      i = bb2 < bd2 ? bb2 : bd2;
      if (i > bs2)
	 i = bs2;
      if (i > 0)
      {
	 bb2 -= i;
	 bd2 -= i;
	 bs2 -= i;
      }
      if (bb5 > 0)
      {
	 bs = pow5mult (bs, bb5);
	 bb1 = mult (bs, bb);
	 Bfree (bb);
	 bb = bb1;
      }
      if (bb2 > 0)
	 bb = lshift (bb, bb2);
      if (bd5 > 0)
	 bd = pow5mult (bd, bd5);
      if (bd2 > 0)
	 bd = lshift (bd, bd2);
      if (bs2 > 0)
	 bs = lshift (bs, bs2);
      delta = diff (bb, bd);
      dsign = delta->sign;
      delta->sign = 0;
      i = cmp (delta, bs);
      if (i < 0)
      {
	 /*
	  * Error is less than half an ulp -- check for
	  * * special case of mantissa a power of two.
	  */
	 if (dsign || getword1 (rv) || getword0 (rv) & Bndry_mask)
	    break;
	 delta = lshift (delta, Log2P);
	 if (cmp (delta, bs) > 0)
	    goto drop_down;
	 break;
      }
      if (i == 0)
      {
	 /*
	  * exactly half-way between 
	  */
	 if (dsign)
	 {
	    if ((getword0 (rv) & Bndry_mask1) == Bndry_mask1
		&& getword1 (rv) == 0xffffffff)
	    {
	       /*
	        * boundary case -- increment exponent
	        */
	       putword0 (rv, (getword0 (rv) & Exp_mask)
			 + (Exp_msk1 | (Exp_msk1 >> 4)));
	       putword1 (rv, 0);
	       break;
	    }
	 }
	 else if (!(getword0 (rv) & Bndry_mask) && !getword1 (rv))
	 {
	  drop_down:
	    /*
	     * boundary case -- decrement exponent 
	     */
#ifdef Sudden_Underflow
	    L = getword0 (rv) & Exp_mask;
	    if (L < Exp_msk1)
	       goto undfl;
	    L -= Exp_msk1;
#else
	    L = (getword0 (rv) & Exp_mask) - Exp_msk1;
#endif
	    putword0 (rv, L | Bndry_mask1);
	    putword1 (rv, 0xffffffff);
	    goto cont;
	 }
#ifndef ROUND_BIASED
	 if (!(getword1 (rv) & LSB))
	    break;
#endif
	 if (dsign)
	 {
#ifdef DEBUG_STRTOD
	    printf (" ulp-1\n");
#endif
#ifdef USS
	    rv += ulp (rv);
#else
	    xval = ulp (rv);
	    ibm_addl (&rv, &xval);
#endif
	 }
#ifndef ROUND_BIASED
	 else
	 {
#ifdef DEBUG_STRTOD
	    printf (" ulp-2\n");
#endif
#ifdef USS
	    rv -= ulp (rv);
#else
	    xval = ulp (rv);
	    ibm_subl (&rv, &xval);
#endif
#ifndef Sudden_Underflow
	    if (!rv)
	       goto undfl;
#endif
	 }
#endif
	 break;
      }
      aadj = ratio (delta, bs);
#ifdef USS
      if (aadj <= 2.)
#else
#ifdef DEBUG_STRTOD
      printf (" aadj <= 2.0\n");
#endif
      cc = ibm_cmpl (&aadj, &TWO);
      if (cc == CC_LT || c == CC_EQ)
#endif
      {
	 if (dsign)
	    aadj = aadj1 = ONE;
	 else if (getword1 (rv) || getword0 (rv) & Bndry_mask)
	 {
#ifndef Sudden_Underflow
	    if (getword1 (rv) == Tiny1 && !getword0 (rv))
	       goto undfl;
#endif
	    aadj = ONE;
	    aadj1 = MONE;
	 }
	 else
	 {
	    /*
	     * special case -- power of FLT_RADIX to be 
	     * rounded down... 
	     */

#ifdef USS
	    if (aadj < 2. / FLT_RADIX)
#else
	    xval = TWO;
	    ibm_fltl (&yval, FLT_RADIX);
	    ibm_divl (&xval, &yval);
#ifdef DEBUG_STRTOD
	    printf (" aadj < 2.0/FLT_RADIX\n");
#endif
	    if (ibm_cmpl (&aadj, &xval) == CC_LT)
#endif
	    {
#ifdef USS
	       aadj = 1. / FLT_RADIX;
#else
	       xval = ONE;
	       ibm_divl (&xval, &yval);
	       aadj = xval;
#endif
	    }
	    else
#ifdef USS
	       aadj *= 0.5;
#else
	       ibm_mpyl (&aadj, &P5);
#endif
#ifdef USS
	    aadj1 = -aadj;
#else
	    aadj1 = aadj;
	    ibm_negl (&aadj1);
#endif
	 }
      }
      else
      {
#ifdef USS
	 aadj *= 0.5;
#else
	 ibm_mpyl (&aadj, &P5);
#endif
#ifdef USS
	 aadj1 = dsign ? aadj : -aadj;
#else
	 aadj1 = aadj;
	 if (!dsign)
	    ibm_negl (&aadj1);
#endif
#ifdef Check_FLT_ROUNDS
	 switch (FLT_ROUNDS)
	 {
	 case 2:		/* towards +infinity */
#ifdef USS
	    aadj1 -= 0.5;
#else
	    ibm_subl (&aadj1, &P5);
#endif
	    break;
	 case 0:		/* towards 0 */
	 case 3:		/* towards -infinity */
#ifdef USS
	    aadj1 += 0.5;
#else
	    ibm_addl (&aadj1, &P5);
#endif
	 }
#else
	 if (FLT_ROUNDS == 0)
#ifdef USS
	    aadj1 += 0.5;
#else
	    ibm_addl (&aadj1, &P5);
#endif
#endif
      }
      y = getword0 (rv) & Exp_mask;

      /*
       * Check for overflow 
       */

      if (y == Exp_msk1 * (DBL_MAX_EXP + Bias - 1))
      {
	 rv0 = rv;
	 putword0 (rv, getword0 (rv) - P * Exp_msk1);
#ifdef DEBUG_STRTOD
	 printf (" ulp-3\n");
#endif
#ifdef USS
	 adj = aadj1 * ulp (rv);
	 rv += adj;
#else
	 xval = ulp (rv);
	 ibm_mpyl (&xval, &aadj1);
	 adj = xval;
	 ibm_addl (&rv, &adj);
#endif
	 if ((getword0 (rv) & Exp_mask) >= Exp_msk1 * (DBL_MAX_EXP + Bias - P))
	 {
	    if (getword0 (rv0) == Big0 && getword1 (rv0) == Big1)
	       goto ovfl;
	    putword0 (rv, Big0);
	    putword1 (rv, Big1);
	    goto cont;
	 }
	 else
	    putword0 (rv, getword0 (rv) + P * Exp_msk1);
      }
      else
      {
#ifdef Sudden_Underflow
	 if ((getword0 (rv) & Exp_mask) <= P * Exp_msk1)
	 {
	    rv0 = rv;
	    putword0 (rv, getword0 (rv) + P * Exp_msk1);
#ifdef DEBUG_STRTOD
	    printf (" ulp-4\n");
#endif
#ifdef USS
	    adj = aadj1 * ulp (rv);
	    rv += adj;
#else
	    xval = ulp (rv);
	    ibm_mpyl (&xval, &aadj1);
	    adj = xval;
	    ibm_addl (&rv, &adj);
#endif
	    if ((getword0 (rv) & Exp_mask) < P * Exp_msk1)
	    {
	       if (getword0 (rv0) == Tiny0 && getword1 (rv0) == Tiny1)
		  goto undfl;
	       putword0 (rv, Tiny0);
	       putword1 (rv, Tiny1);
	       goto cont;
	    }
	    else
	       putword0 (rv, getword0(rv) - P * Exp_msk1);
	 }
	 else
	 {
#ifdef DEBUG_STRTOD
	    printf (" ulp-5\n");
#endif
#ifdef USS
	    adj = aadj1 * ulp (rv);
	    rv += adj;
#else
	    xval = ulp (rv);
	    ibm_mpyl (&xval, &aadj1);
	    adj = xval;
	    ibm_addl (&rv, &adj);
#endif
	 }
#else
	 /*
	  * Compute adj so that the IEEE rounding rules will
	  * correctly round rv + adj in some half-way cases.
	  * If rv * ulp(rv) is denormalized (i.e.,
	  * y <= (P-1)*Exp_msk1), we must adjust aadj to avoid
	  * trouble from bits lost to denormalization;
	  * example: 1.2e-307 .
	  */
#ifdef USS
	 if (y <= (P - 1) * Exp_msk1 && aadj >= 1.)
#else
#ifdef DEBUG_STRTOD
	 printf (" aadj >= 1.0\n");
#endif
	 cc = ibm_cmpl (&aadj, &ONE);
	 if (y <= (P - 1) * Exp_msk1 && (cc == CC_GT || cc == CC_EQ))
#endif
	 {
#ifdef USS
	    aadj1 = (DOUBLE) (int) (aadj + 0.5);
#else
	    xval = aadj;
	    ibm_addl (&xval, &P5);
	    xint = ibm_fixl (&xval);
	    ibm_fltl (&aadj1, xint);
#endif
	    if (!dsign)
#ifdef USS
	       aadj1 = -aadj1;
#else
	       ibm_negl (&aadj1);
#endif
	 }
#ifdef DEBUG_STRTOD
	 printf (" ulp-6\n");
#endif
#ifdef USS
	 adj = aadj1 * ulp (rv);
	 rv += adj;
#else
	 xval = ulp (rv);
	 ibm_mpyl (&xval, &aadj1);
	 adj = xval;
	 ibm_addl (&rv, &adj);
#endif
#endif
      }
      z = getword0 (rv) & Exp_mask;
      if (y == z)
      {
	 /*
	  * Can we stop now? 
	  */
#ifdef USS
	 L = aadj;
	 aadj -= L;
#else
	 L = ibm_fixl (&aadj);
	 ibm_fltl (&xval, L);
	 ibm_subl (&aadj, &xval);

	 xval = P4999999;
	 ibm_fltl (&yval, FLT_RADIX);
	 ibm_divl (&xval, &yval);
#endif
	 /*
	  * The tolerances below are conservative. 
	  */
	 if (dsign || getword1 (rv) || getword0 (rv) & Bndry_mask)
	 {
#ifdef DEBUG_STRTOD
	    printf (" aadj < .499999 || aadj > .5000001\n");
#endif
#ifdef USS
	    if (aadj < .4999999 || aadj > .5000001)
#else
	    cc = ibm_cmpl (&aadj, &P4999999);
	    cc1 = ibm_cmpl (&aadj, &P5000001);
#ifdef DEBUG_STRTOD
	    printf (" cc = %d == %d || cc1 = %d == %d\n",
		    cc, CC_LT, cc1, CC_GT);
#endif
	    if (cc == CC_LT || cc1 == CC_GT)
#endif
	       break;
	 }
	 else
	 {
#ifdef USS
	    if (aadj < .4999999 / FLT_RADIX)
#else
#ifdef DEBUG_STRTOD
	    printf (" aadj < .4999999/FLT_RADIX\n");
#endif
	    if (ibm_cmpl (&aadj, &xval) == CC_LT)
#endif
	       break;
	 }
      }
    cont:
      Bfree (bb);
      Bfree (bd);
      Bfree (bs);
      Bfree (delta);
   }
   Bfree (bb);
   Bfree (bd);
   Bfree (bs);
   Bfree (bd0);
   Bfree (delta);
 ret:
   if (se)
      *se = (char *) s;
#ifdef DEBUG_STRTOD
   xx.xxd = rv;
#ifdef ASM_BIG_ENDIAN
   printf (" ret rv = >%08.8X%08.8X\n", xx.xxl[0], xx.xxl[1]);
#else
   printf (" ret rv = >%08.8X%08.8X\n", xx.xxl[1], xx.xxl[0]);
#endif
#endif
#ifdef USS
   rv = sign ? -rv : rv;
#else
   rv = sign ? (rv | ((t_uint64) Sign_bit << 32)) : rv;
#endif
   return rv;
}
