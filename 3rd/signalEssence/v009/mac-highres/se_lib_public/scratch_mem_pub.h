#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
  (C) Copyright 2009 SignalEssence; All Rights Reserved

  Module Name  - scratch_mem.c

  Author: Robert Yu

***************************************************************************/
#ifndef SCRATCH_MEM_PUB_H
#define SCRATCH_MEM_PUB_H
#include "se_types.h"

#include <stddef.h>


//
// configuration parameters for scratch memory
typedef struct
{
    char *pBufH;
    size_t lenBufH;

    char *pBufX;
    size_t lenBufX;
    
    char *pBufS;
    size_t lenBufS;
} ScratchMemConfig_t;

//
// get highwater marks and current usage
// useful for debug
void ScratchGetHighWaterMarkSummary(size_t *pForX, size_t *pForH, size_t *pForS);
void ScratchGetInUseSummary(size_t *pForX, size_t *pForH, size_t *pForS);

typedef enum {
    POOL_FIRST_DONT_USE,
    POOL_X,
    POOL_H,
    POOL_S,
    POOL_LAST
} ScratchPool_t;

void ScratchMemInit          (ScratchMemConfig_t *pConfig);
void ScratchMemInitBank      (int pool_x_bytes, int pool_h_bytes, int pool_s_bytes);

void ScratchMemDestroy(void);
void ScratchMemDestroyBank(void);

size_t ScratchGetHighWaterMark (ScratchPool_t pool);
size_t ScratchGetInUse         (ScratchPool_t pool);

// uncomment this line to print diagnostics
// when memory is allocd/freed
//#define SCRATCH_MEM_DEBUG_PRINTF 1

#ifndef SCRATCH_MEM_DEBUG_PRINTF
/*
 * non debug versions of allocators
 */
void*  ScratchGetBuf_real              (ScratchPool_t pool, size_t numRequested);
void*  ScratchGetBufZ_real             (ScratchPool_t pool, size_t numRequested);
int16  ScratchReleaseBuf_real          (ScratchPool_t pool, void* pBuf);
void   ScratchCheckBuf_real            (ScratchPool_t pool, void *pBuf);

#define ScratchGetBuf(pool, len)       ScratchGetBuf_real(pool, len)
#define ScratchGetBufX(len) 	       ScratchGetBuf_real(POOL_X, len)
#define ScratchGetBufH(len) 	       ScratchGetBuf_real(POOL_H, len)
#define ScratchGetBufS(len) 	       ScratchGetBuf_real(POOL_S, len)

// zeroing allocators
#define ScratchGetBufZ(pool, len)      ScratchGetBufZ_real(pool, len)
#define ScratchGetBufZX(len)           ScratchGetBufZ_real(POOL_X, len)
#define ScratchGetBufZH(len)           ScratchGetBufZ_real(POOL_H, len)
#define ScratchGetBufZS(len)           ScratchGetBufZ_real(POOL_S, len)

#define ScratchReleaseBuf(pool, pBuf)  ScratchReleaseBuf_real(pool, pBuf)
#define ScratchReleaseBufX(pBuf)       ScratchReleaseBuf_real(POOL_X,     pBuf)
#define ScratchReleaseBufH(pBuf)       ScratchReleaseBuf_real(POOL_H,     pBuf)
#define ScratchReleaseBufS(pBuf)       ScratchReleaseBuf_real(POOL_S,     pBuf)

#define ScratchCheckBuf(pool, pBuf)    ScratchCheckBuf_real(pool  , pBuf)
#define ScratchCheckBufX(pBuf)         ScratchCheckBuf_real(POOL_X, pBuf)
#define ScratchCheckBufH(pBuf)         ScratchCheckBuf_real(POOL_H, pBuf)
#define ScratchCheckBufS(pBuf)         ScratchCheckBuf_real(POOL_S, pBuf)

#else

/*
 * debug versions of allocators
 */
void*  ScratchGetBuf_real         (ScratchPool_t pool, size_t numRequested, const char *, int);
void*  ScratchGetBufZ_real        (ScratchPool_t pool, size_t numRequested, const char *, int);
int16  ScratchReleaseBuf_real     (ScratchPool_t pool, void* pBuf, const char *, int);
void   ScratchCheckBuf_real       (ScratchPool_t pool, void *pBuf, const char *, int);

#define ScratchGetBuf(pool, len)  ScratchGetBuf_real(pool  , len, __FILE__, __LINE__)
#define ScratchGetBufX(len) 	  ScratchGetBuf_real(POOL_X, len, __FILE__, __LINE__)
#define ScratchGetBufH(len) 	  ScratchGetBuf_real(POOL_H, len, __FILE__, __LINE__)
#define ScratchGetBufS(len) 	  ScratchGetBuf_real(POOL_S, len, __FILE__, __LINE__)

#define ScratchGetBufZ(pool, len) ScratchGetBuf_real(pool  , len, __FILE__, __LINE__)  
#define ScratchGetBufZX(len) 	  ScratchGetBuf_real(POOL_X, len, __FILE__, __LINE__)
#define ScratchGetBufZH(len) 	  ScratchGetBuf_real(POOL_H, len, __FILE__, __LINE__)
#define ScratchGetBufZS(len) 	  ScratchGetBuf_real(POOL_S, len, __FILE__, __LINE__)

#define ScratchReleaseBuf(pool, pBuf)  ScratchReleaseBuf_real(pool  , pBuf, __FILE__, __LINE__)
#define ScratchReleaseBufX(pBuf)       ScratchReleaseBuf_real(POOL_X, pBuf, __FILE__, __LINE__)
#define ScratchReleaseBufH(pBuf)       ScratchReleaseBuf_real(POOL_H, pBuf, __FILE__, __LINE__)
#define ScratchReleaseBufS(pBuf)       ScratchReleaseBuf_real(POOL_S, pBuf, __FILE__, __LINE__)

#define ScratchCheckBuf (pool, pBuf) ScratchCheckBuf_real(pool, pbuf, __FILE__, __LINE__)
#define ScratchCheckBufX(pool, pBuf) ScratchCheckBuf_real(POOL_X, pbuf, __FILE__, __LINE__)
#define ScratchCheckBufH(pool, pBuf) ScratchCheckBuf_real(POOL_H, pbuf, __FILE__, __LINE__)
#define ScratchCheckBufS(pool, pBuf) ScratchCheckBuf_real(POOL_S, pbuf, __FILE__, __LINE__)

#endif

#define ScratchGetInUseX() ScratchGetInUse(POOL_X)
#define ScratchGetInUseH() ScratchGetInUse(POOL_H)
#define ScratchGetInUseS() ScratchGetInUse(POOL_S)

#define ScratchGetHighWaterMarkX() ScratchGetHighWaterMark(POOL_X)
#define ScratchGetHighWaterMarkH() ScratchGetHighWaterMark(POOL_H)
#define ScratchGetHighWaterMarkS() ScratchGetHighWaterMark(POOL_S)

//
// for debug:  print pool parameters
void ScratchDumpPoolParams(void);
//
// for debug:  print header & footer info
void ScratchDumpHeaderFooter(char* pa);
int ScratchMemIsInitialized(void);

#ifdef __cplusplus
}
#endif // __cpluspluc
#endif // scratch_mem_pub_h

