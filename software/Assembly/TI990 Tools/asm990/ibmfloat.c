#ifndef USS
/* IBMFLOAT.C   (c) Copyright Peter Kuschnerus, 2000-2003            */
/*              ESA/390 Hex Floatingpoint Instructions               */
/*              Borrowed from Hercules for TI990                     */

/*-------------------------------------------------------------------*/
/* This module implements the ESA/390 Hex Floatingpoint Instructions */
/* described in the manual ESA/390 Principles of Operation.          */
/*-------------------------------------------------------------------*/

#include <stdlib.h>
#include <math.h>
#include "ibmfloat.h"

/*-------------------------------------------------------------------*/
/* Definitions                                                       */
/*-------------------------------------------------------------------*/
#define POS     0                       /* Positive value of sign    */
#define NEG     1                       /* Negative value of sign    */
#define UNNORMAL 0                      /* Without normalisation     */
#define NORMAL  1                       /* With normalisation        */
#define OVUNF   1                       /* Check for over/underflow  */
#define NOOVUNF 0                       /* Leave exponent as is (for
                                           multiply-add/subtrace)    */
#define SIGEX   1                       /* Significance exception if
                                           result fraction is zero   */
#define NOSIGEX 0                       /* Do not raise significance
                                           exception, use true zero  */

/*-------------------------------------------------------------------*/
/* Structure definition for internal short floatingpoint format      */
/*-------------------------------------------------------------------*/
typedef struct _SHORT_FLOAT {
        uint32  short_fract;            /* Fraction                  */
        int16   expo;                   /* Exponent + 64             */
        uint8   sign;                   /* Sign                      */
} SHORT_FLOAT;


/*-------------------------------------------------------------------*/
/* Structure definition for internal long floatingpoint format       */
/*-------------------------------------------------------------------*/
typedef struct _LONG_FLOAT {
        t_uint64 long_fract;            /* Fraction                  */
        int16    expo;                  /* Exponent + 64             */
        uint8    sign;                  /* Sign                      */
} LONG_FLOAT;


#ifdef DEBUG_FLOAT
static void
ibm_printl (LONG_FLOAT *fl, char *what)
{
   printf ("   %s = %llX\n", 
      what,
      ((t_uint64)fl->sign << 63)|((t_uint64)fl->expo << 56)|(fl->long_fract));
}
static void
ibm_prints (SHORT_FLOAT *fl, char *what)
{
   printf ("   %s = %lX\n", 
      what,
      ((uint32)fl->sign << 31)|((uint32)fl->expo << 24)|(fl->short_fract));
}
#endif

/*-------------------------------------------------------------------*/
/* Get short float from register                                     */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format to be converted to             */
/*      fpr     Pointer to register to be converted from             */
/*-------------------------------------------------------------------*/
static void get_sf( SHORT_FLOAT *fl, uint32 *fpr )
{
    fl->sign = (uint8)(*fpr >> 31);
    fl->expo = (int16)((*fpr >> 24) & 0x007F);
    fl->short_fract = *fpr & 0x00FFFFFF;

} /* end function get_sf */


/*-------------------------------------------------------------------*/
/* Store short float to register                                     */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format to be converted from           */
/*      fpr     Pointer to register to be converted to               */
/*-------------------------------------------------------------------*/
static void store_sf( SHORT_FLOAT *fl, uint32 *fpr )
{
    *fpr = ((uint32)fl->sign << 31)
         | ((uint32)fl->expo << 24)
         | (fl->short_fract);

} /* end function store_sf */


/*-------------------------------------------------------------------*/
/* Get long float from register                                      */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format to be converted to             */
/*      fpr     Pointer to register to be converted from             */
/*-------------------------------------------------------------------*/
static void get_lf( LONG_FLOAT *fl, t_uint64 *fpr )
{
    fl->sign = (uint8)(*fpr >> 63);
    fl->expo = (int16)((*fpr >> 56) & 0x007F);
    fl->long_fract = *fpr & MASK01;

} /* end function get_lf */


/*-------------------------------------------------------------------*/
/* Store long float to register                                      */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float format to be converted from           */
/*      fpr     Pointer to register to be converted to               */
/*-------------------------------------------------------------------*/
static void store_lf( LONG_FLOAT *fl, t_uint64 *fpr )
{
    *fpr   = ((t_uint64)fl->sign << 63)
           | ((t_uint64)fl->expo << 56)
           | fl->long_fract;

} /* end function store_lf */


/*-------------------------------------------------------------------*/
/* Normalize short float                                             */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*-------------------------------------------------------------------*/
static void normal_sf( SHORT_FLOAT *fl )
{
    if (fl->short_fract) {
        if ((fl->short_fract & 0x00FFFF00) == 0) {
            fl->short_fract <<= 16;
            fl->expo -= 4;
        }
        if ((fl->short_fract & 0x00FF0000) == 0) {
            fl->short_fract <<= 8;
            fl->expo -= 2;
        }
        if ((fl->short_fract & 0x00F00000) == 0) {
            fl->short_fract <<= 4;
            (fl->expo)--;
        }
    } else {
        fl->sign = POS;
        fl->expo = 0;
    }

} /* end function normal_sf */


/*-------------------------------------------------------------------*/
/* Normalize long float                                              */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*-------------------------------------------------------------------*/
static void normal_lf( LONG_FLOAT *fl )
{
    if (fl->long_fract) {
        if ((fl->long_fract & MASK02) == 0) {
            fl->long_fract <<= 32;
            fl->expo -= 8;
        }
        if ((fl->long_fract & MASK03) == 0) {
            fl->long_fract <<= 16;
            fl->expo -= 4;
        }
        if ((fl->long_fract & MASK04) == 0) {
            fl->long_fract <<= 8;
            fl->expo -= 2;
        }
        if ((fl->long_fract & MASK05) == 0) {
            fl->long_fract <<= 4;
            (fl->expo)--;
        }
    } else {
        fl->sign = POS;
        fl->expo = 0;
    }

} /* end function normal_lf */


