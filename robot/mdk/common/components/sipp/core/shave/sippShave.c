#include <sipp.h>


//These get allocated PER SHAVE
SippPipeline *sipp_pl;     //The global Pipeline structure shared by Shaves and Leon.

#define FIRST_SHAVE        (sipp_pl->gi.sliceFirst)
#define LAST_SHAVE         (sipp_pl->gi.sliceLast)
#define MANAGER_SHAVE       FIRST_SHAVE
#define NUM_SHAVES_MINUS_1 (LAST_SHAVE - FIRST_SHAVE)


//########################################################################################
#ifdef __MOVICOMPILE__
    extern int  scGetShaveNumber(); //asm version
#else //PC:
    UInt32 dbg_svu_no;
    int scGetShaveNumber()
    {
        return dbg_svu_no; //set by "sippKickShave" on PC 
    }
#endif

//########################################################################################
//For a given filter "fptr", return the absolute line address(lineNo) from parent filter 
//indicated by index "parent"
UInt32 getInPtr(SippFilter *fptr, UInt32 parNo, UInt32 lineNo, UInt32 planeNo)
{
    const CommInfo *ci = fptr->gi;
    UInt32 curSvu = scGetShaveNumber();

  //Initial proposal, as HW WinRegs would:
    UInt32 runNo = fptr->exeNo&1;
    Int32  addr  = fptr->dbLinesIn[parNo][runNo][lineNo];

  //WANRING: should move the SLICE START globally to remove duplication
    if ((addr & 0xFF000000) != 0x1F000000) {
        return addr;
    }
    
    addr &= 0x00FFFFFF; //keep lower 24 addr bits
    addr += ci->cmxBase + ci->sliceSize * (fptr->parents[parNo]->firstOutSlc + curSvu - ci->sliceFirst);
    addr += planeNo * fptr->parents[parNo]->planeStride;
    if(addr > ci->wrapAddr)
        addr += ci->wrapCT; //INT OPERATION, as wrapCT is likely a NEGATIVE number

    return (UInt32)addr;
}

//########################################################################################
//same functionality as getInPtr, just that works on output buffer, and ADDR
// should consider firstOutSlc of current buffer!
UInt32 getOutPtr(SippFilter *fptr, UInt32 planeNo)
{
    const CommInfo *ci = fptr->gi;
    UInt32 curSvu = scGetShaveNumber();

  //Initial proposal, as HW WinRegs would:
    UInt32 runNo = fptr->exeNo&1;
    Int32  addr  = (Int32)fptr->dbLineOut[runNo];

  //WANRING: should move the SLICE START globally to remove duplication
    if ((addr & 0xFF000000) != 0x1F000000) {
        return addr;
    }
    
    addr &= 0x00FFFFFF; //keep lower 24 addr bits
    addr += ci->cmxBase + ci->sliceSize * (fptr->firstOutSlc + curSvu - ci->sliceFirst);
    addr += planeNo * fptr->planeStride;
    if(addr > ci->wrapAddr)
        addr += ci->wrapCT;

    return (UInt32)addr;
}

/*
UInt8 *inR = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);

void * sipp_mem_resolve_windowed_addr(void *addr, int sliceno)
{
    UInt32    address = (UInt32)addr;

    if ((address & 0xFF000000) != ((UInt32)LN_POOL_START& 0xFF000000)) {
        return addr;
    }

    address &= 0x00FFFFFF; //keep lower 24 addr bits

    return cmx + SIPP_SLICE_SZ * sliceno + address;
}
*/

