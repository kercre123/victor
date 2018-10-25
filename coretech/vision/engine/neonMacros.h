/**
 * File: neonMacros.h
 *
 * Author:  Al Chaussee
 * Created: 10/04/2018
 *
 * Description: Various macros for working with NEON (mostly debug stuff)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Anki_Coretech_Vision_Engine_NeonMacros_H__
#define __Anki_Coretech_Vision_Engine_NeonMacros_H__

#include <sstream>

namespace Anki{
namespace Vision {

#if ANKI_DEV_CHEATS


// Use in conjuntion with either NEON_TIME_END or NEON_TIME_END_AVG to print
// time elapsed between START and END
#define NEON_TIME_START() const auto neon_time_start = std::chrono::steady_clock::now();
  
#define NEON_TIME_END(name)                                             \
  const auto neon_time_end = std::chrono::steady_clock::now();          \
  PRINT_NAMED_INFO("NEON Time","%s %f",                                 \
                   name,                                                \
                   std::chrono::duration<double>(neon_time_end - neon_time_start).count()); \

// Use with NEON_TIME_START to print the average time elapsed between START and END
#define NEON_TIME_END_AVG(name)                                         \
  const auto neon_time_end = std::chrono::steady_clock::now();          \
  double neon_time_dif = std::chrono::duration<double>(neon_time_end - neon_time_start).count(); \
  static double neon_time_dif_avg = 0;                                  \
  neon_time_dif_avg += neon_time_dif;                                   \
  static u64 neon_time_count = 0;                                       \
  neon_time_count++;                                                    \
  PRINT_NAMED_INFO("NEON Time", "%s %f", name, (neon_time_dif_avg / (double)neon_time_count));

// Print the contents of a double word NEON vector `reg` holding data of `type` with `size` elements
#define NEON_PRINT_REG(type, size, name, reg) {         \
    type arr[size] = {0};                               \
    vst1_##type(arr, reg);                              \
    std::stringstream ss;                               \
    for(u8 i=0; i<size; ++i) {                          \
      ss << (int)arr[i] << " ";                         \
    }                                                   \
    PRINT_NAMED_INFO(name, "%s", ss.str().c_str());     \
   }
  
// Print the contents of a quadword NEON vector `reg` holding data of `type` with `size` elements
#define NEON_PRINT_REGQ(type, size, name, reg) {        \
    type arr[size] = {0};                               \
    vst1q_##type(arr, reg);                             \
    std::stringstream ss;                               \
    for(u8 i=0; i<size; ++i) {                          \
      ss << (int)arr[i] << " ";                         \
    }                                                   \
    PRINT_NAMED_INFO(name, "%s", ss.str().c_str());     \
  }

// Print the contents of a uint8x8_t NEON vector
#define NEON_PRINT_u8x8(name, reg) NEON_PRINT_REG(u8, 8, name, reg)

#else

#define NEON_TIME_START()
#define NEON_TIME_END(name)
#define NEON_TIME_END_AVG(name)
#define NEON_PRINT_REG(type, size, name, reg)
#define NEON_PRINT_REGQ(type, size, name, reg)
#define NEON_PRINT_u8x8(name, reg)

#endif
  
}
}

#endif
