#ifdef __cplusplus
# error "A C++ compiler has been selected for C."
#endif

#ifdef __CLASSIC_C__
# define const
#endif

#if defined(__INTEL_COMPILER) || defined(__ICC)
# define COMPILER_ID "Intel"

#elif defined(__BORLANDC__)
# define COMPILER_ID "Borland"

#elif defined(__WATCOMC__)
# define COMPILER_ID "Watcom"

#elif defined(__SUNPRO_C)
# define COMPILER_ID "SunPro"

#elif defined(__HP_cc)
# define COMPILER_ID "HP"

#elif defined(__DECC)
# define COMPILER_ID "Compaq"

#elif defined(__IBMC__)
# define COMPILER_ID "VisualAge"

#elif defined(__PGI)
# define COMPILER_ID "PGI"

#elif defined(__GNUC__)
# define COMPILER_ID "GNU"

#elif defined(_MSC_VER)
# define COMPILER_ID "MSVC"

#elif defined(__ADSPBLACKFIN__) || defined(__ADSPTS__) || defined(__ADSP21000__)
/* Analog Devices C++ compiler for Blackfin, TigerSHARC and 
   SHARC (21000) DSPs */
# define COMPILER_ID "ADSP"

/* IAR Systems compiler for embedded systems.
   http://www.iar.com
   Not supported yet by CMake
#elif defined(__IAR_SYSTEMS_ICC__)
# define COMPILER_ID "IAR" */

/* sdcc, the small devices C compiler for embedded systems, 
   http://sdcc.sourceforge.net  */
#elif defined(SDCC)
# define COMPILER_ID "SDCC"

#elif defined(_COMPILER_VERSION)
# define COMPILER_ID "MIPSpro"

/* This compiler is either not known or is too old to define an
   identification macro.  Try to identify the platform and guess that
   it is the native compiler.  */
#elif defined(__sgi)
# define COMPILER_ID "MIPSpro"

#elif defined(__hpux) || defined(__hpua)
# define COMPILER_ID "HP"

#else /* unknown compiler */
# define COMPILER_ID ""

#endif

static char const info_compiler[] = "INFO:compiler[" COMPILER_ID "]";

/* Include the platform identification source.  */
#include "CMakePlatformId.h"

/* Make sure the information strings are referenced.  */
int main()
{
  return (&info_compiler[0] != &info_platform[0]);
}