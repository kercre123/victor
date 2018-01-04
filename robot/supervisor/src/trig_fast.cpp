/**
* File: trig_fast.cpp
*
* Author: Kevin Yoon
* Created: 22-OCT-2012
*
* The arctangent lookup table atan_lut stores at each
* index=x*ATAN_LUT_INPUT_MULTIPLIER the value
* atan(x)*ATAN_LUT_OUTPUT_MULTIPLIER as u8
*
* Ditto for arcsine lookup table asin_lut.
*
*
**/
#include <math.h>
#include "coretech/common/shared/types.h"
#include "anki/common/constantsAndMacros.h"
#include "trig_fast.h"
#include "anki/cozmo/robot/logging.h"

// For larger input values to atan, use approximations
// at fixed steps. (Essentially extends the LUT with courser
// resolution at higher input values.)
#define DO_ATAN_DISCRETE_STEP_APPROX

#ifdef USE_SMALL_LUT
// For small table, automatically use interpolation
#define USE_INTERPOLATION

#define ASIN_LUT_SIZE 51
#define ASIN_LUT_INPUT_MULTIPLIER 50
#define ASIN_LUT_OUTPUT_MULTIPLIER 100
const u8 asin_lut[] =
{
  0,2,4,6,8,10,12,14,16,18,
  20,22,24,26,28,30,33,35,37,39,
  41,43,46,48,50,52,55,57,59,62,
  64,67,69,72,75,78,80,83,86,89,
  93,96,100,104,108,112,117,122,129,137,
  157,
};

#define ATAN_LUT_SIZE 51
#define ATAN_LUT_INPUT_MULTIPLIER 2.5
#define ATAN_LUT_OUTPUT_MULTIPLIER 100
const u8 atan_lut[] =
{
  0,38,67,88,101,111,118,123,127,130,
  133,135,137,138,139,141,142,142,143,144,
  145,145,146,146,147,147,147,148,148,148,
  149,149,149,150,150,150,150,150,151,151,
  151,151,151,151,151,152,152,152,152,152,
  152,
};

#else  // not defined USE_SMALL_LUT

#define ASIN_LUT_SIZE 101
#define ASIN_LUT_INPUT_MULTIPLIER 100
#define ASIN_LUT_OUTPUT_MULTIPLIER 100
const u8 asin_lut[] =
{
  0,1,2,3,4,5,6,7,8,9,
  10,11,12,13,14,15,16,17,18,19,
  20,21,22,23,24,25,26,27,28,29,
  30,32,33,34,35,36,37,38,39,40,
  41,42,43,44,46,47,48,49,50,51,
  52,54,55,56,57,58,59,61,62,63,
  64,66,67,68,69,71,72,73,75,76,
  78,79,80,82,83,85,86,88,89,91,
  93,94,96,98,100,102,104,106,108,110,
  112,114,117,119,122,125,129,133,137,143,
  157,
};

#define ATAN_LUT_SIZE 101
#define ATAN_LUT_INPUT_MULTIPLIER 5
#define ATAN_LUT_OUTPUT_MULTIPLIER 100
const u8 atan_lut[] =
{
  0,20,38,54,67,79,88,95,101,106,
  111,114,118,120,123,125,127,128,130,131,
  133,134,135,136,137,137,138,139,139,140,
  141,141,142,142,142,143,143,144,144,144,
  145,145,145,146,146,146,146,146,147,147,
  147,147,147,148,148,148,148,148,148,149,
  149,149,149,149,149,149,150,150,150,150,
  150,150,150,150,150,150,151,151,151,151,
  151,151,151,151,151,151,151,151,151,151,
  152,152,152,152,152,152,152,152,152,152,
  152,
};
#endif // USE_SMALL_LUT

#ifdef DO_ATAN_DISCRETE_STEP_APPROX
const float ATAN_LUT_MAX = (float)(atan_lut[ATAN_LUT_SIZE-1])/ATAN_LUT_OUTPUT_MULTIPLIER;
#define ATAN25 1.5308f
#define ATAN30 1.5375f
#define ATAN40 1.5458f
#define ATAN60 1.5541f
#define ATAN120 1.5625f
#define ATAN600 1.5691f
const float ATAN_LUT_MAX_INPUT = (ATAN_LUT_SIZE-1)/ATAN_LUT_INPUT_MULTIPLIER;
const float SLOPE_TO_25 = (ATAN25-ATAN_LUT_MAX)/(25 - ATAN_LUT_MAX_INPUT);
const float SLOPE_TO_30 = (ATAN30-ATAN25)/(30 - 25);
const float SLOPE_TO_40 = (ATAN40-ATAN30)/(40 - 30);
const float SLOPE_TO_60 = (ATAN60-ATAN40)/(60 - 40);
const float SLOPE_TO_120 = (ATAN120-ATAN60)/(120 - 60);
const float SLOPE_TO_600 = (ATAN600-ATAN120)/(600 - 120);
#endif // DO_ATAN_DISCRETE_STEP_APPROX

