#ifndef ZAI_MATH_LINEAR_ALGEBRA_H
#define ZAI_MATH_LINEAR_ALGEBRA_H

#include "zai_math_basic.h"

/* #############################################################################
 * # [SECTION] Linear Algebra Math (SIMD Detection)
 * #############################################################################
 */
#ifdef ZAI_DISABLE_SIMD
#include "zai_math_linear_algebra_scalar.h"
#elif defined(ZAI_ARCH_X64)
#include "zai_math_linear_algebra_sse2.h"
#endif

#endif /* ZAI_MATH_LINEAR_ALGEBRA_H */