#ifndef __SIPP_H__
#define __SIPP_H__

#include <sippPlatform.h>
#include <sippHwCommon.h>

#ifndef NULL
#define NULL 0
#endif

  #if defined(SIPP_PC) && 1
   //SIPP Filter Output Dumps, undef to cancel dumps
    #define SIPP_F_DUMPS 
  #endif

  #define SIPP_AUTO (-1)

//buffer max allocations
  #define MAX_PARENTS        4
  #define MAX_CONSUMERS      4
  #define MAX_FILTERS       32

  #ifndef BUFF_HUGE_SZ
  #define BUFF_HUGE_SZ     128 //MAX number of lines allocated in each buffer at schedule time
  #endif

//Sipp Flags (SippFilter.flags)
  #define SIPP_FLAG_DO_H_PADDING  (1<<3)
  #define SIPP_RESIZE             (1<<4)
  #define SIPP_CROP               (1<<5)

//Commands from LEON to Shave 
  #define CMD_H_PAD 0x01
  #define CMD_RUN   0x02
  #define CMD_EXIT  0x04

//Macros that do nothing, but make calls to sipp_filter_init() more readable 
  #define N_PL(x)   (x) // number of planes
  #define BPP(x)    (x) // Bytes per pixel of the output buffer 
  #define  SZ(x)    sizeof(x)

//Early typedef declarations
  typedef struct SippFilterS   SippFilter;
  typedef struct SippPipelineS SippPipeline;
  typedef struct SippSchedS    SippSched;
  typedef struct SchedInfoS    SchedInfo;

  typedef void (*FnSvuRun)   (SippFilter   *fptr, int svuNo, int runNo);
  typedef void (*FnHwFltLoad)(SippFilter   *fptr, UInt32 ctx); //loads double buffered regs on a target
  typedef void (*FnHwLnSetup)(SippFilter   *fptr, UInt32 ctx); //per line Leon setup required for some modules (RAW, DS, POLY)

  typedef void (*FnCtxSwitch)(SippPipeline *pl, SippFilter *newF, SippFilter *oldF, UInt32 unitID);
//#############################################################################
typedef struct
{
    UInt32        sliceFirst;     //First slice [0..11]
    UInt32        sliceLast;      //Last slice [0..11]
    UInt32        sliceSize;      //Slice Size in Bytes
    UInt32        curFrame;       //Current frame number
    SippPipeline *pl;             //ref to pipeline struct (needed by Shaves...)

  //Addr translation members
    UInt32        wrapAddr;       //any resolved address that is higherthan this, should wrap
    Int32         wrapCT;         //Wrap Constant that is to be added to overflowing addr
    UInt32        cmxBase;        //CMX base to cope with PC/Fragrak
}CommInfo;

//#############################################################################
//Error codes
enum
{
     E_SUCCESS          = 0,
     E_OUT_OF_MEM       = 1,
     E_INVALID_MEM_P    = 2,
     E_PAR_NOT_FOUND    = 3,
     E_DATA_NOT_FOUND   = 4,
     E_RUN_DON_T_KNOW   = 5,
     E_INVALID_HW_PARAM = 6
};

//Extend a bit the ID list, to have a centralized 
// and unique identification mode
#if(SIPP_MAX_ID>30)
#error DMA_and_SVU_IDs_are_out_of_range_!
#endif

//TBD: review these... could use SIPP_MAX_ID ?
#define SIPP_DMA_ID (SIPP_MAX_ID+1)
#define SIPP_SVU_ID (SIPP_MAX_ID+2)

#define EXE_NUM     (SIPP_MAX_ID+2+1)


//#############################################################################
//Scheduling status enum
typedef enum{
  RS_DONT_KNOW = 0,
  RS_CAN_RUN   = 1,
  RS_CANNOT    = 2
}RunStatus;

//Scheduling Info at Setup
struct SchedInfoS
{
    UInt32 sippHwStartMask;
    UInt32 sippHwWaitMask;
    UInt32 shaveMask;//[2]
    UInt32 dmaMask;  //bottom 24 bits
    UInt32 allMask;
    //UInt32 *ksUpdates;
};

//TBD: Compact & variable format Scheduling Info at Runtime
//     use some offsets in SippPipeline to know where each field starts !

//=============================================================================
struct SippPipelineS 
{
    UInt32      old_mask;
    UInt32      can_run_mask;
    UInt32      nFilters;                 //Number of filters in pipeline
    UInt32      nSwFilters;               //Number of SW filters in pipeline
    UInt32     *svuWinRegs;               //Shaves win regs : 4wins x 8haves
    UInt8      *mbinImg;                  //Shaves SIPP mbin
    UInt32      nIter;                    //number of iterations / frame
    UInt32      svuCmd;                   //Command to Shave (gets cleared by Master Shave later...)