// Uses lookup table for input values upto (ATAN_LUT_SIZE-1)/ATAN_LUT_INPUT_MULTIPLIER.
// Approximates values beyond that with discrete step values. Clunky but fast and good enough. Trying to keep ~0.01 accuracy.
float atan_fast(float x)
{
  // LUT accepts positive input only so need to remember sign so that we
  // can flip result if input is negative.
  u8 isNegative = x < 0 ? 1 : 0;

#ifdef DO_ATAN_DISCRETE_STEP_APPROX
  // Check if input is in discrete step approximation range
  float absx = ABS(x);
  if (absx >= ATAN_LUT_MAX_INPUT) {
    float res;

    if (absx >= 600) {
      res = M_PI_2_F;
    }
    else if (absx >= 120) {
      res = SLOPE_TO_600 * (absx - 120) + ATAN120;
    }
    else if (absx >= 60) {
      res = SLOPE_TO_120 * (absx - 60) + ATAN60;
    }
    else if (absx >= 40) {
      res = SLOPE_TO_60 * (absx - 40) + ATAN40;
    }
    else if (absx >= 30) {
      res = SLOPE_TO_40 * (absx - 30) + ATAN30;
    }
    else if (absx >= 25) {
      res = SLOPE_TO_30 * (absx - 25) + ATAN25;
    }
    else {
      res = SLOPE_TO_25 * (absx - ATAN_LUT_MAX_INPUT) + ATAN_LUT_MAX;
    }

    if (isNegative) {
      res *= -1;
    }

    return res;
  }
#endif

#ifdef USE_INTERPOLATION

  // Convert x to LUT index
  float x_lut_idx = ABS(x)*ATAN_LUT_INPUT_MULTIPLIER;
  u8 x_lut_pre_idx = (u8)(x_lut_idx);
  u8 x_lut_post_idx = x_lut_pre_idx + 1;
  float frac = x_lut_idx - (float)x_lut_pre_idx;

  // Check if input exceeds LUT range
  float lut_result;
  if (x_lut_post_idx >= ATAN_LUT_SIZE) {
    lut_result = M_PI_2_F;
  } else {
    u8 atan_pre_res = atan_lut[x_lut_pre_idx];
    u8 atan_post_res = atan_lut[x_lut_post_idx];

    lut_result = ((float)(atan_post_res - atan_pre_res)*frac + atan_pre_res) / ATAN_LUT_OUTPUT_MULTIPLIER;
  }

#else

  // Convert x to LUT index
  int x_lut_idx = (int)(ABS(x)*ATAN_LUT_INPUT_MULTIPLIER);

  // Check if input exceeds LUT range
  float lut_result;
  if (x_lut_idx >= ATAN_LUT_SIZE) {
    lut_result = PI_DIV2;
  } else {
    lut_result = (float)atan_lut[x_lut_idx] / ATAN_LUT_OUTPUT_MULTIPLIER;
  }
#endif

  if (isNegative) {
    lut_result *= -1;
  }

  return lut_result;
}

float asin_fast(float x)
{
  // Clip to valid range
  x = CLIP(x,-1,1);

#ifdef USE_INTERPOLATION
  // Convert x to LUT index
  float x_lut_idx = ABS(x)*ASIN_LUT_INPUT_MULTIPLIER;
  u8 x_lut_pre_idx = (u8)(x_lut_idx);
  u8 x_lut_post_idx = x_lut_pre_idx + 1;
  float frac = x_lut_idx - (float)x_lut_pre_idx;

  // LUT accepts positive input only so need to remember sign so that we
  // can flip result if input is negative.
  u8 isNegative = x < 0 ? 1 : 0;

  // Check if input exceeds LUT range
  float lut_result;
  if (x_lut_post_idx >= ASIN_LUT_SIZE) {
    lut_result = M_PI_2_F;
  } else {
    u8 asin_pre_res = asin_lut[x_lut_pre_idx];
    u8 asin_post_res = asin_lut[x_lut_post_idx];

    lut_result = ((float)(asin_post_res - asin_pre_res)*frac + asin_pre_res) / ASIN_LUT_OUTPUT_MULTIPLIER;
  }

#else

  // Convert x to LUT index
  int x_lut_idx = (int)(ABS(x)*ASIN_LUT_INPUT_MULTIPLIER);

  // LUT accepts positive input only so need to remember sign so that we
  // can flip result if input is negative.
  u8 isNegative = x < 0 ? 1 : 0;

  // Check if input exceeds LUT range
  float lut_result;
  if (x_lut_idx >= ASIN_LUT_SIZE) {
    lut_result = PI_DIV2;
  } else {
    lut_result = (float)asin_lut[x_lut_idx] / ASIN_LUT_OUTPUT_MULTIPLIER;
  }

#endif

  if (isNegative) {
    lut_result *= -1;
  }

  return lut_result;
}

float atan2_fast(float y, float x)
{
  AnkiAssert( !(y == 0 && x == 0), "");

  if (x>0) {
    return atan_fast(y/x);
  } else if (y >= 0 && x < 0) {
    return atan_fast(y/x) + M_PI_F;
  } else if (y < 0 && x < 0) {
    return atan_fast(y/x) - M_PI_F;
  } else if (y > 0 && x == 0) {
    return M_PI_2_F;
  } else if (y < 0 && x == 0) {
    return -M_PI_2_F;
  }

  return 0;
}

float atan2_acc(float y, float x)
{
  AnkiAssert(y != 0 || x != 0, "");

  float arg = y/x;
  float atan_val = asinf( arg / sqrtf(arg*arg + 1));

  if (x > 0) {
    return atan_val;
  } else if (y >= 0 && x < 0) {
    return atan_val + M_PI_F;
  } else if (y < 0 && x < 0) {
    return atan_val - M_PI_F;
  } else if (y > 0 && x == 0) {
    return M_PI_2_F;
  }
  //else if (y < 0 && x == 0) {
  return -M_PI_2_F;
  //}
}