/*-------------------------------------------------------------------*/
/* Overflow of short float                                           */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int overflow_sf( SHORT_FLOAT *fl )
{
    if (fl->expo > 127) {
        fl->expo &= 0x007F;
        return(PGM_EXPONENT_OVERFLOW_EXCEPTION);
    }
    return(0);

} /* end function overflow_sf */


/*-------------------------------------------------------------------*/
/* Overflow of long float                                            */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int overflow_lf( LONG_FLOAT *fl )
{
    if (fl->expo > 127) {
        fl->expo &= 0x007F;
        return(PGM_EXPONENT_OVERFLOW_EXCEPTION);
    }
    return(0);

} /* end function overflow_lf */


/*-------------------------------------------------------------------*/
/* Underflow of short float                                          */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int underflow_sf( SHORT_FLOAT *fl )
{
    if (fl->expo < 0) {
        if (fl->expo < 127) {
            fl->expo &= 0x007F;
            return(PGM_EXPONENT_UNDERFLOW_EXCEPTION);
        } else {
            /* set true 0 */

            fl->short_fract = 0;
            fl->expo = 0;
            fl->sign = POS;
        }
    }
    return(0);

} /* end function underflow_sf */


/*-------------------------------------------------------------------*/
/* Underflow of long float                                           */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int underflow_lf( LONG_FLOAT *fl )
{
    if (fl->expo < 0) {
        if (fl->expo < 127) {
            fl->expo &= 0x007F;
            return(PGM_EXPONENT_UNDERFLOW_EXCEPTION);
        } else {
            /* set true 0 */

            fl->long_fract = 0;
            fl->expo = 0;
            fl->sign = POS;
        }
    }
    return(0);

} /* end function underflow_lf */


/*-------------------------------------------------------------------*/
/* Overflow and underflow of short float                             */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int over_under_flow_sf( SHORT_FLOAT *fl )
{
    if (fl->expo > 127) {
        fl->expo &= 0x007F;
        return(PGM_EXPONENT_OVERFLOW_EXCEPTION);
    } else {
        if (fl->expo < 0) {
	    if (fl->expo < 127) {
                fl->expo &= 0x007F;
                return(PGM_EXPONENT_UNDERFLOW_EXCEPTION);
            } else {
                /* set true 0 */

                fl->short_fract = 0;
                fl->expo = 0;
                fl->sign = POS;
            }
        }
    }
    return(0);

} /* end function over_under_flow_sf */


/*-------------------------------------------------------------------*/
/* Overflow and underflow of long float                              */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int over_under_flow_lf( LONG_FLOAT *fl )
{
    if (fl->expo > 127) {
        fl->expo &= 0x007F;
        return(PGM_EXPONENT_OVERFLOW_EXCEPTION);
    } else {
        if (fl->expo < 0) {
	    if (fl->expo < 127) {
                fl->expo &= 0x007F;
                return(PGM_EXPONENT_UNDERFLOW_EXCEPTION);
            } else {
                /* set true 0 */

                fl->long_fract = 0;
                fl->expo = 0;
                fl->sign = POS;
            }
        }
    }
    return(0);

} /* end function over_under_flow_lf */


/*-------------------------------------------------------------------*/
/* Significance of short float                                       */
/* The fraction is expected to be zero                               */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*      sigex   Allow significance exception if true                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int significance_sf( SHORT_FLOAT *fl, uint8 sigex )
{
    fl->sign = POS;
    if (sigex) {
        return(PGM_SIGNIFICANCE_EXCEPTION);
    }
    /* set true 0 */

    fl->expo = 0;
    return(0);

} /* end function significance_sf */


/*-------------------------------------------------------------------*/
/* Significance of long float                                        */
/* The fraction is expected to be zero                               */
/*                                                                   */
/* Input:                                                            */
/*      fl      Internal float                                       */
/*      sigex   Allow significance exception if true                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int significance_lf( LONG_FLOAT *fl, uint8 sigex )
{
    fl->sign = POS;
    if (sigex) {
        return(PGM_SIGNIFICANCE_EXCEPTION);
    }
    /* set true 0 */

    fl->expo = 0;
    return(0);

} /* end function significance_lf */


