#include <sipp.h>

extern int               SVU_SYM(SHAVE_MAIN)(void);

//####################################################################################
SippPipeline* sippCreatePipeline(UInt32 sliceFirst,
                                 UInt32 sliceLast,
                                 UInt32 *svuWinRegs,
                                 UInt8  *mbinImg)
{
    SippPipeline    *pl;
    UInt32           i, u;

    //Reset mem pool before we start creating the pipeline !
    //TBD: compute LN_BUF pool limits based on win_reg_D
    sippMemReset(sliceFirst, sliceLast, SIPP_SLICE_SZ);

    //g_sipp_err        = E_SUCCESS;

    pl                = (SippPipeline *)sippMemAlloc(mempool_sipp, sizeof (*pl));
    pl->svuCmd        = 0; //ShaveS: stay IDLE
    pl->nFilters      = 0; //total number of filters
    pl->nFiltersSvu   = 0; //total number of Shave filters
    pl->nFiltersDMA   = 0; //total number of Dma filters

    for(u=0; u<EXE_NUM; u++)
    {
        pl->hwSippFirst [u] = -1; //index
        pl->hwSippFltCnt[u] =  0; //ZERO
        pl->hwSippCurFlt[u] =  0; //NULL
        pl->hwSippCtxOff[u] = -1; //offset will be > 0 for Hw-SIPP filters that run multi-ctx
        pl->hwSippCurCmd[u] =  0;
    }

    pl->schedInfo     = NULL;

    pl->nSwFilters    = 0;
    pl->sliceSz       = 128*1024;
    pl->gi.sliceSize  = pl->sliceSz; //STUPID, this is duplicated, TBD remove a copy !
    pl->gi.curFrame   = 0;
    pl->gi.sliceFirst = sliceFirst;
    pl->gi.sliceLast  = sliceLast;
    pl->gi.pl         = pl;

   #if defined(__sparc)
    pl->gi.cmxBase    = MXI_CMX_BASE_ADR;
   #else
    extern UInt8 cmx[];
    pl->gi.cmxBase    = (UInt32)cmx;
   #endif

  //WARNING: sliceLast is included in pipeline, so add a 1 there...
    pl->gi.wrapCT     = ((int)sliceFirst-(int)(sliceLast+1)) * pl->gi.sliceSize;
    pl->gi.wrapAddr   = pl->gi.cmxBase  + (sliceLast+1)      * pl->gi.sliceSize;

    pl->svuWinRegs    = svuWinRegs;
    pl->mbinImg       = mbinImg;

  //Init Shave sync variables
  #if defined(MYRIAD2) || defined(SIPP_PC)
    for(i=0; i<12; i++)
    {
      pl->svuStat     [i]=0;//IDLE
      pl->svuStartMask[i]=0;//IDLE
    }
    for(i=sliceFirst;i<=sliceLast; i++)
    {
      pl->svuStartMask[i]=1;
    }
  #endif

#ifdef SIPP_PC
    extern void initHwModels();
    initHwModels();
    pl->dbgLevel      = 1;
#endif

#if defined(MYRIAD2) || defined(SIPP_PC)
  //Init internal lookup tables:
    pl->hwFnLnIni[SIPP_CONV_ID]        = NULL;
    pl->hwFnLoad [SIPP_CONV_ID]        = sippLoadConv;
    pl->hwCtxSwc [SIPP_CONV_ID]        = ctxSwitchOnePar;
    pl->ictxAddr [SIPP_CONV_ID][0]     = I_CTX(SIPP_CONV_ID);
    pl->octxAddr [SIPP_CONV_ID]        = O_CTX(SIPP_CONV_ID);
    
    pl->hwFnLnIni[SIPP_MED_ID]         = NULL;
    pl->hwFnLoad [SIPP_MED_ID]         = sippLoadMed;
    pl->hwCtxSwc [SIPP_MED_ID]         = ctxSwitchOnePar;
    pl->ictxAddr [SIPP_MED_ID][0]      = I_CTX(SIPP_MED_ID);
    pl->octxAddr [SIPP_MED_ID]         = O_CTX(SIPP_MED_ID);

    pl->hwFnLnIni[SIPP_DBYR_PPM_ID]    = NULL;
    pl->hwFnLoad [SIPP_DBYR_PPM_ID]    = sippLoadDbyrPP;
    pl->hwCtxSwc [SIPP_DBYR_PPM_ID]    = ctxSwitchOnePar;
    pl->ictxAddr [SIPP_DBYR_PPM_ID][0] = I_CTX(SIPP_DBYR_PPM_ID);
    pl->octxAddr [SIPP_DBYR_PPM_ID]    = O_CTX(SIPP_DBYR_PPM_ID);

    pl->hwFnLnIni[SIPP_DBYR_ID]        = NULL;
    pl->hwFnLoad [SIPP_DBYR_ID]        = sippLoadDbyr;
    pl->hwCtxSwc [SIPP_DBYR_ID]        = ctxSwitchOnePar;
    pl->ictxAddr [SIPP_DBYR_ID][0]     = I_CTX(SIPP_DBYR_ID);
    pl->octxAddr [SIPP_DBYR_ID]        = O_CTX(SIPP_DBYR_ID);

    pl->hwFnLnIni[SIPP_SHARPEN_ID]     = NULL;
    pl->hwFnLoad [SIPP_SHARPEN_ID ]    = sippLoadSharpen;
    pl->hwCtxSwc [SIPP_SHARPEN_ID]     = ctxSwitchOnePar;
    pl->ictxAddr [SIPP_SHARPEN_ID][0]  = I_CTX(SIPP_SHARPEN_ID);
    pl->octxAddr [SIPP_SHARPEN_ID]     = O_CTX(SIPP_SHARPEN_ID);

    pl->hwFnLnIni[SIPP_EDGE_OP_ID]     = NULL;
    pl->hwFnLoad [SIPP_EDGE_OP_ID ]    = sippLoadEdgeOp;
    pl->hwCtxSwc [SIPP_EDGE_OP_ID]     = ctxSwitchOnePar;
    pl->ictxAddr [SIPP_EDGE_OP_ID][0]  = I_CTX(SIPP_EDGE_OP_ID);
    pl->octxAddr [SIPP_EDGE_OP_ID]     = O_CTX(SIPP_EDGE_OP_ID);

    pl->hwFnLnIni[SIPP_LUMA_ID]        = NULL;
    pl->hwFnLoad [SIPP_LUMA_ID ]       = sippLoadLumaDns;
    pl->hwCtxSwc [SIPP_LUMA_ID]        = ctxSwitchTwoPar; //Image and Reference
    pl->ictxAddr [SIPP_LUMA_ID][0]     = I_CTX(SIPP_LUMA_ID);
    pl->ictxAddr [SIPP_LUMA_ID][1]     = I_CTX(SIPP_LUMA_REF_ID);
    pl->octxAddr [SIPP_LUMA_ID]        = O_CTX(SIPP_LUMA_ID);

    pl->hwFnLnIni[SIPP_CHROMA_ID]      = NULL;
    pl->hwFnLoad [SIPP_CHROMA_ID]      = sippLoadChrDns;
    pl->hwCtxSwc [SIPP_CHROMA_ID]      = ctxSwitchChromaDns;
    pl->ictxAddr [SIPP_CHROMA_ID][0]   = I_CTX(SIPP_CHROMA_ID);
    pl->ictxAddr [SIPP_CHROMA_ID][1]   = I_CTX(SIPP_CHROMA_REF_ID);
    pl->octxAddr [SIPP_CHROMA_ID]      = O_CTX(SIPP_CHROMA_ID);

    pl->hwFnLnIni[SIPP_LSC_ID]         = NULL;
    pl->hwFnLoad [SIPP_LSC_ID]         = sippLoadLsc;
    pl->hwCtxSwc [SIPP_LSC_ID]         = ctxSwitchTwoPar; //Image and GainMap
    pl->ictxAddr [SIPP_LSC_ID][0]      = I_CTX(SIPP_LSC_ID);
    pl->ictxAddr [SIPP_LSC_ID][1]      = I_CTX(SIPP_LSC_GM_ID);
    pl->octxAddr [SIPP_LSC_ID]         = O_CTX(SIPP_LSC_ID);

    pl->hwFnLnIni[SIPP_HARRIS_ID]      = NULL;
    pl->hwFnLoad [SIPP_HARRIS_ID]      = sippLoadHarris;
    pl->hwCtxSwc [SIPP_HARRIS_ID]      = ctxSwitchOnePar;
    pl->ictxAddr [SIPP_HARRIS_ID][0]   = I_CTX(SIPP_HARRIS_ID);
    pl->octxAddr [SIPP_HARRIS_ID]      = O_CTX(SIPP_HARRIS_ID);

    pl->hwFnLnIni[SIPP_RAW_ID]         = sippSetupRawLine; //!!!!
    pl->hwFnLoad [SIPP_RAW_ID]         = sippLoadRaw;
    pl->hwCtxSwc [SIPP_RAW_ID]         = ctxSwitchOnePar;
    pl->ictxAddr [SIPP_RAW_ID][0]      = I_CTX(SIPP_RAW_ID);
    pl->octxAddr [SIPP_RAW_ID]         = O_CTX(SIPP_RAW_ID);

    pl->hwFnLnIni[SIPP_LUT_ID]         = NULL;
    pl->hwFnLoad [SIPP_LUT_ID]         = sippLoadLut;
    pl->hwCtxSwc [SIPP_LUT_ID]         = ctxSwitchLut;
    pl->ictxAddr [SIPP_LUT_ID][0]      = I_CTX(SIPP_LUT_ID);
    pl->octxAddr [SIPP_LUT_ID]         = O_CTX(SIPP_LUT_ID);

    pl->hwFnLnIni[SIPP_UPFIRDN_ID]     = sippSetupPolyLine;
    pl->hwFnLoad [SIPP_UPFIRDN_ID]     = sippLoadPolyFir;
    pl->hwCtxSwc [SIPP_UPFIRDN_ID]     = ctxSwitchPoly;
    pl->ictxAddr [SIPP_UPFIRDN_ID][0]  = I_CTX(SIPP_UPFIRDN_ID);
    pl->octxAddr [SIPP_UPFIRDN_ID]     = O_CTX(SIPP_UPFIRDN_ID);

    pl->hwFnLnIni[SIPP_CC_ID]        = NULL;
    pl->hwFnLoad [SIPP_CC_ID ]       = sippLoadColComb;
    pl->hwCtxSwc [SIPP_CC_ID]        = ctxSwitchColComb;
    pl->ictxAddr [SIPP_CC_ID][0]     = I_CTX(SIPP_CC_ID);
    pl->ictxAddr [SIPP_CC_ID][1]     = I_CTX(SIPP_CC_CHROMA_ID);
    pl->octxAddr [SIPP_CC_ID]        = O_CTX(SIPP_CC_ID);

#endif

    //ctxSwitchLut

    //pl->hwFnLoad[SIPP_CC_ID ]     = sippLoadColComb;
    //pl->ictxAddr[SIPP_CC_ID][0]   = I_CTX(SIPP_CC_ID);
    //pl->ictxAddr[SIPP_CC_ID][1]   = I_CTX(SIPP_CC_CHROMA_ID);
    //pl->octxAddr[SIPP_CC_ID]      = O_CTX(SIPP_CC_ID);
    //TBD: the rest...
    
    return pl;
}

