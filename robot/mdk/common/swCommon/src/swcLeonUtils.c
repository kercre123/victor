///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     API manipulating Leon functionalities
///
/// Allows manipulating leon caches and other features
///

#include "swcLeonUtils.h"
#include <mv_types.h>

/// Flushes Leon data cache, and wait while the flush is pending. (DO NOT USE)
///
/// It is not recommended to use this function
/// Leon DCache flush takes 128 cycles as it processes
/// each line of the cache at 1 cycle per line
/// There is no advantage to wait until the flash is not pending anymore.
/// use the swcLeonDataCacheFlush() macro instead.
/// @param[in] - none
/// @param[out] - none
void swcLeonDataCacheFlushBlockWhilePending(void)
{
	unsigned int ccr_val_reg;
	unsigned int test_bits_reg;
	asm volatile(
		"    sta %%g0, [%%g0] %[dcache_flush_asi] \n\t"
		"    lda [%%g0] %[ccr_asi], %[ccr_val_reg] \n\t"
		"    set %[anything_pending] , %[test_bits_reg] \n\t"
		".L_test_pending_%=: \n\t"
		"    andcc %[ccr_val_reg], %[test_bits_reg], %%g0 \n\t"
		"    bnz,a .L_test_pending_%= \n\t"
		"      lda [%%g0] %[ccr_asi], %[ccr_val_reg] \n\t"
		:
		  [ccr_val_reg] "=r" (ccr_val_reg),
		  [test_bits_reg] "=r" (test_bits_reg)
		: [dcache_flush_asi] "I" (__DCACHE_FLUSH_ASI),
		  [ccr_asi] "I" (__CCR_ASI),
		  [anything_pending] "i" (CCR_DP | CCR_IP)
		:  "memory"
	);
}


u8 swcLeonGetByte(void *pointer, pointer_type pt) {
    u32 *word_addr = (u32 *)(((unsigned int) pointer) & ~3);
    u32 word = *word_addr;
    int byte_nr = ((unsigned int) pointer) & 3;
    int place;
    if (pt == le_pointer)
        place = byte_nr << 3;
    else
        place = (byte_nr ^ 3) << 3;

    return (word >> place) & 0xFF;
}

void swcLeonSetByte(void *pointer, pointer_type pt, u8 value) {
    u32 *word_addr = (u32 *)(((unsigned int) pointer) & ~3);
    u32 word = *word_addr;
    int byte_nr = ((unsigned int) pointer) & 3;
    int place;
    if (pt == le_pointer)
        place = byte_nr << 3;
    else
        place = (byte_nr ^ 3) << 3;

    *word_addr = (word & ~((0xFF) << place)) | (value << place);
}

u16 swcLeonGetHalfWord(void *pointer, pointer_type pt) {
    u32 *word_addr = (u32 *)(((unsigned int) pointer) & ~3);
    u32 word = *word_addr;
    int byte_nr = ((unsigned int) pointer) & 3;
    int place;
    
    if (byte_nr & 1) asm volatile ("t 10"); // unaligned access!
    
    if (pt == le_pointer)
        place = byte_nr << 3;
    else
        place = (byte_nr ^ 2) << 3;

    return (word >> place) & 0xFFFF;
}

void swcLeonSetHalfWord(void *pointer, pointer_type pt, u16 value) {
    u32 *word_addr = (u32 *)(((unsigned int) pointer) & ~3);
    u32 word = *word_addr;
    int byte_nr = ((unsigned int) pointer) & 3;
    int place;
    
    if (byte_nr & 1) asm volatile ("t 10"); // unaligned access!
    
    if (pt == le_pointer)
        place = byte_nr << 3;
    else
        place = (byte_nr ^ 2) << 3;

    *word_addr = (word & ~((0xFFFF) << place)) | (value << place);
}

void swcLeonMemCpy(void *dst, pointer_type dst_pt, const void *src, pointer_type src_pt, u32 count) {
    u32 s = (u32)src;
    u32 d = (u32)dst;
    while ((s & 3) && (count > 0)) { // get to a point where the source buffer is aligned
        swcLeonSetByte((void *) d, dst_pt, swcLeonGetByte((void *) s, src_pt));
        ++s; ++d; --count;
    }
    if ((d & 3) == 0) { // if both the destination and the source is aligned, then we can do optimized per-word copy;
        while (count >= sizeof(u32)) {
            u32 t = *(u32*)s;
            if (src_pt != dst_pt)
                t = ((t & 0xff) << 24) | ((t & 0xff00) << 8) | ((t & 0xff0000) >> 8) | ((t >> 24) & 0xff);
            *(u32*)d = t;
            s += sizeof(u32); d += sizeof(u32); count -= sizeof(u32);
        }
    }
    // If the two buffers are relatively unaligned, or if less then 4 bytes left
    while (count > 0) {
        swcLeonSetByte((void *) d, dst_pt, swcLeonGetByte((void *) s, src_pt));
        ++s; ++d; --count;
    }
}

void swcLeonMemMove(void *dst, pointer_type dst_pt, const void *src, pointer_type src_pt, u32 count) {
    u32 s = count + (u32)src;
    u32 d = count + (u32)dst;
    if (s>=d) swcLeonMemCpy(dst, dst_pt, src, src_pt, count);
    else {
        while ((s & 3) && (count > 0)) { // get to a point where the source buffer is aligned
            --s; --d; --count;
            swcLeonSetByte((void *) d, dst_pt, swcLeonGetByte((void *) s, src_pt));
        }
        if ((d & 3) == 0) { // if both the destination and the source is aligned, then we can do optimized per-word copy;
            while (count >= sizeof(u32)) {
                s -= sizeof(u32); d -= sizeof(u32); count -= sizeof(u32);
                u32 t = *(u32*)s;
                if (src_pt != dst_pt)
                    t = ((t & 0xff) << 24) | ((t & 0xff00) << 8) | ((t & 0xff0000) >> 8) | ((t >> 24) & 0xff);
                *(u32*)d = t;
            }
        }
        // If the two buffers are relatively unaligned, or if less then 4 bytes left
        while (count > 0) {
            --s; --d; --count;
            swcLeonSetByte((void *) d, dst_pt, swcLeonGetByte((void *) s, src_pt));
        }
    }
}

void swcLeonSwapBuffer(void *buf, pointer_type pt, u32 count) {
    u32 *wbuf = buf;
    if (((u32) wbuf) & 3) {
        count += ((u32) wbuf) & 3;
        wbuf = (u32 *)(((u32)wbuf) & ~3);
    }
    if (count & 3)
        count += ((~count) & 3) + 1;

    while (count) {
        wbuf[0] = swcLeonSwapU32(wbuf[0]);
        wbuf ++;
        count -= sizeof(u32);
    }
}


/// Stops Leon
/// @param[in] - none
/// @param[out] - none
void swcLeonHalt(void){
	((void)(( { asm volatile( "set 1, %g1; ta 0\n" ); } ), 0 ));
	return;
}