/*-------------------------------------------------------------------*/
/* Add short float                                                   */
/*                                                                   */
/* Input:                                                            */
/*      fl      Float                                                */
/*      add_fl  Float to be added                                    */
/*      normal  Normalize if true                                    */
/*      sigex   Allow significance exception if true                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int add_sf( SHORT_FLOAT *fl, SHORT_FLOAT *add_fl,
    uint8 normal, uint8 sigex )
{
int     pgm_check;
uint8    shift;

    pgm_check = 0;
    if (add_fl->short_fract
    || add_fl->expo) {          /* add_fl not 0 */
        if (fl->short_fract
        || fl->expo) {          /* fl not 0 */
            /* both not 0 */

            if (fl->expo == add_fl->expo) {
                /* expo equal */

                /* both guard digits */
                fl->short_fract <<= 4;
                add_fl->short_fract <<= 4;
            } else {
                /* expo not equal, denormalize */

                if (fl->expo < add_fl->expo) {
                    /* shift minus guard digit */
                    shift = add_fl->expo - fl->expo - 1;
                    fl->expo = add_fl->expo;

                    if (shift) {
                        if (shift >= 6
                        || ((fl->short_fract >>= (shift * 4)) == 0)) {
                            /* 0, copy summand */

                            fl->sign = add_fl->sign;
                            fl->short_fract = add_fl->short_fract;

                            if (fl->short_fract == 0) {
                                pgm_check = significance_sf(fl, sigex);
                            } else {
                                if (normal == NORMAL) {
                                    normal_sf(fl);
                                    pgm_check = underflow_sf(fl);
                                }
                            }
                            return(pgm_check);
                        }
                    }
                    /* guard digit */
                    add_fl->short_fract <<= 4;
                } else {
                    /* shift minus guard digit */
                    shift = fl->expo - add_fl->expo - 1;

                    if (shift) {
                        if (shift >= 6
                        || ((add_fl->short_fract >>= (shift * 4)) == 0)) {
                            /* 0, nothing to add */

                            if (fl->short_fract == 0) {
                                pgm_check = significance_sf(fl, sigex);
                            } else {
                                if (normal == NORMAL) {
                                    normal_sf(fl);
                                    pgm_check = underflow_sf(fl);
                                }
                            }
                            return(pgm_check);
                        }
                    }
                    /* guard digit */
                    fl->short_fract <<= 4;
                }
            }

            /* compute with guard digit */
            if (fl->sign == add_fl->sign) {
                fl->short_fract += add_fl->short_fract;
            } else {
                if (fl->short_fract == add_fl->short_fract) {
                    /* true 0 */

                    fl->short_fract = 0;
                    return( significance_sf(fl, sigex) );

                } else if (fl->short_fract > add_fl->short_fract) {
                    fl->short_fract -= add_fl->short_fract;
                } else {
                    fl->short_fract = add_fl->short_fract - fl->short_fract;
                    fl->sign = add_fl->sign;
                }
            }

            /* handle overflow with guard digit */
            if (fl->short_fract & 0xF0000000) {
                fl->short_fract >>= 8;
                (fl->expo)++;
                pgm_check = overflow_sf(fl);
            } else {

                if (normal == NORMAL) {
                    /* normalize with guard digit */
                    if (fl->short_fract) {
                        /* not 0 */

                        if (fl->short_fract & 0x0F000000) {
                            /* not normalize, just guard digit */
                            fl->short_fract >>= 4;
                        } else {
                            (fl->expo)--;
                            normal_sf(fl);
                            pgm_check = underflow_sf(fl);
                        }
                    } else {
                        /* true 0 */

                        pgm_check = significance_sf(fl, sigex);
                    }
                } else {
                    /* not normalize, just guard digit */
                    fl->short_fract >>= 4;
                    if (fl->short_fract == 0) {
                        pgm_check = significance_sf(fl, sigex);
                    }
                }
            }
            return(pgm_check);
        } else { /* fl 0, add_fl not 0 */
            /* copy summand */

            fl->expo = add_fl->expo;
            fl->sign = add_fl->sign;
            fl->short_fract = add_fl->short_fract;
            if (fl->short_fract == 0) {
                return( significance_sf(fl, sigex) );
            }
        }
    } else {                        /* add_fl 0 */
        if (fl->short_fract == 0) { /* fl 0 */
            /* both 0 */

            return( significance_sf(fl, sigex) );
        }
    }
    if (normal == NORMAL) {
        normal_sf(fl);
        pgm_check = underflow_sf(fl);
    }
    return(pgm_check);

} /* end function add_sf */


/*-------------------------------------------------------------------*/
/* Add long float                                                    */
/*                                                                   */
/* Input:                                                            */
/*      fl      Float                                                */
/*      add_fl  Float to be added                                    */
/*      normal  Normalize if true                                    */
/*      sigex   Allow significance exception if true                 */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int add_lf( LONG_FLOAT *fl, LONG_FLOAT *add_fl,
    uint8 normal, uint8 sigex )
{
int     pgm_check;
uint8    shift;

    pgm_check = 0;
    if (add_fl->long_fract
    || add_fl->expo) {          /* add_fl not 0 */
        if (fl->long_fract
        || fl->expo) {          /* fl not 0 */
            /* both not 0 */

            if (fl->expo == add_fl->expo) {
                /* expo equal */

                /* both guard digits */
                fl->long_fract <<= 4;
                add_fl->long_fract <<= 4;
            } else {
                /* expo not equal, denormalize */

                if (fl->expo < add_fl->expo) {
                    /* shift minus guard digit */
                    shift = add_fl->expo - fl->expo - 1;
                    fl->expo = add_fl->expo;

                    if (shift) {
                        if (shift >= 14
                        || ((fl->long_fract >>= (shift * 4)) == 0)) {
                            /* 0, copy summand */

                            fl->sign = add_fl->sign;
                            fl->long_fract = add_fl->long_fract;

                            if (fl->long_fract == 0) {
                                pgm_check = significance_lf(fl, sigex);
                            } else {
                                if (normal == NORMAL) {
                                    normal_lf(fl);
                                    pgm_check = underflow_lf(fl);
                                }
                            }
                            return(pgm_check);
                        }
                    }
                    /* guard digit */
                    add_fl->long_fract <<= 4;
                } else {
                    /* shift minus guard digit */
                    shift = fl->expo - add_fl->expo - 1;

                    if (shift) {
                        if (shift >= 14
                        || ((add_fl->long_fract >>= (shift * 4)) == 0)) {
                            /* 0, nothing to add */

                            if (fl->long_fract == 0) {
                                pgm_check = significance_lf(fl, sigex);
                            } else {
                                if (normal == NORMAL) {
                                    normal_lf(fl);
                                    pgm_check = underflow_lf(fl);
                                }
                            }
                            return(pgm_check);
                        }
                    }
                    /* guard digit */
                    fl->long_fract <<= 4;
                }
            }

            /* compute with guard digit */
            if (fl->sign == add_fl->sign) {
                fl->long_fract += add_fl->long_fract;
            } else {
                if (fl->long_fract == add_fl->long_fract) {
                    /* true 0 */

                    fl->long_fract = 0;
                    return( significance_lf(fl, sigex) );

                } else if (fl->long_fract > add_fl->long_fract) {
                    fl->long_fract -= add_fl->long_fract;
                } else {
                    fl->long_fract = add_fl->long_fract - fl->long_fract;
                    fl->sign = add_fl->sign;
                }
            }

            /* handle overflow with guard digit */
            if (fl->long_fract & MASK07) {
                fl->long_fract >>= 8;
                (fl->expo)++;
                pgm_check = overflow_lf(fl);
            } else {

                if (normal == NORMAL) {
                    /* normalize with guard digit */
                    if (fl->long_fract) {
                        /* not 0 */

                        if (fl->long_fract & MASK06) {
                            /* not normalize, just guard digit */
                            fl->long_fract >>= 4;
                        } else {
                            (fl->expo)--;
                            normal_lf(fl);
                            pgm_check = underflow_lf(fl);
                        }
                    } else {
                        /* true 0 */

                        pgm_check = significance_lf(fl, sigex);
                    }
                } else {
                    /* not normalize, just guard digit */
                    fl->long_fract >>= 4;
                    if (fl->long_fract == 0) {
                        pgm_check = significance_lf(fl, sigex);
                    }
                }
            }
            return(pgm_check);
        } else { /* fl 0, add_fl not 0 */
            /* copy summand */

            fl->expo = add_fl->expo;
            fl->sign = add_fl->sign;
            fl->long_fract = add_fl->long_fract;
            if (fl->long_fract == 0) {
                return( significance_lf(fl, sigex) );
            }
        }
    } else {                       /* add_fl 0 */
        if (fl->long_fract == 0) { /* fl 0 */
            /* both 0 */

            return( significance_lf(fl, sigex) );
        }
    }
    if (normal == NORMAL) {
        normal_lf(fl);
        pgm_check = underflow_lf(fl);
    }
    return(pgm_check);

} /* end function add_lf */


