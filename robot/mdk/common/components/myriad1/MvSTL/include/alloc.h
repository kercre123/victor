#ifndef __ALLOC_H__
#define __ALLOC_H__

#include "mvstl_types.h"

#ifdef __MOVICOMPILE__
//#define MVSTL_STATIC_DATA __attribute((section(".mvstl_static_data")))
#define MVSTL_STATIC_DATA
#else
#define MVSTL_STATIC_DATA
#endif

typedef struct 
{
	u32 magic;		// 'LMCB'
	u32 used_size;	// [31]: `used` bit; [30:0]: size (incl header)
	u32 data;		// This is the first location of the allocated block
} MCB_t;

typedef MCB_t* MCB_p;

void init_heap(u8* heap_start, size_t heap_size);
void check_heap();
void clear_heap();

void* mv_malloc(size_t size);
void mv_free(void *ptr);
void* mv_realloc(void* ptr, size_t size);

void smartCopy(u8* dst, u8* src, u32 numBytes, u32 width = 0, s32 stride = 0);

#endif
