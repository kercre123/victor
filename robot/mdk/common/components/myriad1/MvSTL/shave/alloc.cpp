#include <mvstl_utils.h>
#include <mvstl_types.h>
#include <string.h>
#include <stdlib.h>
#include <alloc.h>
#include <assert.h>

#define ALLOC_NO_PRINT

#ifdef ALLOC_NO_PRINT
#define MALLOC_PRINTF(...)
#define FREE_PRINTF(...)
#else
#define MALLOC_PRINTF(...) printf(__VA_ARGS__)
#define FREE_PRINTF(...) printf(__VA_ARGS__)
#endif

// Define possible error codes for failure assessment
enum cause_of_death 
{
	BAD_ALIGN		= 0x6660, 
	ZERO_SIZE		= 0x6661, 
	MCB_INVALID		= 0x6662, 
	OUT_OF_MEMORY	= 0x6663, 
	HEAP_OVERFLOW	= 0x6664, 
	FREE_NULL_PTR	= 0x6665, 
	BAD_FREE		= 0x6666
};

// Padding value for heap start alignment. Also used as a marker
// to detect overflows
#define PAD_VAL (0x69)

// Heap pointer
static u8* heap;
// Heap size
static u32 heap_size;

// Heap start is aligned to 64bit and doesn't include padding
static u32* MVSTL_STATIC_DATA __heap_start__;
// Heap end is not aligned and doesn't include padding
static u32* MVSTL_STATIC_DATA __heap_end__;

// Profiling variables
static u32 MVSTL_STATIC_DATA now_allocated;
static u32 MVSTL_STATIC_DATA max_allocated;

static int align(int size, int align)
{
    return (size + align - 1) & -align;
}

/**
 * Initializes the heap with a static buffer and its size.
 */
void init_heap(u8* heap_start, size_t heap_size)
{
	int i;
	u8* pad;

	heap = heap_start;
	heap_size = heap_size;
	
	__heap_start__  = (u32*)align((int)(heap_start + 4), 8);
	__heap_end__	= (u32*)align((int)((u8*)__heap_start__  + heap_size - 12), 8);

	// Wrap the heap with markers to detect overflows
	pad = heap_start;
	for (i = 0; i < (u8*)__heap_start__ - heap_start; i++)
		pad[i] = PAD_VAL;

	pad = (u8*)__heap_end__;
	for (i = 0; i < heap_start + heap_size - (u8*)__heap_end__; i++)
		pad[i] = PAD_VAL;

	// Reset counters
	now_allocated = 0;
	max_allocated = 0;
}

/**
 * Checks begining and ending markers of the heap to detect any overflows.
 * It should be called only after the heap has been initialized and used.
 */
void check_heap()
{
	int i;
	u8* pad;

	MALLOC_PRINTF("Peak memory allocation: %d bytes\n", max_allocated);

	// Check the existence of markers around heap
	pad = heap;
	for (i = 0; i < (u8*)__heap_start__ - heap; i++)
		if (pad[i] != PAD_VAL)
			exit(HEAP_OVERFLOW);

	pad = (u8*)__heap_end__;
	for (i = 0; i < heap + heap_size - (u8*)__heap_end__; i++)
		if (pad[i] != PAD_VAL)
			exit(HEAP_OVERFLOW);

	if (now_allocated != 0)
		exit(BAD_FREE);
}

/**
 * Clears the heap area to 0. The heap must be initialized first.
 */
void clear_heap()
{
	memset(__heap_start__, 0xCA, __heap_end__ - __heap_start__);
}

/**
 * Allocates a block of memory from the heap. The heap must be initialized.
 */
void* mv_malloc(size_t size)
{
	MCB_p t;

	// initialize heap, if need be (only on first call)
	if(( *(u32*)__heap_start__ ) != 'LMCB' ) 
	{
		*(u32*)__heap_start__ = 'LMCB';
		*(1+(u32*)__heap_start__) = (u32)__heap_end__ - (u32)__heap_start__;
	}

	if( !size )
		exit(ZERO_SIZE);
		//return 0;

	// include MCB's header in size, and align to 8n
	size =( size + 8 + 7 )& ~7;

	// count currently and peak allocated memory
	now_allocated += size;
	max_allocated = MAX(now_allocated, max_allocated);

	// start scanning the MCBs
	t = (MCB_p)__heap_start__;

	while( (u32)t < (u32)__heap_end__ ) 
	{
		u32 bsize = t->used_size & ~0x80000000;
		MALLOC_PRINTF( "  @0x%08X { size=0x%X, %sused }: ", t,  bsize, ( t->used_size >> 31 )?"":"not " );
		
		// if the current block is invalid, fail
		if( t->magic != 'LMCB' ) 
		{
			MALLOC_PRINTF( "INVALID\n" );
			exit(MCB_INVALID);
			//return 0;
		}
		
		// if the current block is free
		if( !( t->used_size >> 31 )) 
		{
			// if the current block is large enough
			if( bsize >= size ) 
			{
				// create a new MCB with the remaining memory
				u32 next_size = t->used_size - size;
				MALLOC_PRINTF( "ok, left=0x%X", next_size );
				
				// IF there's any memory left
				if( next_size ) 
				{
					// create a new free MCB
					*(u32*)(size+(u32)t) = 'LMCB';
					*(1+(u32*)(size+(u32)t)) = next_size;
					MALLOC_PRINTF( "; next@0x%08X\n", size+(u32)t );
				}
				else
					MALLOC_PRINTF( "; end\n", size+(u32)t );

				// change the size of the current block
				t->used_size = 0x80000000 | size;
				//// zero-fill it
				//memsetx( t->data, 0, size-4 );

				// return the allocated block
				return &t->data;
			}
		}
		else
		{
			MALLOC_PRINTF( "skipping\n" );
		}

		t = (MCB_p)( bsize + (u32)t );
	}

	MALLOC_PRINTF( "\n" );

	// if we got here, no block is large enough, so fail
	exit(OUT_OF_MEMORY);
	return 0;
}