//########################################################################################
void memsetBpp(UInt8 *i_dest_left, 
               UInt8 *i_source_left, 
               UInt32 i_padding, 
               UInt32 i_bpp)
{
    UInt8   Format_1b;
    UInt16  Format_2b;
    UInt32  Format_4b;
    UInt32  i;

    switch(i_bpp)
    {   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        case 1:
           Format_1b =  *((UInt8*)i_source_left);
           for (i = 0; i < i_padding; i++)
           {
              *(UInt8*)i_dest_left = Format_1b;
              i_dest_left +=i_bpp;
           }
           break;
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        case 2:
           Format_2b = *((UInt16*)i_source_left);
           for (i = 0; i < i_padding; i++) 
           {
             *((UInt16*)i_dest_left) = Format_2b;
             i_dest_left +=i_bpp;
           }
           break;
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        case 4:
            Format_4b = *((UInt32*)i_source_left);
            for (i = 0; i < i_padding; i++) {
              *(UInt32*)i_dest_left = Format_4b;
               i_dest_left +=i_bpp;
            }
            break;
    }//switch
}

#ifdef __MOVICOMPILE__
//########################################################################################
//Resolve Windowed Address (A subset for WRESOLVE macro) required for line padding
// we must use absolute addresses, as 1F0000 - something will turn to 0x1E....
// thus point to invalid locations...
#define MXI_CMX_BASE_ADR 0x10000000 //WARNING: 0x70000000 for Myriad2 !
UInt32 RWA(UInt32 inAddr, int targetS, UInt32 *windowRegs)
{ 
    if(inAddr == 0) return 0;
  //subset of Leon's swcSolveShaveRelAddrAHB
  // "case 0x1F: resolved = (((inAddr & 0x00FFFFFF)+(windowRegs[targetS])[3]) & 0x0FFFFFFF) | 0xA0000000; break;"
    return (((inAddr & 0x00FFFFFF)+windowRegs[targetS*4+3]) & 0x0FFFFFFF) | MXI_CMX_BASE_ADR;
}
#endif

//########################################################################################
//Padding usually does padding on previous generated line !
// - the last line of a buffer gets padded when the bottom replication starts
// - this is when last line of that buffer will get used due to the DELAY required by HW
//########################################################################################
void sippDoHorizReplication(SippFilter *fptr, int svuNo)
{
    int32_t  i_plane_stride = fptr->planeStride * fptr->bpp;
    int32_t  i_nplanes      = fptr->nPlanes;
    
    int32_t  i_padding      = fptr->hPadding; 
    int32_t  i_bpp          = fptr->bpp;
    int32_t  i_width;
    
    UInt8 *i_dest_left;
    UInt8 *i_dest_right;  
    UInt8 *i_source_left; 
    UInt8 *i_source_right;
    UInt8 *i_line_ptr;
    
  #ifdef __MOVICOMPILE__
    i_line_ptr = (UInt8 *)RWA(fptr->lnToPad[fptr->exeNo & 1], svuNo, fptr->gi->pl->svuWinRegs); 
  #else //Windows
    i_line_ptr = (UInt8 *)WRESOLVE(fptr->lnToPad[fptr->exeNo & 1], svuNo);
  #endif

    if(i_line_ptr == NULL) //when first line is generated, prev_out is not valid, so early exit here
        return;

    //TBD: need to capture in fptr, the "runNo" it refers to !
    //     and the prev_output_line for the run, so that we pad the right line !

    if (svuNo == LAST_SHAVE)
          i_width = fptr->outputW - (fptr->sliceWidth * NUM_SHAVES_MINUS_1);
    else
          i_width = fptr->sliceWidth;  //task->u.hrep.width;
    
    i_dest_left    = i_line_ptr - (i_padding * i_bpp);
    i_dest_right   = i_line_ptr + (i_width   * i_bpp);
    i_source_left  = i_line_ptr - SIPP_SLICE_SZ + (fptr->sliceWidth - i_padding )*i_bpp;
    i_source_right = i_line_ptr + SIPP_SLICE_SZ;

    //
    if (svuNo == FIRST_SHAVE)  i_source_left  = i_line_ptr;
    if (svuNo == LAST_SHAVE)   i_source_right = i_line_ptr;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //loop through all planes:
    do{
        //Default behavior
          memcpy(i_dest_left,  i_source_left,  (i_padding * i_bpp));
          memcpy(i_dest_right, i_source_right, (i_padding * i_bpp));

        //Special case for [first shave]
          if(svuNo == FIRST_SHAVE)
             memsetBpp(i_dest_left, i_source_left, i_padding, i_bpp);
            

        //Special case for [last shave]
           if(svuNo == LAST_SHAVE)
              memsetBpp(i_dest_right,i_dest_right-i_bpp,i_padding, i_bpp);

        //Change plane if are more that one 
          i_dest_left     += i_plane_stride;
          i_dest_right    += i_plane_stride;
          i_source_left   += i_plane_stride;
          i_source_right  += i_plane_stride;

    }while(--i_nplanes);
}