//####################################################################################
//Filter Constructor, returns a ptr. to currently initialized filter
//####################################################################################
SippFilter * sippCreateFilter (SippPipeline *pl, 
                               UInt32      flags,
                               UInt32      out_W, 
                               UInt32      out_H,
                               UInt32      num_pl,
                               UInt32      bpp,
                               UInt32      paramSz,
                               FnSvuRun    funcSvuRun,
                               const char  *name)
{
    SippFilter *fptr;

  //Create space for new filter
    pl->filters[pl->nFilters] = (SippFilter *)sippMemAlloc(mempool_sipp, sizeof(SippFilter));
    fptr                      = pl->filters[pl->nFilters];
    
  //Const params
    fptr->gi       = &pl->gi;
    fptr->id       = pl->nFilters; //attach a filter number (debug mostly)
    fptr->nParents = 0;
    fptr->nCons    = 0;
    fptr->nLines   = 0;
    fptr->sch      = NULL;
    fptr->outputW  = out_W;
    fptr->outputH  = out_H;
    fptr->nPlanes  = num_pl;
    fptr->bpp      = bpp;
    fptr->iBuf[0]  = 0;
    fptr->iBuf[1]  = 0;
    
    fptr->planeStride  = 0;
    fptr->firstOutSlc  = pl->gi.sliceFirst; //default first output chunk to FIRST_SLICE
    fptr->shiftPlanes  = 0;
    fptr->outputBuffer = NULL;

  //User should override only what's needed...
    fptr->funcSvuRun = NULL; //shave run

  //ASK func: regular, resize or CUSTOM
    if(flags & SIPP_RESIZE)
       fptr->funcAsk = askResizer;
    else
       fptr->funcAsk = askRegular;

  //Assign an HW exec unit (also force the size of config area for known HW peripherals)
    fptr->funcSvuRun      = 0; //by default (only shave tasks will get this properly assigned...)

    switch ((UInt32)funcSvuRun)
    {
        //==================================
        case SIPP_DMA_ID:     fptr->unit = SIPP_DMA_ID;         paramSz = SZ(DmaParam);    
                            //Add filter to specific filter list
                              pl->filtersDMA[pl->nFiltersDMA] = fptr;
                              pl->nFiltersDMA++;
                              break;
        
       #if defined(SIPP_PC) || defined(MYRIAD2)
        case SIPP_MED_ID:      fptr->unit = SIPP_MED_ID;      paramSz = SZ(MedParam);     pl->hwSippFltCnt[SIPP_MED_ID    ]++;  break;
        case SIPP_LSC_ID:      fptr->unit = SIPP_LSC_ID;      paramSz = SZ(LscParam);     pl->hwSippFltCnt[SIPP_LSC_ID    ]++;  break;
        case SIPP_RAW_ID:      fptr->unit = SIPP_RAW_ID;      paramSz = SZ(RawParam);     pl->hwSippFltCnt[SIPP_RAW_ID    ]++;  break;
        case SIPP_SHARPEN_ID:  fptr->unit = SIPP_SHARPEN_ID;  paramSz = SZ(UsmParam);     pl->hwSippFltCnt[SIPP_SHARPEN_ID]++;  break;
        case SIPP_LUMA_ID:     fptr->unit = SIPP_LUMA_ID;     paramSz = SZ(YDnsParam);    pl->hwSippFltCnt[SIPP_LUMA_ID   ]++;  break;
        case SIPP_CHROMA_ID:   fptr->unit = SIPP_CHROMA_ID;   paramSz = SZ(ChrDnsParam);  pl->hwSippFltCnt[SIPP_CHROMA_ID ]++;  break;
        case SIPP_LUT_ID:      fptr->unit = SIPP_LUT_ID;      paramSz = SZ(LutParam);     pl->hwSippFltCnt[SIPP_LUT_ID    ]++;  break;
        case SIPP_CONV_ID:     fptr->unit = SIPP_CONV_ID;     paramSz = SZ(ConvParam);    pl->hwSippFltCnt[SIPP_CONV_ID   ]++;  break;
        case SIPP_UPFIRDN_ID:  fptr->unit = SIPP_UPFIRDN_ID;  paramSz = SZ(PolyFirParam); pl->hwSippFltCnt[SIPP_UPFIRDN_ID]++;  break; 
        case SIPP_EDGE_OP_ID:  fptr->unit = SIPP_EDGE_OP_ID;  paramSz = SZ(EdgeParam);    pl->hwSippFltCnt[SIPP_EDGE_OP_ID]++;  break;
        case SIPP_DBYR_PPM_ID: fptr->unit = SIPP_DBYR_PPM_ID; paramSz = SZ(DbyrPpParam);  pl->hwSippFltCnt[SIPP_DBYR_PPM_ID]++; break;

        case SIPP_HARRIS_ID:  fptr->unit = SIPP_HARRIS_ID;
                              paramSz    = SZ(HarrisParam);  
                              pl->hwSippFltCnt[SIPP_HARRIS_ID]++;
                              fptr->funcAsk = askHwHarris;
                              break;

        case SIPP_CC_ID:      fptr->unit    = SIPP_CC_ID;
                              fptr->funcAsk = askHwColorComb;
                              paramSz       = SZ(ColCombParam);
                              pl->hwSippFltCnt[SIPP_CC_ID]++;
                              break;

        case SIPP_DBYR_ID:    fptr->unit    = SIPP_DBYR_ID;
                              fptr->funcAsk = askHwDebayer; 
                              paramSz       = SZ(DbyrParam);
                              pl->hwSippFltCnt[SIPP_DBYR_ID]++;
                              break;
       #endif

        //==================================
        //case SIPP_SVU_ID:
        default: //Shave code
            fptr->unit       = SIPP_SVU_ID;
            fptr->funcSvuRun = funcSvuRun;
            pl->nSwFilters++;
            //priv_alloc stays as defined by USER

           //Add to Shave filter list
            pl->filtersSvu[pl->nFiltersSvu] = fptr;
            pl->nFiltersSvu++;
            break;
    }

   //Internal memory buffers
    if (paramSz == 0) {
      fptr->params = NULL;
    } else {
      fptr->params = (void*)sippMemAlloc(mempool_sipp, paramSz);
    }
    
   #if defined(SIPP_F_DUMPS)
    if(name){
      strcpy(pl->filtName[fptr->id], name);
    }
   #endif
   
    pl->nFilters++;//Increment number of filters !
    return fptr;
}

