///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     API manipulating Leon functionalities
///
/// Allows manipulating leon caches and other features
///

#ifndef __LEON_UTILS_H__
#define __LEON_UTILS_H__

#include "swcLeonUtilsDefines.h"
#include <mv_types.h>

typedef enum {
    be_pointer, ///< normal leon pointer
    le_pointer  ///< little-endian/shave pointer
} pointer_type; /// the pointer type is only relevant if it is addressing < 32bit values

/// Swaps endianness of a 32-bit integer (usefull when sharing data between Leon and Shave)
/// @param[in] value u32 integer to be swapped
/// @returns swapped integer
#define swcLeonSwapU32(value) \
        ((((u32)((value) & 0x000000FF)) << 24) | \
                ( ((u32)((value) & 0x0000FF00)) <<  8) | \
                ( ((u32)((value) & 0x00FF0000)) >>  8) | \
                ( ((u32)((value) & 0xFF000000)) >> 24))


/// Swaps endianness of a 16-bit integer (usefull when sharing data between Leon and Shave)
/// @param[in] value u16 integer to be swapped
/// @returns swapped integer
#define swcLeonSwapU16(value) \
        ((((u16)((value) & 0x00FF)) << 8) | \
                (((u16)((value) & 0xFF00)) >> 8))


/// Reads data bypassing leon LRAM cache
/// @param[in] addr u32 address to read
/// @return - u8 variable value read bypassing cache
#define swcLeonReadNoCacheU8(addr) \
  ({ unsigned char x; \
     asm volatile( "lduba [%1] 1, %0" : "=r"(x) : "r"(addr) ); \
     x; })


/// Reads data bypassing leon LRAM cache
/// @param[in] addr u32 address to read
/// @return - i8 variable value read bypassing cache
#define swcLeonReadNoCacheI8(addr) \
  ({ signed char x; \
     asm volatile( "ldsba [%1] 1, %0" : "=r"(x) : "r"(addr) ); \
     x; })


/// Reads data bypassing leon LRAM cache
/// @param[in] addr u32 address to read
/// @return - u16 variable value read bypassing cache
#define swcLeonReadNoCacheU16(addr) \
  ({ unsigned short x; \
     asm volatile( "lduha [%1] 1, %0" : "=r"(x) : "r"(addr) ); \
     x; })


/// Reads data bypassing leon LRAM cache
/// @param[in] addr u32 address to read
/// @return - i16 variable value read bypassing cache
#define swcLeonReadNoCacheI16(addr) \
  ({ signed short x; \
     asm volatile( "ldsha [%1] 1, %0" : "=r"(x) : "r"(addr) ); \
     x; })


/// Reads data bypassing leon LRAM cache
/// @param[in] addr u32 address to read
/// @return - u32 variable value read bypassing cache
#define swcLeonReadNoCacheU32(addr) \
  ({ unsigned int x; \
     asm volatile( "lda [%1] 1, %0" : "=r"(x) : "r"(addr) ); \
     x; })


/// Reads data bypassing leon LRAM cache
/// @param[in] addr u32 address to read
/// @return - s32 variable value read bypassing cache
#define swcLeonReadNoCacheI32(addr) ((int)swcLeonReadNoCacheU32(addr))


/// Reads data bypassing leon L1 cache
/// @param[in] addr address of u64 to read
/// @return - u64 variable value read bypassing cache
#define swcLeonReadNoCacheU64(addr) \
  ({ u64 result; \
     asm volatile( "ldda  [%1] 1, %0" : "=r"(result) : "r"((addr))); \
     result; })


/// Reads data bypassing leon L1 cache
/// @param[in] addr s64 address to read
/// @return - s64 variable value read bypassing cache
#define swcLeonReadNoCacheI64(addr) ((s64)swcLeonReadNoCacheU64(addr))


/// Writes data bypassing leon LRAM cache
/// @param[in] addr - u32 address to write
/// @param[in] data - i8/u8 variable to write
#define swcLeonWriteNoCache8(addr,data) \
  asm volatile( "stba %1, [%0] 1" :: "r"(addr), "r"(data) )