    SippFilter *filters[MAX_FILTERS];     //All fillters

  //References to filters specific to each exec unit.
  //This would speedup context-switch, as we'll only dig through
  //less filters
    SippFilter *filtersSvu   [MAX_FILTERS];  UInt32 nFiltersSvu;
    SippFilter *filtersDMA   [8];            UInt32 nFiltersDMA; 

  //TBD: clean up here, I'md not sure what's really needed...
    Int32       hwSippFirst [EXE_NUM]; //index of first filter to run on a given HW unit
    UInt32      hwSippFltCnt[EXE_NUM]; //num of filters that execute on a given HW unit
    UInt32      hwSippCurCmd[EXE_NUM]; //current command index
    SippFilter *hwSippCurFlt[EXE_NUM]; //reference to current filter that executes on a HW unit

    Int32       hwSippCtxOff[EXE_NUM]; //offsets within current schedule info where associated info exists
                                       //can be different form pipeline to pipeline, but will remain CT for a given pipeline

    UInt32      ictxAddr[EXE_NUM][2];  //put all ctx switch addresses in array, as we need to deal with these programatically
    UInt32      octxAddr[EXE_NUM];
    FnHwFltLoad hwFnLoad[EXE_NUM];
    FnHwLnSetup hwFnLnIni[EXE_NUM];
    FnCtxSwitch hwCtxSwc[EXE_NUM];

    UInt32      shadowSelect;

    UInt32      hwSippCtxSwMask;       //altenate to SIPP_INT1_ADR, to cope with multiple-context/HW_unit

    SchedInfo   *schedInfo;            //Fixed-size schedule info (all/shave/dma/sipp_start masks)
    UInt32       schedInfoEntries;    

    UInt32      *schedInfoCtx;         //Variable sched info for sipp HW filters CTX switch
    UInt32       schedInfoCtxSz;       //size in UInt32 words (this offset gets added to *schedInfoCtx)

    CommInfo    gi;                    //Global info
    UInt32      sliceSz;               //Slice size (for chunking mode)
    int         dbgLevel;
    UInt32      iteration;             //Current iteration

  #if defined(SIPP_PC) || defined(MYRIAD2)
   //Myriad2 Shave Sync-Barrier variables
    UInt8       svuStat     [12];  //Shaves status: 0=idle; 1=running
    UInt8       svuStartMask[12];  //Shave start mask, based on sliceFirst/Last markers
    UInt32      multiHwCtx;        //0: if num_Filter per HW unit <=1; 1 else
  #endif

  #ifdef SIPP_F_DUMPS //debug output files for all filters in pipeline
    FILE       *filtFile[MAX_FILTERS];
    char        filtName[MAX_FILTERS][256];
  #endif
};



//#############################################################################
// Aditional data structure required ONLY for scheduling & full-frame test
struct SippSchedS 
{
  //nParents already available in filter def
    UInt32   parentsKsClr[MAX_PARENTS]; 
    UInt32   parentsKsMin[MAX_PARENTS];

  //some consumer info:
  //the number of lines required is SippFilter->n_line_used
    int        *reqInLns  [MAX_PARENTS]; //lines needed (Explicit listing)
  
    int        lineIndices[BUFF_HUGE_SZ];
    RunStatus  canRunP; //can run due to Parent   conditions
    RunStatus  canRunC; //cna run due to Consumer conditions
    int        dbgJustRoll; 
    int        curLine;
};

//#############################################################################
//Input & Output buffer data structure that maps on HW register lists
//Some HW filters have mutliple input buffers...
typedef struct
{
    UInt32 base;     //+0x0000 : base address
    UInt32 cfg;      //+0x0004 : configuration
    UInt32 ls;       //+0x0008 : line stride
    UInt32 ps;       //+0x000C : plane stride
    UInt32 irqRate;  //+0x0010 : fileds for Chunk-size !
    UInt32 fillCtrl; //+0x0014 : unused in sync mode 
    UInt32 ctx;      //+0x0018 : context/status
}SippHwBuf;

void sippIbufSetup(SippFilter *fptr, UInt32 parNo);
void sippObufSetup(SippFilter *fptr);


//#############################################################################
struct SippFilterS 
{
  //TBD: put back the logic ordering here for readability
  //     caches are OK now
    UInt32      nCons;
    UInt8      *outLinePtr;       //Ptr to current produced line
    UInt8      *linePtrs;         //Circular output buffer linear view
    UInt8      *linePtrs3rdBase;  //
    UInt8      *linePtrs2ndBase;  //
    SippFilter *parents   [MAX_PARENTS];
    UInt32      parentsKS [MAX_PARENTS]; //in parent buffs
    UInt32      outputH;          //output height = number of runs per frame
    UInt32      schNo;            //Current scheduled num (can be in advance with 1 iteration vs. runNo)
    UInt8      *lnToPad   [2];    //line to be padded
    UInt8      *outLinePtrPrev;   //Ptr to previous produced line

    