//####################################################################################
//Create bidirectional link between a fitler "f" and it's parent "par"
// ("par" will also add in it's consumer link the "f" filter)
void sippLinkFilter(SippFilter *f, SippFilter *par, UInt32 vKerSz, UInt32 hKerSz)
{
    f->parents     [f->nParents] = par;

  #if 0
   //for HW filters, check that kernel sizes are within limits
   //also, for filters that have a single parent, make sure it's 0 now
   // (i.e. will be incremented to 1 later...)
    switch(f->unit)
    {
      case EXE_HW_LSC     : sippAssert(nLinesUsed == LSC_KER_SZ,  E_INVALID_HW_PARAM);  
                            sippAssert(f->nParents == 0, E_INVALID_HW_PARAM);
                            break;

      case EXE_HW_RAW     : sippAssert(nLinesUsed == RAW_KER_SZ,  E_INVALID_HW_PARAM);  
                            sippAssert(f->nParents == 0,  E_INVALID_HW_PARAM);
                            break;

      case EXE_HW_DEBAYER : sippAssert(nLinesUsed == DBYR_KER_SZ, E_INVALID_HW_PARAM);  
                            sippAssert(f->nParents == 0, E_INVALID_HW_PARAM);
                            break;

      case EXE_HW_C_DNS   : sippAssert(nLinesUsed == CDNS_KER_SZ, E_INVALID_HW_PARAM);  
                            break;

      case EXE_HW_MEDIAN  : sippAssert((nLinesUsed >= 3) && (nLinesUsed <= 7), E_INVALID_HW_PARAM);
                            sippAssert(f->nParents == 0, E_INVALID_HW_PARAM);
                            break;

      case EXE_HW_CONV    : sippAssert((nLinesUsed == 3) || (nLinesUsed == 5), E_INVALID_HW_PARAM);
                            sippAssert(f->nParents == 0, E_INVALID_HW_PARAM);
                            break;
    }
  #endif

    f->nLinesUsed[f->nParents] = vKerSz; 
    f->nParents++; //move to next parent

    //TBD: add checks: e.g. median KS <= 7 , >=3, etc...

    par->cons[par->nCons] = f;
    par->nCons++; //move to next consumer.

    //NOTE: Could add checks here: if exceeds MAX_PARENTS, MAX_CONSUMERS
    //      nParents, or nCons, could ERROR
}