//#############################################################################
#ifdef __MOVICOMPILE__
   extern int  scFifoRead();  //asm version
   extern void scFifoWrite(int svuNo, int word0, int word1); //asm version
   #define GO        (0x1923)

 // The sync uses the barrier method. One Shave is chosen by SIPP 
 // to be the manager. When it completes, it waits for all others 
 // to signal completion. Then it starts them one by one writing
 // to each Shave's FIFO.
__attribute__((always_inline)) void sync_barrier(int svuNo)
{
    int fifo_rd;
    int num_shaves = sipp_pl->gi.sliceLast - sipp_pl->gi.sliceFirst + 1;

   //Early exit case: if there's just one shave running, then
   //just return as we don't need to sync with anybody else...
   //(this would be for test modes mostly)
    if(num_shaves == 1)
        return;

    if(svuNo == MANAGER_SHAVE)
    {
        //Manager Shave already completed so we mark it 
        int shaves = 1;//num of shaves that are done!
        int i;

        //Manager Shave waits for all others to complete
        do {
            fifo_rd = scFifoRead();
            shaves++;
        } while (shaves != num_shaves);

        //At this moment, all shaves reached the barrier.
        //Manager Shave (i.e. first one) starts remaining shaves 
        for (i = FIRST_SHAVE + 1; i <= LAST_SHAVE; i++)
             scFifoWrite(i, GO, 0);
    }
    else {
        //Signal READY to master
        scFifoWrite(MANAGER_SHAVE, svuNo, 0);

        //Wait for GO signal from master
        do {
            fifo_rd = scFifoRead();
        } while (fifo_rd != GO);
    }
}
#else
void sync_barrier(int svuNo)
{
    //empty
}
#endif


//#############################################################################
//#############################################################################
//#############################################################################
int SHAVE_MAIN(void)
{
    UInt32      cur_svu = scGetShaveNumber();
    SippFilter *fptr;
    UInt32      i, p;

    while(1)
    {
        UInt32      num_filtes = sipp_pl->nFiltersSvu;
        SippFilter  **filters  = sipp_pl->filtersSvu;
        UInt32      runMask    = sipp_pl->schedInfo[sipp_pl->iteration].shaveMask;

        ////~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        //if(sipp_pl->svuCmd & CMD_H_PAD)
        //{
        //  for(i=0; i<num_filtes; i++)
        //  {
        //    //if(filters[i]->flags & SIPP_FLAG_DO_H_PADDING) //maste only can clear flag...
        //      if(filters[i]->hPadding)
        //      {
        //          sippDoHorizReplication(filters[i], cur_svu); //cur_svu can go away
        //         // fptr->flags &= ~SIPP_FLAG_DO_H_PADDING; //master only cand update this
        //      }
        //  }
        //}//H-padding
        //sync_barrier(cur_svu);

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if(sipp_pl->svuCmd & CMD_RUN)
        {
          for(i=0; i<num_filtes; i++)
          {
            if(runMask & (1<<i))
            {
                fptr = filters[i];

               //First, pad input buffers (i.e. parent buffers)
                for(p=0; p<fptr->nParents; p++)
                    sippDoHorizReplication(fptr->parents[p], cur_svu);

               //Then Run
                fptr->funcSvuRun(fptr, cur_svu, fptr->exeNo);
            }
          }
        }
        sync_barrier(cur_svu);

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if(sipp_pl->svuCmd & CMD_EXIT)
        {
           #if defined(SIPP_PC)
            return 1;
           #else //Sabre
            __asm("BRU.swih 0x1E \n\t nop 5");
           #endif
        }
    }

    return 0;
}