 //These functions run on the RISC
    void (*funcAsk )   (SippFilter *fptr, int line_no); //scheduling & test purposes

 //Filter-Shave function (if any)
    FnSvuRun     funcSvuRun;

 //Points to filter's private persistent data store, if any
    void       *params; //filter specific parameters
    UInt32      flags;

 //Parent info
    UInt32      nParents;
    UInt32      nLinesUsed[MAX_PARENTS]; //number of lines needed from parent buffs

 //Consumer data 
    SippFilter *cons[MAX_CONSUMERS];

 //Double bufferered base line pointers
    UInt32    *dbLinesIn[MAX_PARENTS][2];
    UInt8     *dbLineOut [2];
    
               
    UInt32    outputW;          //Total line width, ignoring slices [pixels]
    UInt32    sliceWidth;       //One slice's output width          [pixels]
    UInt32    bpp;              //Bytes per pixel
    UInt32    nPlanes;          //Number of planes
    UInt32    lineStride;       //Line stride [pixels] (including Left+Right hPadding)
    UInt32    planeStride;      //Plane stride [pixels] (including paddings)
    UInt32    hPadding;         //Horizontal Padding [pixels]
               
    UInt8     *outputBuffer;    //The OUTPUT BUFFER
    
    UInt8     **linePtrs1stBase;//

    UInt32    nLines;           //Number of lines in output buffer
    UInt32    id;               //Filter unique id (gets allocated incremently at CREATE time)
    
    UInt32    outputBufferIdx;  //alu: current line to be filled (max value = nLines-1) circular rolling
    UInt32    exeNo;            //Actual number of executed runs
    UInt32    nCtxLoads;

    UInt32    unit;             //HW Execution unit that will actually run the filter
    const CommInfo *gi;

   //For HW-SIPP filters that require context switch, alloc I/O Buff configs
   //(including ICTX/OCTX). Filter specific params are already stored in params aea
    SippHwBuf *iBuf[2]; //some filters use 2 input buffers
    SippHwBuf *oBuf;

    UInt32    firstOutSlc;     //First output slice
    UInt32    shiftPlanes;     //Flag to indicate shifted plane mode

    //--------------------------------------------------------
    SippSched *sch;  //used at schedule & PC_test time ONLY
    //--------------------------------------------------------
};

//#############################################################################
//Memory pools definition
typedef enum {
    mempool_sipp   = 0,
    mempool_lnbuff = 1,
    mempool_sched  = 2,
    mempool_npools = 3
} SippMemPoolType; //SIPP_INTERNAL

//#############################################################################
//Pipeline creation:
SippPipeline* sippCreatePipeline(UInt32  shaveFirst,
                                 UInt32  shaveLast,
                                 UInt32 *svuWinRegs,
                                 UInt8  *mbinImg);

SippFilter * sippCreateFilter(SippPipeline *pl, 
                              UInt32      flags,
                              UInt32      outW, 
                              UInt32      outH,
                              UInt32      numPl,
                              UInt32      bpp,
                              UInt32      paramsAlloc,
                              void        (*funcSvuRun)(struct SippFilterS *fptr, int svuNo, int runNo),
                              const char  *name);

void sippLinkFilter(SippFilter *f, 
                    SippFilter *par, 
                    UInt32 vKerSz,   //Vertical kernel size == number of lines used
                    UInt32 hKerSz);  //Horizontal kernel size [pixels] for h-padding

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Memory allocation
void * sippMemAlloc(SippMemPoolType pool, int n_bytes);  //SIPP_INTERNAL
void   sippMemReset(UInt32 sliceFirst, 
                    UInt32 sliceLast, 
                    UInt32 sliceSize);         //SIPP_INTERNAL 
void   sippMemStatus();                        //SIPP_INTERNAL : debug...

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Sipp internal functions 
void sippSchedPipeline(SippPipeline *pl);
void sippProcessFrame (SippPipeline *pl);
void sippChainDmaDesc (SippPipeline *pl);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Schedule RD/WR routines (Only update & test in pair!)
void   sippSchedWr(SippPipeline *pl, UInt32 iteration); //SIPP_INTERNAL
UInt32 sippSchedRd(SippPipeline *pl, UInt32 iteration); //SIPP_INTERNAL

void sippGetCtxOrder     (SippPipeline *pl);  //SIPP_INTERNAL
void sippComputeHwCtxChg (SippPipeline *pl);  //SIPP_INTERNAL
void sippHandleCtxSwitch (SippPipeline *pl);  //SIPP_INTERNAL

