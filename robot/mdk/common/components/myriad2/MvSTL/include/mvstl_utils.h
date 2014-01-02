#ifndef __UTILS_H__
#define __UTILS_H__

//#define NO_PRINT

//If defined, strips printfs from all files that include this header
#ifdef NO_PRINT
#define printf(...) 
#endif

// General purpose 
#define KB(x)				(x * 1024)
#define MB(x)				(KB(x) * 1024)

#ifdef __MOVICOMPILE__
// Memory blocks start address on Myriad1
#define DDRL2B_START_ADDR	0x48000000
#define DDRL2C_START_ADDR	0x40000000
#define CMX_MXI_START_ADDR	0x10000000
#define CMX_LHB_START_ADDR	0xA0000000
#define LRAM_START_ADDR		0x90100000

// Memory blocks length on Myriad1
#define DDRL2B_SIZE		MB(64)
#define DDRL2C_SIZE		MB(64)
#define CMX_MXI_SIZE	MB(1)
#define CMX_LHB_SIZE	MB(1)
#define LRAM_SIZE		KB(32)

// Memory blocks end address on Myriad1
#define DDRL2B_END_ADDR		(DDRL2B_START_ADDR + DDRL2B_SIZE)
#define DDRL2C_END_ADDR		(DDRL2C_START_ADDR + DDRL2C_SIZE)
#define CMX_MXI_END_ADDR	(CMX_MXI_START_ADDR + CMX_MXI_SIZE)
#define CMX_LHB_END_ADDR	(CMX_LHB_START_ADDR + CMX_LHB_SIZE)
#define LRAM_END_ADDR		(LRAM_START_ADDR + LRAM_SIZE)

// DDR address range
#define IS_DDRL2B(x)		(((u32)(x) >= DDRL2B_START_ADDR) && ((u32)(x) <= DDRL2B_END_ADDR))
#define IS_DDRL2C(x)		(((u32)(x) >= DDRL2C_START_ADDR) && ((u32)(x) <= DDRL2C_END_ADDR))

// CMX address range
#define IS_SHAVE_CMX(x)		(((u32)(x) >= CMX_MXI_START_ADDR) && ((u32)(x) <= CMX_MXI_END_ADDR))
#define IS_LEON_CMX(x)		(((u32)(x) >= CMX_LHB_START_ADDR) && ((u32)(x) <= CMX_LHB_END_ADDR))

// Main memory blocks used by applications
#define IS_DDR(x)			(IS_DDRL2B((u32)(x)) || IS_DDRL2C((u32)(x)))
#define IS_CMX(x)			(IS_SHAVE_CMX((u32)(x)) || IS_LEON_CMX((u32)(x)))
#define IS_LRAM(x)			(((u32)(x) >= LRAM_START_ADDR) && ((u32)(x) <= LRAM_END_ADDR))

// Check if it is a valid memory address
#define IS_VALID_ADDR(x)	(IS_DDR((u32)(x)) || IS_CMX((u32)(x)) || IS_LRAM((u32)(x)))

#else

// Dummy DDR address range
#define IS_DDRL2B(x) 1
#define IS_DDRL2C(x) 1

// Dummy CMX address range
#define IS_SHAVE_CMX(x) 1
#define IS_LEON_CMX(x) 1

// Dummy main memory blocks used by applications
#define IS_DDR(x) 1
#define IS_CMX(x) 1
#define IS_LRAM(x) 1

// Dummy check if it is a valid memory address
#define IS_VALID_ADDR(x) 1

#endif

#if defined(__MOVICOMPILE__) && !defined(_WIN32)
//Shave

#include "svuCommonShave.h"

#define NOLOAD __attribute__((section("data_no_load \n.noload"))) __attribute__((aligned(16)))
#define SHAVE_HALT __asm volatile("bru.swih 0x13")
#define svu_assert(X) ({if (!(X)) { __asm volatile("bru.swih 0x11 || \
												lsu0.ldil i1, 0xFFFF || \
												lsu1.ldih i1, 0xFFFF || \
												cmu.cpti i0, B_IP_0"); } })

#define ENABLE_FREE_COUNTER()	(*(int*)0x80020008 = 1)
#define CLEAR_FREE_COUNTER()	(*(int*)0x80020090 = 0)
#define GET_FREE_COUNTER()		(*(int*)0x80020090)

//moviCompile wants this in order to inline functions
#define inline __attribute__((always_inline))

//dma_copy comes from external assembly file
#define dma_copy scDmaCopyBlocking

//#ifdef __cplusplus
//void dma_copy(unsigned char* dst, const unsigned char* src, unsigned int len);
//#else
//void dma_copy(unsigned char* dst, const unsigned char* src, unsigned int len);
//#endif

#define WAIT_KEY 

#define set_stack_pointer() __asm("lsu0.ldil i19 shave_sp || lsu1.ldih i19 shave_sp")

#else
//PC

#include <string.h>
#include <myriadModel.h>

#define NOLOAD
#define SHAVE_HALT
#define svu_assert(X) assert(X)

//#include <intrin.h>
/*unsigned __int64 rdtsc(void)
{
	return __rdtsc();
}*/

#define ENABLE_FREE_COUNTER()
#define CLEAR_FREE_COUNTER()	//rdtsc
#define GET_FREE_COUNTER()		0//rdtsc

//Simulate dma_copy with memcpy
#define dma_copy memcpy
//#define scDmaSetup(taskNum, src, dst, numBytes) memcpy(dst, src, numBytes)
//#define scDmaStart(a)
//#define scDmaSetupStrideSrc(taskNum, src, dst, numBytes, width, stride)
//#define scDmaCopyBlocking(a, b, c)
//#define scDmaWaitFinished()

#define WAIT_KEY system("PAUSE")

#define set_stack_pointer()

#endif // __PC__

// Using custom assert
//#include <assert.h>

#ifndef ABS
#define ABS(a) (( (a) > 0 )? (a) : -(a))
#endif

#ifndef MIN
#define MIN(a,b) ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#endif

#ifndef NULL
#define NULL 0
#endif

#endif