/// Writes data bypassing leon LRAM cache
/// @param[in] addr - u32 address to write
/// @param[in] data - i16/u16 variable to write
#define swcLeonWriteNoCache16(addr,data) \
  asm volatile( "stha %1, [%0] 1" :: "r"(addr), "r"(data) )


/// Writes data bypassing leon LRAM cache
/// @param[in] addr - u32 address to write
/// @param[in] data - s32/u32 variable to write
#define swcLeonWriteNoCache32(addr,data) \
  asm volatile( "sta %1, [%0] 1" :: "r"(addr), "r"(data) )


/// Writes data bypassing leon L1 cache
/// @param[in] addr - u64 address to write
/// @param[in] data - s64/u64 variable to write
#define swcLeonWriteNoCache64(addr,data) \
  asm volatile( "stda  %0, [%1] 1" :: "r"((u64)(data)), "r"((addr)))


/// Flush Leon Instruction and Data Caches
#define swcLeonFlushCaches() \
  asm volatile( "flush" ::: "memory" )


/// Flush Leon Data Cache
#define swcLeonDataCacheFlush()                       \
  asm volatile(                                    \
    "sta %%g0, [%%g0] %[dcache_flush_asi]  \n\t"   \
    :: [dcache_flush_asi] "I" (__DCACHE_FLUSH_ASI) \
    : "memory"                                     \
  )

#define swcLeonFlushDcache() swcLeonDataCacheFlush()
#define swcLeonDataCacheFlushNoWait() swcLeonDataCacheFlush()

/// Flush Leon Instruction Cache
#define swcLeonInstructionCacheFlush()            \
  asm volatile(                         \
    "lda [%%g0] %1, %%g1; "             \
    "bset %0, %%g1; "                   \
    "sta %%g1, [%%g0] %1"               \
    :                                   \
    : "r"(CCR_FI), "I"(__CCR_ASI)       \
    : "%g1"                             \
  )

#define swcLeonFlushIcache() swcLeonInstructionCacheFlush()

/// Check if Leon cache flush is pending
#define swcLeonIsCacheFlushPending()    \
  ({ unsigned t;                        \
     asm volatile(                      \
       "lda [%%g0] %1, %0"              \
       : "=r"(t)                        \
       : "I"(__CCR_ASI) );              \
     (t>>14)&3; })

/// Enable Leon Instruction and Data Caches
/// @param[in] flush flag: 0 = don't flush cache ; 1 = flush cache
#ifndef MYRIAD2
#define swcLeonEnableCaches(flush)                            \
  asm volatile(                                               \
    "sta %0, [%%g0] %1"                                       \
    :                                                         \
    : "r"(CCR_IB | CCR_ICS_ENABLED | CCR_DCS_ENABLED |  \
            ((flush)?(CCR_FI | CCR_FD):0)), "I"(__CCR_ASI)      \
  )
#else
#define swcLeonEnableCaches(flush)                            \
  asm volatile(                                               \
    "sta %0, [%%g0] %1"                                       \
    :                                                         \
    : "r"(CCR_ICS_ENABLED | CCR_DCS_ENABLED |  \
            ((flush)?(CCR_FI | CCR_FD):0)), "I"(__CCR_ASI)      \
  )
#endif

/// Enable Leon Instruction Cache
/// @param[in] flush flag: 0 = don't flush cache ; 1 = flush cache
#ifndef MYRIAD2
#define swcLeonEnableIcache(flush)      \
  asm volatile(                         \
    "lda [%%g0] %1, %%g1; "             \
    "bset %0, %%g1; "                   \
    "sta %%g1, [%%g0] %1"               \
    :                                   \
    : "r"(CCR_IB | CCR_ICS_ENABLED | ((flush)?CCR_FI:0)), "I"(__CCR_ASI) \
    : "%g1"                             \
  )