//###########################################################################
//If total number of lines is != 0, it means we're using a precompued 
//schedule that inits all line buffers to proper values BEFORE sippProcessFrame
//gets invoked
int usingPrecompSched(SippPipeline *pl)
{
    UInt32 i, totLines = 0;

  //count total number of lines
    totLines = 0;
    for(i=0; i<pl->nFilters; i++)
      totLines += pl->filters[i]->nLines;

    return totLines;
}

//###########################################################################
void sippAllocLnBuffs(SippPipeline *pl)
{
    UInt32      i, j, totLines;
    SippFilter *fptr;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //1) Record the line numbers (remove this... later)
    for(i=0; i<pl->nFilters; i++)
    {

      #if defined(SIPP_PC)
        //useful for VCS setup
         printf("nLines[%2d] = %3d \n", i, pl->filters[i]->nLines);
         for(j=0; j<pl->filters[i]->nParents; j++)
             printf("    KS[%2d] = %d \n", j, pl->filters[i]->parentsKS[j]);
      #endif

        //Add one line for output line (in case filter has output buffer)
        pl->filters[i]->nLines = (pl->filters[i]->nLines == 0) ? 0 : (pl->filters[i]->nLines + 1);
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //3)Figure out h-padding ??? go throug all consumers and record MAX usage
    for(i=0; i<pl->nFilters; i++)
    {
      if(pl->filters[i]->nLines > 0)
      {
        pl->filters[i]->hPadding = 8;
      }
      else{//it buffer doesn't have OUT buffer, then padding should be ZERO
        pl->filters[i]->hPadding = 0;
      }
    }

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //4) Compute slice width, line stride, plane stride
    for(i=0; i<pl->nFilters; i++)
    {
        UInt32 nSlices = pl->gi.sliceLast - pl->gi.sliceFirst + 1;
        fptr = pl->filters[i];

        //Convert slie width based on output widht and number of shaves
        fptr->sliceWidth = (fptr->outputW + nSlices - 1) / nSlices;
        //Make slice-width a multiple of 8 pixels (to allow for SIMD optimizations
        fptr->sliceWidth = (fptr->sliceWidth + 7) & (~7);

       //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       //TBD: review this
       //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       //Round pixels per slice up to 4 pixels for efficiency (so that
       //the Shaves don't have to work on awkward multiples).
       //Would 8 be better?
        //if (fptr->sliceWidth & (fptr->align-1)) {
        //    fptr->sliceWidth += fptr->align - (fptr->sliceWidth & (fptr->align-1));
        //}
        //ALU: rounding sliceWidth is more complex: must include I/O height ratios so that
        //     we can resize

        //Compute the slice line stride (consider LEFT + RIGHT horizontal padding)
        fptr->lineStride  = (fptr->sliceWidth + fptr->hPadding * 2);
        
        //Compute plane stride (if not already assigned by user)
        fptr->planeStride = fptr->lineStride * fptr->nLines;
        
        if(fptr->shiftPlanes){
            fptr->planeStride += (pl->gi.sliceSize / fptr->bpp); //div by bpp, as will be multiplied by bpp later
                                                                 //need to clean this up...
        }
         
    }

    
    for(i=0; i<pl->nFilters; i++)
    {
      if(pl->filters[i]->nLines > 0)
      {
        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        //2) Alloc the lines in SLICE poll
        //The pointer needs to be a SHAVE-RELATIVE ptr
            UInt32 planeSize = pl->filters[i]->lineStride * pl->filters[i]->nLines;
            
        //Allocate lines for all planes; even if the data of next plane belongs to next chunk!     
            pl->filters[i]->outputBuffer = (UInt8 *)sippMemAlloc(mempool_lnbuff, 
                                                                 pl->filters[i]->nPlanes 
                                                               * planeSize        //pl->filters[i]->planeStride 
                                                               * pl->filters[i]->bpp);
          #if 0
           //Safety clamp to valid range
             if(fptr->firstOutSlc < pl->gi.sliceFirst)
                fptr->firstOutSlc = pl->gi.sliceFirst;

             if(fptr->firstOutSlc > pl->gi.sliceLast)
                fptr->firstOutSlc = pl->gi.sliceLast;
          #endif

        //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        //3) Alloc the lines-pointers in LRAM poll
         pl->filters[i]->linePtrs1stBase = (UInt8**)sippMemAlloc(mempool_sipp, 
                                                                 pl->filters[i]->nLines * 3 * sizeof(UInt32*));
         //they will be assigned to first line in the buffer in "sippFrameReset", 
         //because we need to do this every frame not just at the beginning
      }
   }

  //At the end of all allocs, display the status
   sippMemStatus();
}

//###########################################################################
void sippInitSvus(SippPipeline *pl)
{
  #if defined(__sparc)
    UInt32 s;

    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    //Setup win-regs and load the mbin
    #if defined(MYRIAD2)
       // for (s = pl->gi.sliceFirst; s <= pl->gi.sliceLast; s++)
       // {
       // //winregs already set in sippOneTimeSetup
         // sippLoadMbin(pl->mbinImg, s);
       // }
        sippLoadMbinMulti(pl->mbinImg, pl->gi.sliceFirst, pl->gi.sliceLast);
       //sippLoadMbinOpt(pl->mbinImg, pl->gi.sliceFirst, pl->gi.sliceLast);
    //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~  
    #else //myriad1
      //Set windows registers
      for (s = pl->gi.sliceFirst; s <= pl->gi.sliceLast; s++)
      {
        swcSetShaveWindows(s, pl->svuWinRegs[s*4 + 0],
                              pl->svuWinRegs[s*4 + 1],
                              pl->svuWinRegs[s*4 + 2],
                              pl->svuWinRegs[s*4 + 3]);

        swcLoadMbin(pl->mbinImg, s);
      }
    #endif
       
    //Tell shaves where is the main Pipeline structure
      for (s = pl->gi.sliceFirst; s <= pl->gi.sliceLast; s++)
        *((SippPipeline **)WRESOLVE((UInt32)&SVU_SYM(sipp_pl), s)) = pl;

    #if defined(MYRIAD2)
      for (s = pl->gi.sliceFirst; s <= pl->gi.sliceLast; s++)
      {
       //Reset and Start shaves just once at pipeline creation
       //TBD: fix sipp pipeline context switch if ever needed...
         swcResetShave(s);
         swcStartShave(s, SVU_SYM(SHAVE_MAIN));
      }
    #endif
    
  #elif defined(SIPP_PC)
    SVU_SYM(sipp_pl) = pl;  //Tell shave where is the pipe struct
  #endif
}

extern void loadCtx(SippPipeline *pl, SippFilter *fptr, UInt32 unitID);

//####################################################################################
//If a HW SIPP exec unit needs to run a fiter, load first filter
void initUnitLoad(SippPipeline *pl, UInt32 uID)
{
    UInt32      *filtCnt  = pl->hwSippFltCnt;
    
    if(filtCnt[uID]) 
    {
        Int32      *firstIdx  = pl->hwSippFirst;
        SippFilter **filters  = pl->filters;
        SippFilter *first     = filters[firstIdx[uID]];
        
        pl->hwFnLoad[uID](first, 0); //sippLoadXXXXX
      //Initially, ictx, octx = 0 (after reset), and OLD = NEW = first
        pl->hwCtxSwc[uID](pl, first, first, uID);
    }
}
//####################################################################################
// At setup, program the HW SIPP filters with first filter in the filter list 
// associated to each unit.
// In the case of single-filter/HW_unit, this is the gross setup.
// In multi-filter/HW_unit case, it's potentially useless, but no harm is done)
void sippHwInitialLoad(SippPipeline *pl)
{
    UInt32      *filtCnt = pl->hwSippFltCnt;
    Int32       *first   = pl->hwSippFirst;
    SippFilter **filters = pl->filters;

    initUnitLoad(pl, SIPP_CONV_ID    );
    initUnitLoad(pl, SIPP_MED_ID     );
    initUnitLoad(pl, SIPP_DBYR_PPM_ID);
    initUnitLoad(pl, SIPP_DBYR_ID    );
    initUnitLoad(pl, SIPP_SHARPEN_ID );
    initUnitLoad(pl, SIPP_EDGE_OP_ID );
    initUnitLoad(pl, SIPP_LUMA_ID    );
    initUnitLoad(pl, SIPP_CHROMA_ID  );
    initUnitLoad(pl, SIPP_LSC_ID     );
    initUnitLoad(pl, SIPP_HARRIS_ID  );
    initUnitLoad(pl, SIPP_RAW_ID     );
    initUnitLoad(pl, SIPP_LUT_ID     );
    initUnitLoad(pl, SIPP_UPFIRDN_ID );
    initUnitLoad(pl, SIPP_CC_ID      );
}

//####################################################################################
//Get an index to the first filter of a given type that will run on ExecUnit 
//given by "type"
void sippGetFirstHwFiltIdx(SippPipeline *pl)
{
    int i, f, u;
    int found;

    for(u=0; u<EXE_NUM; u++)
    {
        //only scan units that are used:
        if(pl->hwSippFltCnt[u]>0)
        {
            //for current unit, scan through all iterations,
            //and return first filter of type "u" that will run
            found = 0;

            for(i=0; (i<pl->nIter)&&(!found); i++)
            {
                for(f=0; f<pl->nFilters; f++)
                  if( (pl->filters[f]->unit == u) &&
                      (pl->schedInfo[i].allMask & (1<<f)) )
                  {
                      pl->hwSippFirst [u] = f;
                      pl->hwSippCurFlt[u] = pl->filters[f];
                      found               = 1;
                      break;
                  }
            }
        }
    }
}

//####################################################################################
void sippIniHwFilters(SippPipeline *pl)
{
  #if defined(MYRIAD2) || defined(SIPP_PC)
    UInt32 i;
    SippFilter **filters = pl->filters;

    for(i=0;i<pl->nFilters;i++)
    {
        switch(filters[i]->unit)
        {
            case SIPP_CONV_ID     : sippInitConv   (filters[i]); break;
            case SIPP_MED_ID      : sippInitMed    (filters[i]); break;
            case SIPP_DBYR_PPM_ID : sippInitDbyrPP (filters[i]); break;
            case SIPP_DBYR_ID     : sippInitDbyr   (filters[i]); break;
            case SIPP_SHARPEN_ID  : sippInitSharpen(filters[i]); break;
            case SIPP_EDGE_OP_ID  : sippInitEdgeOp (filters[i]); break;
            case SIPP_LUMA_ID     : sippInitLumaDns(filters[i]); break;
            case SIPP_CHROMA_ID   : sippInitChrDns (filters[i]); break;
            case SIPP_LSC_ID      : sippInitLsc    (filters[i]); break;
            case SIPP_HARRIS_ID   : sippInitHarris (filters[i]); break;
            case SIPP_RAW_ID      : sippInitRaw    (filters[i]); break;
            case SIPP_LUT_ID      : sippInitLut    (filters[i]); break;
            case SIPP_DMA_ID      : sippInitDma    (filters[i]); break;
            case SIPP_UPFIRDN_ID  : sippInitPolyFir(filters[i]); break;
            case SIPP_CC_ID       : sippInitColComb(filters[i]); break;
        }
    }
  #endif
}

//####################################################################################
void sippOneTimeSetup(SippPipeline *pl)
{
  #if defined(MYRIAD2) || defined(SIPP_PC)
    sippGetCtxOrder (pl); //determine if at least one filter requires context switch
  #endif
    
    if(!usingPrecompSched(pl))
    {
        sippSchedPipeline(pl);    //compute the main schedule 

       #if defined(MYRIAD2) || defined(SIPP_PC)
        sippGetFirstHwFiltIdx(pl);//
        sippComputeHwCtxChg  (pl);//compute context switch helpers
        
        if (pl->dbgLevel > 0) {
         sippDbgDumpSchedForVcsCArr(pl);
         sippDbgDumpAsmOffsets(pl);
        }
       #endif
    }
    else{//if using a precomputed header, we still need to know this:
        sippGetFirstHwFiltIdx(pl);
    }

   //Alloc line buffers in CMX, comupte padding strides etc...
    sippAllocLnBuffs(pl);

    #if defined(MYRIAD2)
    {//Setup win-regs; mandatory even if Shaves are not used !
     //as all addr translation will use the values programmed in win-regs
        UInt32 s;
        //for (s = pl->gi.sliceFirst; s <= pl->gi.sliceLast; s++)
        for (s = 0; s <= 11; s++)
        {
             sippSetShaveWindow(s, 0, pl->svuWinRegs[s*4 + 0]);
             sippSetShaveWindow(s, 1, pl->svuWinRegs[s*4 + 1]);
             sippSetShaveWindow(s, 2, pl->svuWinRegs[s*4 + 2]);
             sippSetShaveWindow(s, 3, pl->svuWinRegs[s*4 + 3]);
        }
    }
    #endif 
   
   //Only load code and set win-regs if at least one SW filter exits
   //this is intended to optimize VCS time
    if(pl->nSwFilters)
        sippInitSvus(pl);

  
  #if defined(MYRIAD2) || defined(SIPP_PC)
     //TBD: reset SIPP, clear ints etc...
     //put all SIPP filters in SYNC mode and enable:
       SET_REG_WORD(I_CFG(SIPP_CONV_ID    ),  1<<SIPP_SC_OFFSET);
       SET_REG_WORD(I_CFG(SIPP_MED_ID     ),  1<<SIPP_SC_OFFSET);
       SET_REG_WORD(I_CFG(SIPP_LSC_ID     ),  1<<SIPP_SC_OFFSET);
       SET_REG_WORD(I_CFG(SIPP_LSC_GM_ID  ),  1<<SIPP_SC_OFFSET); //needed?
       SET_REG_WORD(I_CFG(SIPP_HARRIS_ID  ),  1<<SIPP_SC_OFFSET);
       SET_REG_WORD(I_CFG(SIPP_LUMA_ID    ),  1<<SIPP_SC_OFFSET);
       SET_REG_WORD(I_CFG(SIPP_CHROMA_ID  ),  1<<SIPP_SC_OFFSET);
       SET_REG_WORD(I_CFG(SIPP_SHARPEN_ID ),  1<<SIPP_SC_OFFSET);
       SET_REG_WORD(I_CFG(SIPP_DBYR_ID    ),  1<<SIPP_SC_OFFSET);
       SET_REG_WORD(I_CFG(SIPP_DBYR_PPM_ID),  1<<SIPP_SC_OFFSET);
       //TBD: for all HW units
       
     //Enable all filters  
       SET_REG_WORD(SIPP_CONTROL_ADR,  0xFFFFFFFF);
       
     //enable all EOL SIPP ints
       SET_REG_WORD(SIPP_INT1_ENABLE_ADR, 0xFFFFFFFF); 
       
     //set slice size, first and last indexes
       SET_REG_WORD(SIPP_SLC_SIZE_ADR,   pl->sliceSz
                                       |(pl->gi.sliceFirst<<24)
                                       |(pl->gi.sliceLast <<28) );

  //Compute hard constants for all HW-SIPP filters, this needs to be done AFTER
  //all line buffers are allocated
    sippIniHwFilters (pl);  
    sippHwInitialLoad(pl); //load initial  SW-filters on their targets
    sippChainDmaDesc (pl);
    sippDumpHtmlMap  (pl); //DEBUG
  #endif
}
