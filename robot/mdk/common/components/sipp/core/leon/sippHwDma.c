#include <sipp.h>


#if defined(__sparc) && !defined(MYRIAD2) //thus Myriad1
void SabreDmaCopy(struct SippFilterS *fptr, int runNo)
{
    DmaParam  *cfg = (DmaParam *)fptr->params;
    UInt8     *src_addr, *dst_addr;
    UInt32     i, p, pixels_left, slice_pixels;


    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    //One time setup for computing constants
    if(runNo == 0)
    {
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        if(fptr->nParents == 0)//DDR to CMX
        {
          cfg->srcChkS = fptr->bpp * fptr->sliceWidth;//no stride between chunks (i.e. increment == chunk width)
          cfg->dstChkS = SIPP_SLICE_SZ;               //going to next SHAVE
          cfg->srcPlS = fptr->bpp * (fptr->outputW * fptr->outputH);//src in DDR, stride = full image plane
          cfg->dstPlS = fptr->bpp * fptr->planeStride;
        }
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        else if(fptr->nCons == 0) //CMX to DDR
        {
          cfg->srcChkS = SIPP_SLICE_SZ;
          cfg->dstChkS = fptr->bpp * fptr->sliceWidth;
          cfg->srcPlS = fptr->bpp * fptr->parents[0]->planeStride;
          cfg->dstPlS = fptr->bpp * (fptr->outputW * fptr->outputH);
        }
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if(fptr->nParents == 0)//DDR to CMX
    {
        src_addr      = (UInt8*)cfg->ddrAddr + (runNo * fptr->outputW * fptr->bpp);
        dst_addr      = (UInt8 *)WRESOLVE(fptr->dbLineOut[runNo&1], fptr->gi->sliceFirst);
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    else if(fptr->nCons == 0) //CMX to DDR
    {
        src_addr      = (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], fptr->gi->sliceFirst);
        dst_addr      = (UInt8*)cfg->ddrAddr + (runNo * fptr->outputW * fptr->bpp); //proper line
    }else
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    {
      #if defined(SIPP_PC)
        printf("__WEIRD case: DMA has both parents and consumers ????__\n");
        exit(2);
      #endif
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //The actual copy:
    pixels_left = fptr->outputW;
    for (i = fptr->gi->sliceFirst; i <= fptr->gi->sliceLast; i++)
    {
        slice_pixels = fptr->sliceWidth;

        //if we got less bytes in the last run:
        if (fptr->sliceWidth > pixels_left) {
            slice_pixels = pixels_left;
        }

        for(p=0; p<fptr->nPlanes; p++)
        {
           swcDmaMemcpy(i, 
                        dst_addr + p * cfg->dstPlS,
                        src_addr + p * cfg->srcPlS,
                        slice_pixels * fptr->bpp,
                        1);
        }

         dst_addr += cfg->dstChkS;
         src_addr += cfg->srcChkS;

         pixels_left -= slice_pixels;
    }
}
#endif

//#################################################################################
void sippInitDma(SippFilter *fptr)
{
 #if defined(MYRIAD2) || defined(SIPP_PC)
    SippPipeline *pl = fptr->gi->pl;
    UInt64 id;
  //Mem for params is already allocated at sippCreateFilter time, so can populate now
    DmaParam *param = (DmaParam*)fptr->params;

  //Scan in the DMA filters and see where we find current filter
    for(id=0; id<pl->nFiltersDMA; id++)
    {
        if(fptr == pl->filtersDMA[id])
            break;
    }

    if(id==0)
    {//Enable the CMX-DMA when we init first DMA filter
     //this is a bit risky, move the inits somewhere else !
       SET_REG_DWORD(CDMA_CTRL_ADR,   0x1LL<<3); //SW reset

       NOP;NOP;NOP;NOP;NOP; NOP;NOP;NOP;NOP;NOP;
       NOP;NOP;NOP;NOP;NOP; NOP;NOP;NOP;NOP;NOP;
       NOP;NOP;NOP;NOP;NOP; NOP;NOP;NOP;NOP;NOP;

       SET_REG_DWORD(CDMA_INT_EN_ADR,     0xFFFFLL);
       SET_REG_DWORD(CDMA_CTRL_ADR,       0x1LL<<2); //Enable DMA
       SET_REG_DWORD(CDMA_SLICE_SIZE_ADR, pl->sliceSz);
    }

   param->dscCtrlLinkAddr =  (0x00000000LL  << 0)  //Link Addr (terminator for now)
                             |(       0x3LL <<32)  //Transaction type: 2D with chunking
                             |(       0x0LL <<34)  //Priority
                             |(       0x4LL <<36)  //Burst Len
                             |(          id <<40)  //TransactionID
                             |(          id <<44)  //Interrupt Sel : check bit 0 in INT_STATUS
                             |(       0x0LL <<48)  //Partition
                             |(          id <<59); //skip 

   //No point in programming SRC/DST addresses as we'll do this before every start, for every
   // descriptor that is to be parsed !
   param->dscPlanesLen    = ((UInt64)(fptr->nPlanes-1)<<32) | ((UInt64)(fptr->outputW * fptr->bpp));

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   if(fptr->nParents == 0)//DDR to CMX
   {
    if(param->dstChkW == 0){
       param->dstChkW  = fptr->bpp * fptr->sliceWidth;               //dst in CMX, thus chuked
       param->dstChkS  = pl->sliceSz;                                //chunk stride = slice size
       param->dstPlS   = fptr->bpp * fptr->planeStride;              //????
       param->dstLnS   = param->dstChkW; //will start in same slice 
    }

    if(param->srcChkW == 0){
       param->srcChkW  = fptr->bpp * fptr->outputW;                  //src in DDR: thus full line width
       param->srcChkS  = param->srcChkW;                             //full line, no chunks should exist, thus 0
       param->srcPlS   = fptr->bpp * (fptr->outputW * fptr->outputH);//src in DDR, pl stride = full image plane stride
       param->srcLnS   = param->srcChkW;
    }
   }
   //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   else if(fptr->nCons == 0) //CMX to DDR
   {
   //Note: fptr->planeStride = fptr->lineStride * fptr->nLines;
   //      given a CONSUMER-DMA filter doesn't get an output buffer, thus fptr->nLines = 0
   //      thus planeStride = 0. So use (fptr->parents[0])->planeStride
    if(param->dstChkW == 0){
       param->dstChkW  = fptr->outputW * fptr->bpp;                  //dst in DDR, thus line width = full line width
       param->dstChkS  = param->dstChkW;                             //full line, no chunks should exist, thus 0
       param->dstPlS   = fptr->bpp * (fptr->outputW * fptr->outputH);//src in DDR, pl stride = full image plane stride
       param->dstLnS   = param->dstChkW;
    }

    if(param->srcChkW == 0){
       param->srcChkW  = fptr->bpp * fptr->sliceWidth; //src in CMX, thus chunked
       param->srcChkS  = pl->sliceSz;
       param->srcPlS   = fptr->bpp *  fptr->parents[0]->planeStride;
       param->srcLnS   = param->srcChkW;
    }
   }
 
  param->dscSrcStrdWidth =  (((UInt64)param->srcChkS)<<32) //chunk/line stride
                           |(((UInt64)param->srcChkW)    );//chunk/line width
 
  param->dscDstStrdWidth =  (((UInt64)param->dstChkS)<<32) //chunk/line stride
                           |(((UInt64)param->dstChkW)    );//chunk/line width

  param->dscPlStrides    =  (((UInt64)param->dstPlS)<<32) 
                           |(((UInt64)param->srcPlS)    );
 #endif
}

//#################################################################################
void sippUpdateDmaAddr(SippFilter *fptr)
{
  #if defined(MYRIAD2) || defined(SIPP_PC)
    UInt32 runNo = fptr->exeNo;
    UInt32 srcAddr, dstAddr;
    DmaParam *cfg = (DmaParam*)fptr->params;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    if(fptr->nParents == 0)//DDR to CMX
    {
        srcAddr      = (UInt32)cfg->ddrAddr + (runNo * cfg->srcLnS);
        dstAddr      = (UInt32)WRESOLVE(fptr->dbLineOut[runNo&1], fptr->firstOutSlc);
    }
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    else if(fptr->nCons == 0) //CMX to DDR
    {
        srcAddr      = (UInt32)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], fptr->parents[0]->firstOutSlc);
        dstAddr      = (UInt32)cfg->ddrAddr + (runNo * cfg->dstLnS); //proper line
    }
 
  //Update the descriptor SRC and DST addresses:
    cfg->dscDstSrcAddr = ((UInt64)dstAddr<<32) | (srcAddr);
  #endif
}