#else
#define swcLeonEnableIcache(flush)      \
  asm volatile(                         \
    "lda [%%g0] %1, %%g1; "             \
    "bset %0, %%g1; "                   \
    "sta %%g1, [%%g0] %1"               \
    :                                   \
    : "r"(CCR_ICS_ENABLED | ((flush)?CCR_FI:0)), "I"(__CCR_ASI) \
    : "%g1"                             \
  )
#endif


/// Enable Leon Data Cache
/// @param[in] flush flag: 0 = don't flush cache ; 1 = flush cache
#define swcLeonEnableDcache(flush)      \
  asm volatile(                         \
    "lda [%%g0] %1, %%g1; "             \
    "bset %0, %%g1; "                   \
    "sta %%g1, [%%g0] %1"               \
    :                                   \
    : "r"((CCR_DCS_ENABLED)|((flush)?CCR_FD:0)), "I"(__CCR_ASI) \
    : "%g1"                             \
  )


/// Disable Leon Instruction and Data Caches
#define swcLeonDisableCaches()          \
  asm volatile( "sta %g0, [%g0] 2" )


/// Disable Leon Data Cache
#define swcLeonDisableDcache()          \
  asm volatile(                         \
    "lda [%%g0] %1, %%g1; "             \
    "bclr %0, %%g1; "                   \
    "sta %%g1, [%%g0] %1"               \
    :: "r"(CCR_DCS_ENABLED), "I"(__CCR_ASI) : "%g1" )


/// Disable Leon Instruction Cache
#ifndef MYRIAD2
#define swcLeonDisableIcache()          \
  asm volatile(                         \
    "lda [%%g0] %1, %%g1; "             \
    "bclr %0, %%g1; "                   \
    "sta %%g1, [%%g0] %1"               \
    :: "r"(CCR_IB | CCR_ICS_ENABLED), "I"(__CCR_ASI) : "%g1" )
#else
#define swcLeonDisableIcache()          \
  asm volatile(                         \
    "lda [%%g0] %1, %%g1; "             \
    "bclr %0, %%g1; "                   \
    "sta %%g1, [%%g0] %1"               \
    :: "r"(CCR_ICS_ENABLED), "I"(__CCR_ASI) : "%g1" )
#endif

/// Flushes Leon data cache, and wait while the flush is pending. (DO NOT USE)
///
/// It is not recommended to use this function
/// Leon DCache flush takes 128 cycles as it processes
/// each line of the cache at 1 cycle per line
/// There is no advantage to wait until the flush is not pending anymore.
/// use the swcLeonDataCacheFlush() macro instead.
void swcLeonDataCacheFlushBlockWhilePending(void);

/// Stops Leon
void swcLeonHalt(void);

/// Sets the Processor Interrupt Level atomically
///
/// @param[in] pil - processor interrupt level
/// @return        - previous processor interrupt level
int swcLeonSetPIL( u32 pil);  // in traps.S

/// Get IRQ Level
/// Returns PIL field from the Leon's PSR.
/// @return current PIL field
static inline u32 swcLeonGetPIL() {
  u32 t;
  _ASM( "rd %%psr, %0\n"
        "and %0, %1, %0\n"
        "srl %0, %2, %0\n"
        : "=r"(t) : "I"(MASK_PSR_PIL), "I"(POS_PSR_PIL) );
  return t;
}

/// Flushes all the interrupt windows before the caller's to the stack
///  (you'd ideally call this before your main app loop, if any -
///  allows you to avoid window_overflow's for the next 6-deep calls)
void swcLeonFlushWindows();

/// Disable traps
///  enter/leave a critical section in a clean way - do NOT call any function
///  between these!!!
/// @return 1 if traps were enable, 0 if traps were not enabled
int swcLeonDisableTraps(); // in traps.S

/// Enable traps
///  enter/leave a critical section in a clean way - do NOT call any function
///  between these!!!
/// @return 1 if traps were enable, 0 if traps were not enabled
int swcLeonEnableTraps(); // in traps.S