/**
 * Frees a block of memory.
 */
void mv_free(void* ptr)
{
	MCB_p t;
	MCB_p t2;

	if (ptr == null)
		exit(FREE_NULL_PTR);

	// check that the pointer is aligned properly
	if( (u32)ptr & 7 )
		exit(BAD_ALIGN);
		//return;

	// check that the pointer is inside the heap
	t = (MCB_p)((u32)ptr - 8 );
	if( (u32)t < (u32)__heap_start__ || (u32)t >= (u32)__heap_end__ )
		return;

	// pass 1: free the block
	t->used_size &= ~0x80000000;
	FREE_PRINTF("free block @0x%08X, size=0x%x\n", t, t->used_size);

	// count currently allocated memory
	now_allocated -= t->used_size;
	assert(now_allocated >= 0 && "Atempting to free more memory than it was allocated");

	// pass 2: defragment the free memory
	t = (MCB_p)__heap_start__;
	t2 = (MCB_p)(( t->used_size & ~0x80000000 )+ (u32)t );

	while( (u32)t2 < (u32)__heap_end__ ) 
	{
		// abort if we went past ptr
		if( (u32)t > (u32)ptr )
			return;

		// if the blocks at t and t2 are both free
		if( !( t->used_size & 0x80000000 ) &&
			!( t2->used_size & 0x80000000 )) 
		{
			FREE_PRINTF( "  free() joining @0x%08X and @0x%08X\n", t, t2 );
			// join them 
			t->used_size += t2->used_size;
			t2->magic = 'HEAP';
			t2->used_size = 'HEAP';
		}
		else
		{
			// go to the next
			t = t2;
		}

		t2 = (MCB_p)(( t->used_size & ~0x80000000 )+ (u32)t );
	}
}

/**
 * Reallocates a block of memory from the heap, freeing the old block.
 */
void* mv_realloc(void* ptr, size_t size)
{
	const MCB_p t = (MCB_p)((u32)ptr - 8);
	size_t old_size;
	void* ptr_new = NULL;

	if (ptr == null)
		exit(FREE_NULL_PTR);

	// check that the pointer is aligned properly
	if ((u32)ptr & 7)
		exit(BAD_ALIGN);
		//return NULL;

	// check that the pointer is inside the heap
	if ((u32)t < (u32)__heap_start__ || (u32)t >= (u32)__heap_end__)
		return NULL;

	// If size is zero, the function behaves like the free function
	if (size != 0)
	{
		// Get the size of the old allocated block
		old_size = t->used_size & ~0x80000000;

		// Try to allocate memory for the new block
		if ((ptr_new = mv_malloc(size)) == NULL)
			return NULL;

		// Copy over content from the old location to the newly
		// allocated one
		memcpy(ptr_new, ptr, old_size);
	}
	
	// Release old block
	mv_free(ptr);

	return ptr_new;
}

void smartCopy(u8* dst, u8* src, u32 numBytes, u32 width, s32 stride)
{
	//s32 dstaddr = (s32)dst;
	//s32 srcaddr = (s32)src;

	//assert((srcaddr >= 0x48000000 && srcaddr <= 0x4C000000) || 
	//	(srcaddr >= 0x40000000 && srcaddr <= 0x44000000) && 
	//	"Source address is not DDR");

	//assert(((dstaddr & 0xFF000000) == 0x1C000000) ||
	//	((dstaddr & 0xFF000000) == 0x1E000000) ||
	//	((dstaddr & 0xFF000000) == 0x10000000) &&
	//	"Destination address is not CMX");

	if (IS_DDR(src))
	{
		scDmaSetupStrideSrc(DMA_TASK_0, src, dst, numBytes, (width == 0) ? numBytes : width, stride);
		scDmaStart(DMA_TASK_0);
	}
	else
	{
		u32 height = (width == 0) ? 1 : (numBytes / width);
		u32 len = (width == 0) ? numBytes : width;
		for (u32 i = 0; i < height; i++, src += (len + stride), dst += len)
		{
			memcpy(dst, src, len);
		}
	}
}