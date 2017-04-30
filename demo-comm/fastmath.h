/* @(#)fastmath.h	1.1 04/10/17 */

#ifndef _FASTMATH_H_
#define _FASTMATH_H_

#include <math.h>
#include <float.h>

#ifdef __STDC__
 extern int    ifloor(double x);
 extern int    iceil(double x);
#else
 extern int    ifloor();
 extern int    iceil();
#endif

#ifndef _MATH_H /* if Sun's math.h was not included, use values from this file */

 #ifdef __STDC__
  extern double expm1(double x);
 #else
  extern double expm1();
 #endif

 #ifdef M_E
  #undef M_E
 #endif
 #define	M_E		2.7182818284590452354

 #ifdef M_LOG2E
  #undef M_LOG2E
 #endif
 #define	M_LOG2E		1.4426950408889634074

 #ifdef M_LOG10E
  #undef M_LOG10E
 #endif
 #define	M_LOG10E	0.43429448190325182765

 #ifdef M_LN2
  #undef M_LN2
 #endif
 #define	M_LN2		0.69314718055994530942

 #ifdef M_LN10
  #undef M_LN10
 #endif
 #define	M_LN10		2.30258509299404568402

 #ifdef M_PI
  #undef M_PI
 #endif
 #define	M_PI		3.14159265358979323846

 #ifdef M_PI_2
  #undef M_PI_2
 #endif
 #define	M_PI_2		1.57079632679489661923

 #ifdef M_PI_4
  #undef M_PI_4
 #endif
 #define	M_PI_4		0.78539816339744830962

 #ifdef M_1_PI
  #undef M_1_PI
 #endif
 #define	M_1_PI		0.31830988618379067154

 #ifdef M_2_PI
  #undef M_2_PI
 #endif
 #define	M_2_PI		0.63661977236758134308

 #ifdef M_2_SQRTPI
  #undef M_2_SQRTPI
 #endif
 #define	M_2_SQRTPI	1.12837916709551257390

 #ifdef M_SQRT2
  #undef M_SQRT2
 #endif
 #define	M_SQRT2		1.41421356237309504880

 #ifdef M_SQRT1_2
  #undef M_SQRT1_2
 #endif
 #define	M_SQRT1_2	0.70710678118654752440

#else  /* if _MATH_H */
 #define NEED_IROUND_MACRO
#endif /* end of #ifndef _MATH_H */

/**=== a Few Things for m430 ===**/

#ifdef M_2PI
 #undef M_2PI
#endif
#define M_2PI    (2.0*M_PI)

#ifdef M_PI_SQR
 #undef M_PI_SQR
#endif
#define M_PI_SQR (M_PI*M_PI)

#ifdef M_PI_INV
 #undef M_PI_INV
#endif
#define M_PI_INV (1.0/M_PI)

#ifdef DEGR_PER_RADIAN
 #undef DEGR_PER_RADIAN
#endif
#define DEGR_PER_RADIAN (57.29577951308232088)

#ifndef RADIAN_PER_DEGR
 #undef RADIAN_PER_DEGR
#endif
#define RADIAN_PER_DEGR (1.0/DEGR_PER_RADIAN)

/**=============================**/

#if (defined(__SVR4) || defined(__linux__))
 #define NEED_IRINT_MACRO
#endif

#ifdef VX
 #ifndef NOT_VX

/* We rely that compiler defines "mc68000" for the entire m68k family 
   and that appropriate CPU type (MC68060) is defined in our makefiles. 
   also we rely that compiler defines "_ARCH_PPC" for the entire PPC family.
*/

  #ifdef _ARCH_PPC

   #ifndef NEED_IRINT_MACRO
    #define NEED_IRINT_MACRO
   #endif

   #ifndef NEED_IROUND_MACRO
    #define NEED_IROUND_MACRO
   #endif

  #else /* not _ARCH_PPC */

   #ifdef NEED_IRINT_MACRO
    #undef NEED_IRINT_MACRO
   #endif 

   #ifdef NEED_IROUND_MACRO
    #undef NEED_IROUND_MACRO
   #endif 

   #ifdef mc68000
    #ifndef __math_68881
     #include <math-68881.h>
    #endif
   #endif

  #endif /*end of ifdef _ARCH_PPC */

 #endif /*end of ifndef NOT_VX */ 
#endif /*end of ifdef VX */ 

/* macros defining common math functions */

#ifdef IROUND
 #undef IROUND
#endif
#define IROUND(x)  ((x) < 0.0 ? (int)(x - 0.5) : (int)(x + 0.5))

#ifdef IRINT
 #undef IRINT
#endif
#define IRINT(x)  IROUND(x)

#ifdef UROUND
 #undef UROUND
#endif
#define UROUND(x)  ((unsigned int)(x + 0.5))

#ifdef ICEIL
 #undef ICEIL
#endif
#define ICEIL(x) ((x) > 0.0 ? ((int)((x) * (1 - DBL_EPSILON)) + 1): (int)(x))

#ifdef IFLOOR
 #undef IFLOOR
#endif
#define IFLOOR(x) ((x) < 0.0 ? ((int)((x) * (1 - DBL_EPSILON)) - 1): (int)(x))

#ifdef ABS
 #undef ABS
#endif
#define ABS(a) ((a) >= 0? (a): -(a))

#ifdef MAX
 #undef MAX  /* defined in xview/base.h remove warning */
#endif
#define MAX(a,b) ((a) > (b)? (a): (b))

#ifdef MIN
 #undef MIN  /* defined in xview/base.h remove warning */
#endif
#define MIN(a,b) ((a) < (b)? (a): (b))

#ifdef SIGN
 #undef SIGN
#endif
#define SIGN(a,b) ((b) >= 0? ABS(a): -ABS(a))

#ifdef SQUARE
 #undef SQUARE
#endif
#define SQUARE(n) ((n)*(n))

/* We define here hypot as macro because it's faster this way.
Sun and VxWorks 5.5.1 implementation slower but it works
correctly with numbers ~ DBL_MAX, which we never use for now. 
VxWorks 5.4 does not have hypot for ppc.  AB */

#define hypot(A, B) (sqrt(A * A + B * B))

/* irint() is often much more efficient than the alternatives [eg. on 68K],
   but it was removed from Solaris.  So we define it as a macro.
 * According to the VxWorks man page, irint() converts a double precision
 * value to an integer.  The rounding direcection is not pre-selectable and
 * is fixed to round-to-nearest.
 */

#ifdef NEED_IRINT_MACRO
 #ifdef irint
  #undef irint
 #endif
 #define irint(x)  IRINT(x)
#endif

/* iround() in vxWorks rounds a double to the nearest integer.
 * We DO NOT support the case:
 * "If the argument is spaced evenly between two integers,
 *  it returns the even integer"
 */
#ifdef NEED_IROUND_MACRO
 #ifdef iround
  #undef iround
 #endif
 #define iround(x)  IROUND(x)
#endif

/* ifloor(x) is similar to ANSI floor(x), except that its return type is integer;
   ifloor(x) returns the largest integer not greater than x.
 */

#ifdef NEED_IFLOOR_MACRO
 #ifdef ifloor
  #undef ifloor
 #endif
 #define ifloor(x)  IFLOOR(x)
#endif

/* iceil(x) is similar to ANSI ceil(x), except that its return type is integer;
   iceil(x) returns the smallest integer not less than x.
 */

#ifdef NEED_ICEIL_MACRO
 #ifdef iceil
  #undef iceil
 #endif
 #define iceil(x)  ICEIL(x)
#endif


#endif /* _FASTMATH_H_ */