/// Read the ET flag from Leon's PSR
/// @return A 32 bit value representative of ET value. If !=0 then Traps Are ENABLED
static inline u32 swcLeonAreTrapsEnabled() {
  u32 t;
  // read current PSR and mask out everything except ET
  _ASM( 
    "rd %%psr, %0 \n"
    "and %0, %1, %0 \n" 
    : "=r"(t)
    : "I"(PSR_ET)
  );
  return t;
}

/// Read the value of the PSR register
static inline u32 swcLeonGetPSR() {
  u32 t;
  _ASM( "rd %%psr, %0\n"
        : "=r"(t) );
  return t;
}

/// Sets correct byte depending on endianess
/// @param[in] pointer void* pointer to byte to set
/// @param[in] pt pointer_type be_pointer or le_pointer
/// @param[in] value u8 value to set
void swcLeonSetByte(void *pointer, pointer_type pt, u8 value);

/// Gets correct byte depending on endianess
/// @param[in] pointer void* pointer to byte to set
/// @param[in] pt pointer_type be_pointer or le_pointer
u8 swcLeonGetByte(void *pointer, pointer_type pt);

/// Sets correct byte depending on endianess
/// @param[in] pointer void* pointer to byte to set
/// @param[in] pt pointer_type be_pointer or le_pointer
/// @param[in] value u8 value to set
void swcLeonSetHalfWord(void *pointer, pointer_type pt, u16 value);

/// Gets correct byte depending on endianess
/// @param[in] pointer void* pointer to byte to set
/// @param[in] pt pointer_type be_pointer or le_pointer
u16 swcLeonGetHalfWord(void *pointer, pointer_type pt);


/// Generic memory copying function to copy le/be buffers to le/be buffers
///
/// The buffers may be unaligned, and they may have an unaligned size
/// The buffers may be anywhere in memory, data is accessed using word-access only
/// The buffers may not overlap! If you need overlapping buffers, then see swcLeonMemMove().
/// Exceptions to the no-overlap rule:
/// 2) same endianness buffers may overlap if you know for sure that the destination
///   will always be before the source (meaning (u32)src >= (u32)dst),
///   assert(src >= dst);
/// 3) different endianness buffers may overlap if (u32)src >= (u32)dst + 3
///    if (src_pt != dst_pt) assert(src >= dst + 3);
/// @param[out] dst The destination buffer.
/// @param[in] dst_pt The endianness of the destination buffer
/// @param[in] src The source buffer
/// @param[in] src_pt The endianness of the source buffer
/// @param[in] count Number of bytes to copy. It is not required for this to be divisible by 4.
void swcLeonMemCpy(void *dst, pointer_type dst_pt, const void *src, pointer_type src_pt, u32 count);

/// Same as swcLeonMemCpy, except buffers may overlap.
/// The distance between overlapping buffer pointers of opposite endianness must be >= 3
///
/// if (src_pt != dst_pt) assert( abs(src - dst) >=3 );
/// @param[out] dst The destination buffer.
/// @param[in] dst_pt The endianness of the destination buffer
/// @param[in] src The source buffer
/// @param[in] src_pt The endianness of the source buffer
/// @param[in] count Number of bytes to copy. It is not required for this to be divisible by 4.
void swcLeonMemMove(void *dst, pointer_type dst_pt, const void *src, pointer_type src_pt, u32 count);

/// Swap the endianness of a buffer in place
///
/// The buffer pointer and count may be unaligned, but you have to make sure
/// that sufficient bytes are aligned before and after the buffer, to fit the flipped buffer.
/// @param[inout] buf Buffer to work on
/// @param[in] pt initial endianness of the buffer
/// @param[in] count number of bytes
void swcLeonSwapBuffer(void *buf, pointer_type pt, u32 count);

/// 32 bit read from the Leon Asr17 (Processor Configuration Register)
/// @return - 32 bit value read back
static inline unsigned int swcLeonRead32Asr17(void)
{
 unsigned int pcrValue; // used for returned value
 asm volatile ("rd %%asr17,%0\n\t" :
                "=&r" (pcrValue) );
return pcrValue;
}