/*-------------------------------------------------------------------*/
/* Compare short float                                               */
/*                                                                   */
/* Input:                                                            */
/*      fl      Float                                                */
/*      cmp_fl  Float to be compared                                 */
/*      cc      CPU condition code                                   */
/*-------------------------------------------------------------------*/
static void cmp_sf( SHORT_FLOAT *fl, SHORT_FLOAT *cmp_fl, int *cc )
{
uint8    shift;

    if (cmp_fl->short_fract
    || cmp_fl->expo) {          /* cmp_fl not 0 */
        if (fl->short_fract
        || fl->expo) {          /* fl not 0 */
            /* both not 0 */

            if (fl->expo == cmp_fl->expo) {
                /* expo equal */

                /* both guard digits */
                fl->short_fract <<= 4;
                cmp_fl->short_fract <<= 4;
            } else {
                /* expo not equal, denormalize */

                if (fl->expo < cmp_fl->expo) {
                    /* shift minus guard digit */
                    shift = cmp_fl->expo - fl->expo - 1;

                    if (shift) {
                        if (shift >= 6
                        || ((fl->short_fract >>= (shift * 4)) == 0)) {
                            /* Set condition code */
                            if (cmp_fl->short_fract) {
                                *cc = cmp_fl->sign ? 2 : 1;
                            } else {
                                *cc = 0;
                            }
                            return;
                        }
                    }
                    /* guard digit */
                    cmp_fl->short_fract <<= 4;
                } else {
                    /* shift minus guard digit */
                    shift = fl->expo - cmp_fl->expo - 1;

                    if (shift) {
                        if (shift >= 6
                        || ((cmp_fl->short_fract >>= (shift * 4)) == 0)) {
                            /* Set condition code */
                            if (fl->short_fract) {
                                *cc = fl->sign ? 1 : 2;
                            } else {
                                *cc = 0;
                            }
                            return;
                        }
                    }
                    /* guard digit */
                    fl->short_fract <<= 4;
                }
            }

            /* compute with guard digit */
            if (fl->sign != cmp_fl->sign) {
                fl->short_fract += cmp_fl->short_fract;
            } else if (fl->short_fract >= cmp_fl->short_fract) {
                fl->short_fract -= cmp_fl->short_fract;
            } else {
                fl->short_fract = cmp_fl->short_fract - fl->short_fract;
                fl->sign = ! (cmp_fl->sign);
            }

            /* handle overflow with guard digit */
            if (fl->short_fract & 0xF0000000) {
                fl->short_fract >>= 4;
            }

            /* Set condition code */
            if (fl->short_fract) {
                *cc = fl->sign ? 1 : 2;
            } else {
                *cc = 0;
            }
            return;
        } else { /* fl 0, cmp_fl not 0 */
            /* Set condition code */
            if (cmp_fl->short_fract) {
                *cc = cmp_fl->sign ? 2 : 1;
            } else {
                *cc = 0;
            }
            return;
        }
    } else {                        /* cmp_fl 0 */
        /* Set condition code */
        if (fl->short_fract) {
            *cc = fl->sign ? 1 : 2;
        } else {
            *cc = 0;
        }
        return;
    }

} /* end function cmp_sf */


/*-------------------------------------------------------------------*/
/* Compare long float                                                */
/*                                                                   */
/* Input:                                                            */
/*      fl      Float                                                */
/*      cmp_fl  Float to be compared                                 */
/*      cc      CPU condition code                                   */
/*-------------------------------------------------------------------*/
static void cmp_lf( LONG_FLOAT *fl, LONG_FLOAT *cmp_fl, int *cc )
{
uint8    shift;

    if (cmp_fl->long_fract
    || cmp_fl->expo) {          /* cmp_fl not 0 */
        if (fl->long_fract
        || fl->expo) {          /* fl not 0 */
            /* both not 0 */

            if (fl->expo == cmp_fl->expo) {
                /* expo equal */

                /* both guard digits */
                fl->long_fract <<= 4;
                cmp_fl->long_fract <<= 4;
            } else {
                /* expo not equal, denormalize */

                if (fl->expo < cmp_fl->expo) {
                    /* shift minus guard digit */
                    shift = cmp_fl->expo - fl->expo - 1;

                    if (shift) {
                        if (shift >= 14
                        || ((fl->long_fract >>= (shift * 4)) == 0)) {
                            /* Set condition code */
                            if (cmp_fl->long_fract) {
                                *cc = cmp_fl->sign ? 2 : 1;
                            } else {
                                *cc = 0;
                            }
                            return;
                        }
                    }
                    /* guard digit */
                    cmp_fl->long_fract <<= 4;
                } else {
                    /* shift minus guard digit */
                    shift = fl->expo - cmp_fl->expo - 1;

                    if (shift) {
                        if (shift >= 14
                        || ((cmp_fl->long_fract >>= (shift * 4)) == 0)) {
                            /* Set condition code */
                            if (fl->long_fract) {
                                *cc = fl->sign ? 1 : 2;
                            } else {
                                *cc = 0;
                            }
                            return;
                        }
                    }
                    /* guard digit */
                    fl->long_fract <<= 4;
                }
            }

            /* compute with guard digit */
            if (fl->sign != cmp_fl->sign) {
                fl->long_fract += cmp_fl->long_fract;
            } else if (fl->long_fract >= cmp_fl->long_fract) {
                fl->long_fract -= cmp_fl->long_fract;
            } else {
                fl->long_fract = cmp_fl->long_fract - fl->long_fract;
                fl->sign = ! (cmp_fl->sign);
            }

            /* handle overflow with guard digit */
            if (fl->long_fract & 0xF0000000) {
                fl->long_fract >>= 4;
            }

            /* Set condition code */
            if (fl->long_fract) {
                *cc = fl->sign ? 1 : 2;
            } else {
                *cc = 0;
            }
            return;
        } else { /* fl 0, cmp_fl not 0 */
            /* Set condition code */
            if (cmp_fl->long_fract) {
                *cc = cmp_fl->sign ? 2 : 1;
            } else {
                *cc = 0;
            }
            return;
        }
    } else {                        /* cmp_fl 0 */
        /* Set condition code */
        if (fl->long_fract) {
            *cc = fl->sign ? 1 : 2;
        } else {
            *cc = 0;
        }
        return;
    }

} /* end function cmp_lf */


