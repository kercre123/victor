#include <sipp.h>

// On Sabre, [Lines buffers] and [scratch memory] get allocated through window_regs_D
// (i.e. 0x1FXXXXXX)
#define LN_POOL_START ((UInt8 *)0x1F000040)

//The SIPP pool contains shared data structures (Leon and Shave can access them)
#ifndef SIPP_POOL_SZ
#define SIPP_POOL_SZ (256*1024)
#endif

#if defined(SIPP_PC)
 UInt8 cmx[16*SIPP_SLICE_SZ]; //on PC, create storage for 16 slices
#endif

//The main memory pool for dynamic allocation that holds pipeline, filter definitions
//This structure is visible for Leon and Shaves
 UInt8 CMX_DATA sippMemPool[SIPP_POOL_SZ] ALIGNED(32) = 
 {
     1,0,0,0
 };

//#################################################################################
static struct pool_s
{
    const char *name;  //only for debug...
    UInt8    *start; //start addr
    UInt8    *pos;   //current position
    UInt8    *end;   //end addr
}
sippPools[mempool_npools] =
{
  { "SIPP_", sippMemPool, 
             sippMemPool, 
             sippMemPool + sizeof(sippMemPool)},

 //Theoretical poll for line buffers it doesn't require actual buffer... but maps well on dyn alloc 
  { "SLICE", LN_POOL_START,
             LN_POOL_START,
             LN_POOL_START + SIPP_SLICE_SZ}, //TBD: this size is WRONG, need to compute this based on win_regs_D 

 //The schedule pool maps on all slices that are currently used by current 
 //pipeline instance. In terms of physical addresses, it overlaps with SLICE pool
 //But: they are mutual exclusive: schedule pool is just some temporary storage to 
 //get the schedule done (which is then stored in sippMemPool), whereas mempool_lnbuff
 //is used at runtime
  { "SCHED", 0,
             0,
             0} 
};

//#################################################################################
//Always returns an 8-byte aligned memory chunk within required pool
// (no point in checking poll num here, as on SIPP allocates mem dynamically,
//  and this code is OK by design)
//#################################################################################
void *sippMemAlloc(SippMemPoolType pool, int n_bytes)
{
    UInt8 *pos = sippPools[(int)pool].pos;
    UInt8 *end = sippPools[(int)pool].end;
    UInt8 *new_pos;

  //Make sure we return the next 8 byte aligned address within current pool
    pos = (UInt8*)( (7 + (UInt32)pos) & ~7);

  //Advance new ptr. position for next alloc (i.e. skip over what's allocated now)
    new_pos = pos + n_bytes;
    new_pos = (UInt8*)( (7 + (UInt32)new_pos) & ~7);

    if(new_pos >= end)
       sippError(E_OUT_OF_MEM);
    
   //Save current ptr for next allocation
    sippPools[(UInt32)pool].pos = new_pos;
    
    VCS_PRINT_INT(pos);
    return pos;
}

//#################################################################################
#ifdef SIPP_PC
void * sipp_mem_resolve_windowed_addr(void *addr, int sliceno)
{
    UInt32    address = (UInt32)addr;

    if ((address & 0xFF000000) != ((UInt32)LN_POOL_START& 0xFF000000)) {
        return addr;
    }

    address &= 0x00FFFFFF; //keep lower 24 addr bits

    return cmx + SIPP_SLICE_SZ * sliceno + address;
}
#endif

//#################################################################################
// Initialize all pools current POS to start, and memset all pools
// This is vital, as I rely on the fact that params are ZERO by default after alloc
//#################################################################################
void sippMemReset(UInt32 sliceFirst, UInt32 sliceLast, UInt32 sliceSize)
{
   int i;

   //Compute the start, size of "virtual" pools (i.e. mempool_lnbuff & mempool_sched)
   #if   defined(__sparc) && !defined(MYRIAD2)
     sippPools[mempool_sched].start = (UInt8*)LHB_CMX_BASE_ADR;
   #elif defined(__sparc) && defined(MYRIAD2)
     sippPools[mempool_sched].start = (UInt8*)CMX_BASE_ADR;
   #elif defined(SIPP_PC)
     sippPools[mempool_sched].start = (UInt8*)&cmx[0];
   #endif

   sippPools[mempool_sched].start += sliceFirst * sliceSize;                //Point to first slice
   sippPools[mempool_sched].pos    = sippPools[mempool_sched].start;        //Cur_pos = start
   sippPools[mempool_sched].end    = sippPools[mempool_sched].start +       //Finish = start + N * SLC_SZ
                                      (sliceLast - sliceFirst + 1) * sliceSize;
      
   //#error same for LN_BUFF pool ???

   //quick clearing of sipp-mempool:
   sippMemPool[0] = 0;

   for (i = 0; i < mempool_lnbuff; i++) 
   {
    //Reset start pointer
      sippPools[i].pos = sippPools[i].start;

    #if !defined(SIPP_VCS)
    //Clear the pool
      sipp_memset(sippPools[i].start, 
                  0x000000000, 
                  (UInt32)(sippPools[i].end - sippPools[i].start));
    #endif
   }
}

//#################################################################################
//if a memory alloc failed due to insufficient storage, sippError would have 
//beend called by now, this is just debug 
void sippMemStatus()
{
   UInt32 i;

#ifdef SIPP_PC
   printf("\n\n");
   printf("Mem-pools status:\n");
   
   for(i=0; i < mempool_npools; i++) 
   {
       printf("   Mem-POOL: __%s__ : size = %8d, occupied = %8d (%05.2f%%)\n", 
           sippPools[i].name,
           sippPools[i].end - sippPools[i].start, //Size [bytes]
           sippPools[i].pos - sippPools[i].start,
           100.0f*(sippPools[i].pos - sippPools[i].start)/(sippPools[i].end - sippPools[i].start) //Size [%]
           );
   }
   printf("\n\n");
   
#elif defined(MYRIAD2) && 0
   for(i=0; i < mempool_npools; i++) 
   {
       printMsgInt("Mempool occupied : ", sippPools[i].pos - sippPools[i].start);
   }
#endif
} 