/// 32 bit read from Processor Configuration Register
/// @return - 32 bit value read back
static inline unsigned int swcLeonGetProcessorConfig(void)
{
    return swcLeonRead32Asr17();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Leon Address Space Access Helper functions
//  The following functions allow C level access to the different address spaces exposed
//  by the Leon processor
//  Note: There is some duplication between the address spaces accessed below and the application specific functionality
//  implemented above (e.g. Cache control). Where appropriate the application specific functions should be used.
//  The functions below allow 32 bit r/w access to all address spaces with in orthogonal API.
//  
//  Summary of Leon Address Spaces
//  0x01  -- Forced Cache Miss
//  0x02  -- Cache Control Registers
//  0x08  -- Normal Cached Access (Replace if cacheable)
//  0x09  -- Normal Cached Access (Replace if cacheable) 
//  0x0A  -- Normal Cached Access (Replace if cacheable) 
//  0x0B  -- Normal Cached Access (Replace if cacheable) 
//  0x0C  -- L1 Instruction Cache Tags
//  0x0D  -- L1 Instruction Cache Data
//  0x0E  -- L1 Data Cache Tags
//  0x0F  -- L1 Data Cache Data
//  0x10  -- Flush Instruction Cache // MMU Flush
//  0x11  -- Flush Data Cache
//  0x13  -- MMU Flush context
//  0x14  -- MMU Diagnostic DCache Context Access
//  0x15  -- MMU Diagnostic ICache Context Access
//  0x19  -- MMU Registers
//  0x1C  -- MMU Cache Bypass
//  0x1D  -- MMU Diagnostic Access
//  0x1E  -- MMU Snoop tags diag access
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/// 32 bit write to the Leon 0x1 Address space (Forced Cache Miss)
/// @param[in]   addr address where to write 
/// @param[in]  val 32 bit value to write
static inline void swcWrite32Asi01(u32 addr, u32 val)
{
 asm volatile (" sta %0, [%1] 0x01 " // store val to addr at ASI 0x01
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 32 bit read from the Leon 0x1 Address space (Forced Cache Miss) 
/// @param[in]   addr address where to read from
/// @return - 32 bit value read back
static inline unsigned int swcRead32Asi01(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" lda [%1] 0x01, %0 " // load tmp from addr at ASI 0x01
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}


/// 32 bit write to the Leon 0x2 Address space (Cache Control Registers)
/// @param[in]   addr address where to write
/// @param[in] val 32 bit value to write
static inline void swcWrite32Asi02(u32 addr, u32 val)
{
 asm volatile (" sta %0, [%1] 0x02 " // store val to addr at ASI 0x02
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 32 bit read from the Leon 0x2 Address space (Cache Control Registers) 
/// @param[in]   addr address where to read from
/// @return  32 bit value read back
static inline unsigned int swcRead32Asi02(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" lda [%1] 0x02, %0 " // load tmp from addr at ASI 0x02
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}

/// 32 bit write to the Leon 0xC Address space (L1 ICache Tags)
/// @param[in]   addr address where to write
/// @param[in] val 32 bit value to write
static inline void swcWrite32Asi0C(u32 addr, u32 val)
{
 asm volatile (" sta %0, [%1] 0x0C " // store val to addr at ASI 0x0C
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 32 bit read from the Leon 0xC Address space (L1 ICache Tags) 
/// @param[in]   addr address where to read from
/// @return 32 bit value read back
static inline unsigned int swcRead32Asi0C(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" lda [%1] 0x0C, %0 " // load tmp from addr at ASI 0x0C
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}


/// 32 bit write to the Leon 0xD Address space (L1 ICache Data)
/// @param[in]   addr address where to write
/// @param[in]  val 32 bit value to write
static inline void swcWrite32Asi0D(u32 addr, u32 val)
{
 asm volatile (" sta %0, [%1] 0x0D " // store val to addr at ASI 0x0D
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 32 bit read from the Leon 0xD Address space (L1 ICache Data) 
/// @param[in]   addr address where to read from
/// @return 32 bit value read back
static inline unsigned int swcRead32Asi0D(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" lda [%1] 0x0D, %0 " // load tmp from addr at ASI 0x0D
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}


/// 32 bit write to the Leon 0xE Address space (L1 DCache Tags)
/// @param[in]   addr address where to write
/// @param[in]  val 32 bit value to write
static inline void swcWrite32Asi0E(u32 addr, u32 val)
{
 asm volatile (" sta %0, [%1] 0x0E " // store val to addr at ASI 0x0E
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 32 bit read from the Leon 0xE Address space (L1 DCache Tags) 
/// @param[in]   addr address where to read from
/// @return 32 bit value read back
static inline unsigned int swcRead32Asi0E(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" lda [%1] 0x0E, %0 " // load tmp from addr at ASI 0x0E
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}

/// 32 bit write to the Leon 0xF Address space (L1 DCache Data)
/// @param[in]   addr address where to write
/// @param[in] val 32 bit value to write
static inline void swcWrite32Asi0F(u32 addr, u32 val)
{
 asm volatile (" sta %0, [%1] 0x0F " // store val to addr at ASI 0x0F
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 32 bit read from the Leon 0xF Address space (L1 DCache Tags) 
/// @param[in]   addr address where to read from
/// @return 32 bit value read back
static inline unsigned int swcRead32Asi0F(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" lda [%1] 0x0F, %0 " // load tmp from addr at ASI 0x0F
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}

/// 32 bit write to the Leon 0x10 Address space (Flush L1 ICache // MMU Flush Page)
/// @param[in]   addr address where to write
/// @param[in]  val 32 bit value to write
static inline void swcWrite32Asi10(u32 addr, u32 val)
{
 asm volatile (" sta %0, [%1] 0x10 " // store val to addr at ASI 0x10
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 32 bit read from the Leon 0x10 Address space (Flush L1 ICache // MMU Flush Page) 
/// @param[in]   addr address where to read from
/// @return 32 bit value read back
static inline unsigned int swcRead32Asi10(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" lda [%1] 0x10, %0 " // load tmp from addr at ASI 0x10
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}

/// 32 bit write to the Leon 0x11 Address space (Flush L1 DCache)
/// @param[in] addr address where to write
/// @param[in] val 32 bit value to write
static inline void swcWrite32Asi11(u32 addr, u32 val)
{
 asm volatile (" sta %0, [%1] 0x11 " // store val to addr at ASI 0x11
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 32 bit read from the Leon 0x11 Address space (Flush L1 DCache) 
/// @param[in] addr address where to read from
/// @return 32 bit value read back
static inline unsigned int swcRead32Asi11(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" lda [%1] 0x11, %0 " // load tmp from addr at ASI 0x11
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}

/// 32 bit write to the Leon 0x13 Address space (MMU Flush context)
/// @param[in]   addr address where to write
/// @param[in] val 32 bit value to write
static inline void swcWrite32Asi13(u32 addr, u32 val)
{
 asm volatile (" sta %0, [%1] 0x13 " // store val to addr at ASI 0x13
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 32 bit read from the Leon 0x13 Address space (MMU Flush context) 
/// @param[in]  addr address where to read from
/// @return 32 bit value read back
static inline unsigned int swcRead32Asi13(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" lda [%1] 0x13, %0 " // load tmp from addr at ASI 0x13
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}                                               

/// 32 bit write to the Leon 0x14 Address space (MMU Diag dcache context access)
/// @param[in]   addr address where to write
/// @param[in] val 32 bit value to write
static inline void swcWrite32Asi14(u32 addr, u32 val)
{
 asm volatile (" sta %0, [%1] 0x14 " // store val to addr at ASI 0x14
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 32 bit read from the Leon 0x14 Address space (MMU Diag dcache context access) 
/// @param[in]   addr address where to read from
/// @return 32 bit value read back
static inline unsigned int swcRead32Asi14(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" lda [%1] 0x14, %0 " // load tmp from addr at ASI 0x14
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}                                               

/// 32 bit write to the Leon 0x15 Address space (MMU Diag icache context access)
/// @param[in]   addr address where to write
/// @param[in] val 32 bit value to write
static inline void swcWrite32Asi15(u32 addr, u32 val)
{
 asm volatile (" sta %0, [%1] 0x15 " // store val to addr at ASI 0x15
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 32 bit read from the Leon 0x15 Address space (MMU Diag icache context access) 
/// @param[in]   addr address where to read from
/// @return 32 bit value read back
static inline unsigned int swcRead32Asi15(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" lda [%1] 0x15, %0 " // load tmp from addr at ASI 0x15
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}                                               


/// 32 bit write to the Leon 0x19 Address space (MMU Register access)
/// @param[in]   addr address where to write
/// @param[in] val 32 bit value to write
static inline void swcWrite32Asi19(u32 addr, u32 val)
{
 asm volatile (" sta %0, [%1] 0x19 " // store val to addr at ASI 0x19
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 32 bit read from the Leon 0x19 Address space (MMU Register access) 
/// @param[in]   addr address where to read from
/// @return 32 bit value read back
static inline unsigned int swcRead32Asi19(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" lda [%1] 0x19, %0 " // load tmp from addr at ASI 0x19
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}                                               


/// 32 bit write to the Leon 0x1C Address space  (MMU Bypass)
/// @param[in]   addr address where to write
/// @param[in] val 32 bit value to write
static inline void swcWrite32Asi1C(u32 addr, u32 val)
{
 asm volatile (" sta %0, [%1] 0x1C " // store val to addr at ASI 0x1C
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 32 bit read from the Leon 0x1C Address space   (MMU Bypass) 
/// @param[in]   addr address where to read from
/// @return 32 bit value read back
static inline unsigned int swcRead32Asi1C(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" lda [%1] 0x1C, %0 " // load tmp from addr at ASI 0x1C
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}                                               

/// 64 bit write to the Leon 0x1C Address space  (MMU Bypass)
/// @param[in]   addr address where to write
/// @param[in] val 64 bit value to write
static inline void swcWrite64Asi1C(u32 addr, u64 val)
{
 asm volatile (" stda %0, [%1] 0x1C " // store val to addr at ASI 0x1C
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 64 bit read from the Leon 0x1C Address space   (MMU Bypass) 
/// @param[in]   addr address where to read from
/// @return 64 bit value read back
static inline u64 swcRead64Asi1C(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" ldda [%1] 0x1C, %0 " // load tmp from addr at ASI 0x1C
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}                                               


/// 32 bit write to the Leon 0x1D Address space  (MMU Diag access)
/// @param[in]   addr address where to write
/// @param[in] val 32 bit value to write
static inline void swcWrite32Asi1D(u32 addr, u32 val)
{
 asm volatile (" sta %0, [%1] 0x1D " // store val to addr at ASI 0x1D
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 32 bit read from the Leon 0x1D Address space  (MMU Diag access) 
/// @param[in]   addr address where to read from
/// @return 32 bit value read back
static inline unsigned int swcRead32Asi1D(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" lda [%1] 0x1D, %0 " // load tmp from addr at ASI 0x1D
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}                                               



/// 32 bit write to the Leon 0x1E Address space  (MMU snoop Diag access)
/// @param[in]   addr address where to write 
/// @param[in] val 32 bit value to write
static inline void swcWrite32Asi1E(u32 addr, u32 val)
{
 asm volatile (" sta %0, [%1] 0x1E " // store val to addr at ASI 0x1E
 : // output
 : "r" (val), "r" (addr) // inputs
);

}
/// 32 bit read from the Leon 0x1E Address space  (MMU snoop  Diag access) 
/// @param[in]   addr address where to read from
/// @return 32 bit value read back
static inline unsigned int swcRead32Asi1E(u32 addr)
{
 unsigned int tmp; // used for returned value
 asm volatile (" lda [%1] 0x1E, %0 " // load tmp from addr at ASI 0x1E
 : "=r" (tmp) // output
 : "r" (addr) // input
 );
return tmp;
}                                               


#endif //__LEON_UTILS_H__