/*-------------------------------------------------------------------*/
/* Multiply short float                                              */
/*                                                                   */
/* Input:                                                            */
/*      fl      Multiplicand short float                             */
/*      mul_fl  Multiplicator short float                            */
/*      ovunf   Handle overflow/underflow if true                    */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int mul_sf( SHORT_FLOAT *fl, SHORT_FLOAT *mul_fl,
    uint8 ovunf )
{
t_uint64     wk;

    if (fl->short_fract
    && mul_fl->short_fract) {
        /* normalize operands */
        normal_sf( fl );
        normal_sf( mul_fl );

        /* multiply fracts */
        wk = (t_uint64) fl->short_fract * mul_fl->short_fract;

        /* normalize result and compute expo */
        if (wk & MASK08) {
            fl->short_fract = (uint32)(wk >> 24);
            fl->expo = fl->expo + mul_fl->expo - 64;
        } else {
            fl->short_fract = (uint32)(wk >> 20);
            fl->expo = fl->expo + mul_fl->expo - 65;
        }

        /* determine sign */
        fl->sign = (fl->sign == mul_fl->sign) ? POS : NEG;

        /* handle overflow and underflow if required */
        if (ovunf == OVUNF)
            return( over_under_flow_sf(fl) );

        /* otherwise leave exponent as is */
        return(0);
    } else {
        /* set true 0 */

        fl->short_fract = 0;
        fl->expo = 0;
        fl->sign = POS;
        return(0);
    }

} /* end function mul_sf */

/*-------------------------------------------------------------------*/
/* Multiply long float                                               */
/*                                                                   */
/* Input:                                                            */
/*      fl      Multiplicand long float                              */
/*      mul_fl  Multiplicator long float                             */
/*      ovunf   Handle overflow/underflow if true                    */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int mul_lf( LONG_FLOAT *fl, LONG_FLOAT *mul_fl,
    uint8 ovunf )
{
t_uint64     wk;
uint32     v;

    if (fl->long_fract
    && mul_fl->long_fract) {
        /* normalize operands */
        normal_lf( fl );
        normal_lf( mul_fl );

        /* multiply fracts by sum of partial multiplications */
        wk = ((fl->long_fract & MASK09) * (mul_fl->long_fract & MASK09)) >> 32;

        wk += ((fl->long_fract & MASK09) * (mul_fl->long_fract >> 32));
        wk += ((fl->long_fract >> 32) * (mul_fl->long_fract & MASK09));
        v = (uint32)wk;

        fl->long_fract = (wk >> 32) + ((fl->long_fract >> 32) *
	      (mul_fl->long_fract >> 32));

        /* normalize result and compute expo */
        if (fl->long_fract & MASK08) {
            fl->long_fract = (fl->long_fract << 8)
                           | (v >> 24);
            fl->expo = fl->expo + mul_fl->expo - 64;
        } else {
            fl->long_fract = (fl->long_fract << 12)
                           | (v >> 20);
            fl->expo = fl->expo + mul_fl->expo - 65;
        }

        /* determine sign */
        fl->sign = (fl->sign == mul_fl->sign) ? POS : NEG;

        /* handle overflow and underflow if required */
        if (ovunf == OVUNF)
            return( over_under_flow_lf(fl) );

        /* otherwise leave exponent as is */
        return(0);
    } else {
        /* set true 0 */

        fl->long_fract = 0;
        fl->expo = 0;
        fl->sign = POS;
        return(0);
    }

} /* end function mul_lf */


/*-------------------------------------------------------------------*/
/* Divide short float                                                */
/*                                                                   */
/* Input:                                                            */
/*      fl      Dividend short float                                 */
/*      div_fl  Divisor short float                                  */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int div_sf( SHORT_FLOAT *fl, SHORT_FLOAT *div_fl )
{
t_uint64     wk;

    if (div_fl->short_fract) {
        if (fl->short_fract) {
            /* normalize operands */
            normal_sf( fl );
            normal_sf( div_fl );

            /* position fracts and compute expo */
            if (fl->short_fract < div_fl->short_fract) {
                wk = (t_uint64) fl->short_fract << 24;
                fl->expo = fl->expo - div_fl->expo + 64;
            } else {
                wk = (t_uint64) fl->short_fract << 20;
                fl->expo = fl->expo - div_fl->expo + 65;
            }
            /* divide fractions */
            fl->short_fract = (uint32)(wk / div_fl->short_fract);

            /* determine sign */
            fl->sign = (fl->sign == div_fl->sign) ? POS : NEG;

            /* handle overflow and underflow */
            return( over_under_flow_sf(fl) );
        } else {
            /* fraction of dividend 0, set true 0 */

            fl->short_fract = 0;
            fl->expo = 0;
            fl->sign = POS;
        }
    } else {
        /* divisor 0 */

        return(PGM_FLOATING_POINT_DIVIDE_EXCEPTION);
    }
    return(0);

} /* end function div_sf */