//#################################################################################
void sippKickDma(SippPipeline *pl)
{
    SippFilter **filters = pl->filtersDMA;
    UInt32      nFilters = pl->nFiltersDMA;
    UInt32      runMask  = pl->schedInfo[pl->iteration].dmaMask & DMA_MASK;
    UInt32      i;

  //1) Update SRC and DST addresses for all DMA filters that are to run in this iter
    for(i=0; i<nFilters; i++)
    {
        if(runMask & (1<<i))
        { 
          #if defined(MYRIAD2) || defined(SIPP_PC)
          sippUpdateDmaAddr(filters[i]);
          #else
          SabreDmaCopy(filters[i], filters[i]->exeNo);
          #endif
        }
    }

  #if defined(MYRIAD2) || defined(SIPP_PC)
  //2)Update the skip descriptor
     SET_REG_DWORD(CDMA_SKIP_DES_ADR, ~runMask);
  //3) The actual kick
     SET_REG_DWORD(CDMA_LINK_CFG0_ADR,   (0x1LL                       <<41) //Wrap enable 
                                       | (((UInt64)pl->gi.sliceLast ) <<37) //maxSlice
                                       | (((UInt64)pl->gi.sliceFirst) <<33) //minSlice
                                       |  (0x1LL                      <<32) //Start
                                       | (UInt64)pl->filtersDMA[0]->params);//Addr of first descriptor
  #endif
}

