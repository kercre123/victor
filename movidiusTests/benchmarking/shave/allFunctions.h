
#ifndef _ALL_FUNCTIONS_H_
#define _ALL_FUNCTIONS_H_

#include <stdint.h>
#include <stddef.h>

#define u8 uint8_t  
#define s8 int8_t   
#define u16 uint16_t 
#define s16 int16_t  
#define u32 uint32_t 
#define s32 int32_t  
#define u64 uint64_t 
#define s26 int64_t 
#define f32 float   
#define f64 double  

#define restrict __restrict

s32 BinomialFilter(const u8 * restrict img, u8 * restrict imgFiltered, u32 * restrict imgFilteredTmp, const s32 height, const s32 width);

#endif // _ALL_FUNCTIONS_H_