/*-------------------------------------------------------------------*/
/* Divide long float                                                 */
/*                                                                   */
/* Input:                                                            */
/*      fl      Dividend long float                                  */
/*      div_fl  Divisor long float                                   */
/* Value:                                                            */
/*              exeption                                             */
/*-------------------------------------------------------------------*/
static int div_lf( LONG_FLOAT *fl, LONG_FLOAT *div_fl )
{
t_uint64     wk;
t_uint64     wk2;
int     i;

    if (div_fl->long_fract) {
        if (fl->long_fract) {
            /* normalize operands */
            normal_lf( fl );
            normal_lf( div_fl );

            /* position fracts and compute expo */
            if (fl->long_fract < div_fl->long_fract) {
                fl->expo = fl->expo - div_fl->expo + 64;
            } else {
                fl->expo = fl->expo - div_fl->expo + 65;
                div_fl->long_fract <<= 4;
            }

            /* partial divide first hex digit */
            wk2 = fl->long_fract / div_fl->long_fract;
            wk = (fl->long_fract % div_fl->long_fract) << 4;

            /* partial divide middle hex digits */
            i = 13;
            while (i--) {
                wk2 = (wk2 << 4)
                    | (wk / div_fl->long_fract);
                wk = (wk % div_fl->long_fract) << 4;
            }

            /* partial divide last hex digit */
            fl->long_fract = (wk2 << 4)
                           | (wk / div_fl->long_fract);

            /* determine sign */
            fl->sign = (fl->sign == div_fl->sign) ? POS : NEG;

            /* handle overflow and underflow */
            return( over_under_flow_lf(fl) );
        } else {
            /* fraction of dividend 0, set true 0 */

            fl->long_fract = 0;
            fl->expo = 0;
            fl->sign = POS;
        }
    } else {
        /* divisor 0 */

        return(PGM_FLOATING_POINT_DIVIDE_EXCEPTION);
    }
    return(0);

} /* end function div_lf */

int
ibm_addl (t_uint64 *m0, t_uint64 *m1)
{
   int retcode;
   LONG_FLOAT im0, im1;

   get_lf (&im0, m0);
   get_lf (&im1, m1);
#ifdef DEBUG_FLOAT
   printf ("ibm_addl: \n");
   ibm_printl (&im0, "m0");
   ibm_printl (&im1, "m1");
#endif

   retcode = add_lf (&im0, &im1, NORMAL, NOSIGEX);
#ifdef DEBUG_FLOAT
   printf ("   retcode = %d\n", retcode);
   ibm_printl (&im0, "sum");
#endif
   if (retcode == 0)
      store_lf (&im0, m0);
   return (retcode);
}

int
ibm_cmpl (t_uint64 *m0, t_uint64 *m1)
{
   int cc;
   LONG_FLOAT im0, im1;

   get_lf (&im0, m0);
   get_lf (&im1, m1);
#ifdef DEBUG_FLOAT
   printf ("ibm_cmpl: \n");
   ibm_printl (&im0, "m0");
   ibm_printl (&im1, "m1");
#endif
   cmp_lf (&im0, &im1, &cc);
#ifdef DEBUG_FLOAT
   printf ("   cc = %d\n", cc);
#endif
   return (cc);
}

int
ibm_divl (t_uint64 *m0, t_uint64 *m1)
{
   int retcode;
   LONG_FLOAT im0, im1;

   get_lf (&im0, m0);
   get_lf (&im1, m1);
#ifdef DEBUG_FLOAT
   printf ("ibm_divl: \n");
   ibm_printl (&im0, "m0");
   ibm_printl (&im1, "m1");
#endif
   retcode = div_lf (&im0, &im1);
#ifdef DEBUG_FLOAT
   printf ("   retcode = %d\n", retcode);
   ibm_printl (&im0, "quo");
#endif
   if (retcode == 0)
      store_lf (&im0, m0);
   return (retcode);
}

int
ibm_fixl (t_uint64 *m0)
{
LONG_FLOAT fl;
int m3 = 1;
int retval;
uint8      shift;
t_uint64   lsfract;

    get_lf(&fl, m0);
#ifdef DEBUG_FLOAT
   printf ("ibm_fixl: \n");
   ibm_printl (&fl, "fl");
#endif

    if (fl.long_fract) {
        /* not zero */
        normal_lf(&fl);

        if (fl.expo > 72) {
            /* exeeds range by exponent */
            retval = fl.sign ? 0x80000000UL : 0x7FFFFFFFUL;
            /*regs->psw.cc = 3;*/
            return(retval);
        }
        if (fl.expo > 64) {
            /* to be right shifted and to be rounded */
            shift = ((78 - fl.expo) * 4);
            lsfract = fl.long_fract << (64 - shift);
            fl.long_fract >>= shift;

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & MASK10) {
                    fl.long_fract++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if ((lsfract > MASK10)
                || ((fl.long_fract & MASK11)
                    && (lsfract == MASK10))) {
                    fl.long_fract++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && lsfract) {
                    fl.long_fract++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && lsfract) {
                    fl.long_fract++;
                }
            }
            if (fl.expo == 72) {
                if (fl.sign) {
                    /* negative */
                    if (fl.long_fract > 0x80000000UL) {
                        /* exeeds range by value */
                        retval = 0x80000000UL;
                        /*regs->psw.cc = 3;*/
                        return(retval);
                    }
                } else {
                    /* positive */
                    if (fl.long_fract > 0x7FFFFFFFUL) {
                        /* exeeds range by value */
                        retval = 0x7FFFFFFFUL;
                        /*regs->psw.cc = 3;*/
                        return(retval);
                    }
                }
            }
        } else if (fl.expo == 64) {
            /* to be rounded */
            lsfract = fl.long_fract << 8;
            fl.long_fract = 0;

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & MASK10) {
                    fl.long_fract++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if (lsfract > MASK10) {
                    fl.long_fract++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && lsfract) {
                    fl.long_fract++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && lsfract) {
                    fl.long_fract++;
                }
            }
        } else {
            /* fl.expo < 64 */
            fl.long_fract = 0;
            if (((m3 == 6)
                && (fl.sign == POS))
            || ((m3 == 7)
                && (fl.sign == NEG))) {
                fl.long_fract++;
            }
        }
        if (fl.sign) {
            /* negative */
            retval = -((int) fl.long_fract);
            /*regs->psw.cc = 1;*/
        } else {
            /* positive */
            retval = (int)fl.long_fract;
            /*regs->psw.cc = 2*/;
        }
    } else {
        /* zero */
        retval = 0;
        /*regs->psw.cc = 0;*/
    }