//#################################################################################
void sippWaitDma(UInt32 waitMask)
{
    #if defined(MYRIAD2)
      UInt64 status;
      while(1)
      {
        status = swcLeonReadNoCacheU64(CDMA_INT_STATUS_ADR | 0x08000000);
        if((status & waitMask) != waitMask)
        {
            NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP;
            NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP; NOP;
            //after a few nops, continue the polling
        }
        else{ 
            break;
        }
      }
     //Clear status bit for next iteration...
      SET_REG_DWORD(CDMA_INT_RESET_ADR, (UInt64)waitMask);
    #else
     //PC and Myriad1 DMA copies are blocking, so nothing to be done
    #endif
}

//#################################################################################
//MUST be done once entire pipeline is defined !
//There must be at least 2 DMAs in the system  !
void sippChainDmaDesc(SippPipeline *pl)
{   
  #if defined(MYRIAD2) || defined(SIPP_PC)
    UInt32    i;
    
    for(i=0; i<pl->nFiltersDMA-1; i++)
    {
        DmaParam *paramCur;
        DmaParam *paramNxt;

        paramCur = (DmaParam*)pl->filtersDMA[i  ]->params;
        paramNxt = (DmaParam*)pl->filtersDMA[i+1]->params;

        paramCur->dscCtrlLinkAddr = paramCur->dscCtrlLinkAddr | ((UInt64)paramNxt);
    }
   //Last DMA: already assumes nothing follows after, so nothing else tbd...
  #endif
}