//Context switch helpers
void ctxSwitchOnePar   (SippPipeline *pl, SippFilter *newF, SippFilter *oldF, UInt32 unitID);
void ctxSwitchTwoPar   (SippPipeline *pl, SippFilter *newF, SippFilter *oldF, UInt32 unitID);
void ctxSwitchChromaDns(SippPipeline *pl, SippFilter *newF, SippFilter *oldF, UInt32 unitID);
void ctxSwitchLut      (SippPipeline *pl, SippFilter *newF, SippFilter *oldF, UInt32 unitID);
void ctxSwitchPoly     (SippPipeline *pl, SippFilter *newF, SippFilter *oldF, UInt32 unitID);
void ctxSwitchColComb  (SippPipeline *pl, SippFilter *newF, SippFilter *oldF, UInt32 unitID);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Utils. make these work on Sabre as well
void sippRdFileU8      (UInt8 *buff, int count, const char *fName);
void sippWrFileU8      (UInt8 *buff, int count, const char *fName);
void sippRdFileU8toF16 (half  *buff, int count, const char *fName);
void sippWrFileF16toU8 (half  *buff, int count, const char *fName);

//Schedule helpers (Internal)
void askRegular    (SippFilter *fptr, int outLineNo); //SIPP_INTERNAL
void askResizer    (SippFilter *fptr, int outLineNo); //SIPP_INTERNAL
void askCrop       (SippFilter *fptr, int outLineNo); //SIPP_INTERNAL
void askHwDebayer  (SippFilter *fptr, int outLineNo); //SIPP_INTERNAL
void askHwColorComb(SippFilter *fptr, int outLineNo);
void askHwHarris   (SippFilter *fptr, int outLineNo); //SIPP_INTERNAL

void sippError    (UInt32 errCode);                   //SIPP_INTERNAL
void sippAssert   (UInt32 condition, UInt32 err_code);//SIPP_INTERNAL

void sippDbgLevel              (SippPipeline *pl, int level);
void sippDbgPrintRunnable      (SippFilter *filters[], UInt32 nFilters, UInt32 iteration); //SIPP_INTERNAL
void sippDbgDumpBuffState      (SippFilter *filters[], UInt32 nFilters, UInt32 iteration); //SIPP_INTERNAL
void sippDbgShowBufferReq      (SippFilter *filters[], UInt32 nFilters);                   //SIPP_INTERNAL
void sippDbgPrintNumPar        (SippFilter *filters[], UInt32 nFilters);                   //SIPP_INTERNAL
void sippDbgShowBuffPtr        (SippFilter* fptr, const char *msg);                        //SIPP_INTERNAL
void sippDbgDumpRunMask        (UInt32 mask,    int iteration, int dbgDump);               //SIPP_INTERNAL
void sippDbgFrameCheck         (SippPipeline *pl);                                         //SIPP_INTERNAL
void sippDbgDumpSchedForVcs    (SippPipeline *pl);
void sippDbgDumpSchedForVcsCArr(SippPipeline *pl);
void sippDbgDumpGraph          (SippPipeline *pl, const char *fname);
void sippDbgCreateDumpFiles    (SippPipeline *pl);
void sippDbgDumpFilterOuts     (SippPipeline *pl);
void sippDbgDumpAsmOffsets     (SippPipeline *pl);
void sippDumpHtmlMap           (SippPipeline *pl);

void sippDbgCompareU8 (UInt8  *refA, UInt8  *refB, int len);
void sippDbgCompareU16(UInt16 *refA, UInt16 *refB, int len);

extern SippPipeline*  SVU_SYM(sipp_pl);


//#####################################
#include <sippHwDefs.h>     // only for Myriad2 and PC model

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Utils
void packConv5x5CCM     (ConvParam    *cfg, UInt16 *ccm5x5);
void packConv3x3CCM     (ConvParam    *cfg, UInt16 *ccm3x3);
void packColCombCCM     (ColCombParam *cfg, float  *ccm3x3);
void packLumaDnsGaussLut(YDnsParam    *cfg, UInt8  *lut   );

void sippUtilComputeFp16Lut  (half (*formula)(half input), half *outLut, UInt32 lutSize);

void sharpenSigmaToCoefficients(float sigma, int *coeffs);
void lumaGenLut                (float strength, UInt8 *lut, int *bitpos);


//Shave filter utils:
#if defined(SIPP_PC)
 UInt32 getInPtr (SippFilter *fptr, UInt32 parent, UInt32 lineNo, UInt32 planeNo);
 UInt32 getOutPtr(SippFilter *fptr, UInt32 planeNo);
#endif

#endif // !__SIPP_H__ 