#ifdef DEBUG_FLOAT
     printf ("   retval = %d\n", retval);
#endif
    return (retval);
}

int
ibm_fltl (t_uint64 *m0, int32 c)
{
LONG_FLOAT fl;
t_int64    fix;

#ifdef DEBUG_FLOAT
   printf ("ibm_fltl: \n");
   printf ("   c = %d\n", c);
#endif

    /* get fixed value */
    fix = c;
    if (fix &  MASK12)
        fix |= MASK13;

    if (fix) {
        if (fix < 0) {
            fl.sign = NEG;
            fl.long_fract = (-fix);
        } else {
            fl.sign = POS;
            fl.long_fract = fix;
        }
        fl.expo = 78;

        /* Normalize result */
        normal_lf(&fl);

#ifdef DEBUG_FLOAT
	ibm_printl (&fl, "flt");
#endif
        /* To register */
        store_lf(&fl, m0);
    } else {
        /* true zero */
        *m0 = 0;
    }
   return (0);
}


int
ibm_mpyl (t_uint64 *m0, t_uint64 *m1)
{
   int retcode;
   LONG_FLOAT im0, im1;

   get_lf (&im0, m0);
   get_lf (&im1, m1);
#ifdef DEBUG_FLOAT
   printf ("ibm_mpyl: \n");
   ibm_printl (&im0, "m0");
   ibm_printl (&im1, "m1");
#endif
   retcode = mul_lf (&im0, &im1, NOOVUNF);
#ifdef DEBUG_FLOAT
   printf ("   retcode = %d\n", retcode);
   ibm_printl (&im0, "prod");
#endif
   if (retcode == 0)
      store_lf (&im0, m0);
   return (retcode);
}

int
ibm_negl (t_uint64 *m0)
{
   LONG_FLOAT im0;

   get_lf (&im0, m0);
#ifdef DEBUG_FLOAT
   printf ("ibm_negl: \n");
   ibm_printl (&im0, "m0");
#endif
   im0.sign = ! (im0.sign);
   store_lf (&im0, m0);
#ifdef DEBUG_FLOAT
   ibm_printl (&im0, "neg");
#endif
   return (0);
}

int
ibm_subl (t_uint64 *m0, t_uint64 *m1)
{
   int retcode;
   LONG_FLOAT im0, im1;

   get_lf (&im0, m0);
   get_lf (&im1, m1);
#ifdef DEBUG_FLOAT
   printf ("ibm_subl: \n");
   ibm_printl (&im0, "m0");
   ibm_printl (&im1, "m1");
#endif

   /* Invert the sign of 2nd operand */
   im1.sign = ! (im1.sign);

   retcode = add_lf (&im0, &im1, NORMAL, NOSIGEX);
#ifdef DEBUG_FLOAT
   printf ("   retcode = %d\n", retcode);
   ibm_printl (&im0, "sum");
#endif
   if (retcode == 0)
      store_lf (&im0, m0);
   return (retcode);
}

int
ibm_adds (uint32 *m0, uint32 *m1)
{
   int retcode;
   SHORT_FLOAT im0, im1;

   get_sf (&im0, m0);
   get_sf (&im1, m1);
#ifdef DEBUG_FLOAT
   printf ("ibm_adds: \n");
   ibm_prints (&im0, "m0");
   ibm_prints (&im1, "m1");
#endif

   retcode = add_sf (&im0, &im1, NORMAL, NOSIGEX);
#ifdef DEBUG_FLOAT
   printf ("   retcode = %d\n", retcode);
   ibm_prints (&im0, "sum");
#endif
   if (retcode == 0)
      store_sf (&im0, m0);
   return (retcode);
}

int
ibm_cmps (uint32 *m0, uint32 *m1)
{
   int cc;
   SHORT_FLOAT im0, im1;

   get_sf (&im0, m0);
   get_sf (&im1, m1);
#ifdef DEBUG_FLOAT
   printf ("ibm_cmps: \n");
   ibm_prints (&im0, "m0");
   ibm_prints (&im1, "m1");
#endif
   cmp_sf (&im0, &im1, &cc);
#ifdef DEBUG_FLOAT
   printf ("   cc = %d\n", cc);
#endif
   return (cc);
}

int
ibm_divs (uint32 *m0, uint32 *m1)
{
   int retcode;
   SHORT_FLOAT im0, im1;

   get_sf (&im0, m0);
   get_sf (&im1, m1);
#ifdef DEBUG_FLOAT
   printf ("ibm_divs: \n");
   ibm_prints (&im0, "m0");
   ibm_prints (&im1, "m1");
#endif
   retcode = div_sf (&im0, &im1);
#ifdef DEBUG_FLOAT
   printf ("   retcode = %d\n", retcode);
   ibm_prints (&im0, "quo");
#endif
   if (retcode == 0)
      store_sf (&im0, m0);
   return (retcode);
}

int
ibm_fixs (uint32 *m0)
{
int  retval;
int     m3 = 1;
SHORT_FLOAT fl;
uint8      shift;
uint32     lsfract;

    /* Get register content */
    get_sf(&fl, m0);

    if (fl.short_fract) {
        /* not zero */
        normal_sf(&fl);

        if (fl.expo > 72) {
            /* exeeds range by exponent */
            retval = fl.sign ? 0x80000000UL : 0x7FFFFFFFUL;
            /*regs->psw.cc = 3;*/
            return(retval);
        }
        if (fl.expo > 70) {
            /* to be left shifted */
            fl.short_fract <<= ((fl.expo - 70) * 4);
            if (fl.sign) {
                /* negative */
                if (fl.short_fract > 0x80000000UL) {
                    /* exeeds range by value */
                    retval = 0x80000000UL;
                    /*regs->psw.cc = 3;*/
                    return(retval);
                }
            } else {
                /* positive */
                if (fl.short_fract > 0x7FFFFFFFUL) {
                    /* exeeds range by value */
                    retval = 0x7FFFFFFFUL;
                    /*regs->psw.cc = 3;*/
                    return(retval);
                }
            }
        } else if ((fl.expo > 64)
               && (fl.expo < 70)) {
            /* to be right shifted and to be rounded */
            shift = ((70 - fl.expo) * 4);
            lsfract = fl.short_fract << (32 - shift);
            fl.short_fract >>= shift;

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & 0x80000000UL) {
                    fl.short_fract++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if ((lsfract > 0x80000000UL)
                || ((fl.short_fract & 0x00000001UL)
                    && (lsfract == 0x80000000UL))) {
                    fl.short_fract++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && lsfract) {
                    fl.short_fract++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && lsfract) {
                    fl.short_fract++;
                }
            }
        } else if (fl.expo == 64) {
            /* to be rounded */
            lsfract = fl.short_fract << 8;
            fl.short_fract = 0;

            if (m3 == 1) {
                /* biased round to nearest */
                if (lsfract & 0x80000000UL) {
                    fl.short_fract++;
                }
            } else if (m3 == 4) {
                /* round to nearest */
                if (lsfract > 0x80000000UL) {
                    fl.short_fract++;
                }
            } else if (m3 == 6) {
                /* round toward + */
                if ((fl.sign == POS)
                && lsfract) {
                    fl.short_fract++;
                }
            } else if (m3 == 7) {
                /* round toward - */
                if ((fl.sign == NEG)
                && lsfract) {
                    fl.short_fract++;
                }
            }
        } else if (fl.expo < 64) {
            fl.short_fract = 0;
            if (((m3 == 6)
                && (fl.sign == POS))
            || ((m3 == 7)
                && (fl.sign == NEG))) {
                fl.short_fract++;
            }
        }
        if (fl.sign) {
            /* negative */
            retval = -((int32) fl.short_fract);
            /*regs->psw.cc = 1;*/
        } else {
            /* positive */
            retval = fl.short_fract;
            /*regs->psw.cc = 2;*/
        }
    } else {
        /* zero */
        retval = 0;
        /*regs->psw.cc = 0;*/
    }
#ifdef DEBUG_FLOAT
   printf ("   retval = %d\n", retval);
#endif
    return (retval);
}


int
ibm_flts (uint32 *m0, int32 c)
{
LONG_FLOAT fl;
SHORT_FLOAT sl;
t_int64 fix;

    /* get fixed value */
    fix = c;
    if (fix &  MASK12)
        fix |= MASK13;

    if (fix) {
        if (fix < 0) {
            fl.sign = NEG;
            fl.long_fract = (-fix);
        } else {
            fl.sign = POS;
            fl.long_fract = fix;
        }
        fl.expo = 78;

        /* Normalize result */
        normal_lf(&fl);

        /* To register (converting to short float) */
	sl.sign = fl.sign;
	sl.expo = fl.expo;
	sl.short_fract = (uint32)((fl.long_fract >> 32) & 0x00ffffff);
        store_sf (&sl, m0);
    } else {
        /* true zero */
        *m0 = 0;
    }
   return (0);
}

int
ibm_mpys (uint32 *m0, uint32 *m1)
{
   int retcode;
   SHORT_FLOAT im0, im1;

   get_sf (&im0, m0);
   get_sf (&im1, m1);
#ifdef DEBUG_FLOAT
   printf ("ibm_mpys: \n");
   ibm_prints (&im0, "m0");
   ibm_prints (&im1, "m1");
#endif
   retcode = mul_sf (&im0, &im1, NOOVUNF);
#ifdef DEBUG_FLOAT
   printf ("   retcode = %d\n", retcode);
   ibm_prints (&im0, "prod");
#endif
   if (retcode == 0)
      store_sf (&im0, m0);
   return (retcode);
}

int
ibm_negs (uint32 *m0)
{
   SHORT_FLOAT im0;

   get_sf (&im0, m0);
#ifdef DEBUG_FLOAT
   printf ("ibm_negs: \n");
   ibm_prints (&im0, "m0");
#endif
   im0.sign = ! (im0.sign);
   store_sf (&im0, m0);
#ifdef DEBUG_FLOAT
   ibm_prints (&im0, "neg");
#endif
   return (0);
}

int
ibm_subs (uint32 *m0, uint32 *m1)
{
   int retcode;
   SHORT_FLOAT im0, im1;

   get_sf (&im0, m0);
   get_sf (&im1, m1);
#ifdef DEBUG_FLOAT
   printf ("ibm_subs: \n");
   ibm_prints (&im0, "m0");
   ibm_prints (&im1, "m1");
#endif

   /* Invert the sign of 2nd operand */
   im1.sign = ! (im1.sign);

   retcode = add_sf (&im0, &im1, NORMAL, NOSIGEX);
#ifdef DEBUG_FLOAT
   printf ("   retcode = %d\n", retcode);
   ibm_prints (&im0, "sum");
#endif
   if (retcode == 0)
      store_sf (&im0, m0);
   return (retcode);
}

/* end of float.c */
#endif /* USS */
