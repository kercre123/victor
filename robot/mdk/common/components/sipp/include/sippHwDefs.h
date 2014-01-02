#ifndef __SIPP_HW_DEFS_H__
#define __SIPP_HW_DEFS_H__


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Nice macros to access addresses based on HW resources IDs
#define CONCAT3(A, B, C) A ## B ## C

#define I_BASE(X) CONCAT3(SIPP_IBUF, X, _BASE_ADR)
#define I_CFG(X)  CONCAT3(SIPP_IBUF, X, _CFG_ADR)
#define I_LS(X)   CONCAT3(SIPP_IBUF, X, _LS_ADR)
#define I_PS(X)   CONCAT3(SIPP_IBUF, X, _PS_ADR)
#define O_BASE(X) CONCAT3(SIPP_OBUF, X, _BASE_ADR)

#define I_CTX(X)  CONCAT3(SIPP_ICTX, X, _ADR)
#define O_CTX(X)  CONCAT3(SIPP_OCTX, X, _ADR)

#define I_SHADOW_BASE(X) CONCAT3(SIPP_IBUF, X, _BASE_SHADOW_ADR)
#define I_SHADOW_CFG(X)  CONCAT3(SIPP_IBUF, X, _CFG_SHADOW_ADR)
#define I_SHADOW_LS(X)   CONCAT3(SIPP_IBUF, X, _LS_SHADOW_ADR)
#define I_SHADOW_PS(X)   CONCAT3(SIPP_IBUF, X, _PS_SHADOW_ADR)
#define O_SHADOW_BASE(X) CONCAT3(SIPP_OBUF, X, _BASE_SHADOW_ADR)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//Program a Input/Output buffer 
#define PROG_IO_BUFF(target, src) \
   SET_REG_WORD((UInt32)&target->base,    src->base); \
   SET_REG_WORD((UInt32)&target->cfg,     src->cfg);  \
   SET_REG_WORD((UInt32)&target->ls,      src->ls);   \
   SET_REG_WORD((UInt32)&target->ps,      src->ps);   \
   SET_REG_WORD((UInt32)&target->irqRate, src->irqRate);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define LSC_KER_SZ       1
#define RAW_KER_SZ       5
#define DBYR_KER_SZ     11
#define CDNS_KER_SZ     11
#define DBYR_KER_SZ_M1 (DBYR_KER_SZ-1)

//Note: on SPARC, SET_REG_WORD is already defined ...
//      on Shave, it makes no sense, so only let definition for Windows-Leon 
#ifdef SIPP_PC
 void   SET_REG_WORD    (UInt32 addr, UInt32 value);
 UInt32 GET_REG_WORD_VAL(UInt32 addr);
 void   SET_REG_DWORD   (UInt32 addr, UInt64 value);
#endif

//===================================================================
typedef struct {
 
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //Myriad2 DMA 2D-chunked descriptor, as in CMXDMA_controller.doc
  //Note: it's like an inline data structure, will use DmaParam 
  //      to refer to the descriptor ...
  #if defined(MYRIAD2) || defined(SIPP_PC)
    UInt64 dscCtrlLinkAddr; //CDMA_CFG_LINK_ADR
    UInt64 dscDstSrcAddr;   //CDMA_DST_SRC_ADR    
    UInt64 dscPlanesLen;    //CDMA_LEN_ADR
    UInt64 dscSrcStrdWidth; //CDMA_SRC_STRIDE_WIDTH_ADR
    UInt64 dscDstStrdWidth; //CDMA_DST_STRIDE_WIDTH_ADR
    UInt64 dscPlStrides;    //CMDA_PLANE_STRIDE_WIDTH
  #endif
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  //User level params to customize transfer
    UInt32 ddrAddr; 

    UInt32 dstChkW, srcChkW;
    UInt32 dstChkS, srcChkS;
    UInt32 dstPlS,  srcPlS;
    UInt32 dstLnS,  srcLnS; //full line strides
}DmaParam;

#define DMA_MASK 0x00FFFFFF //allow 24 dma filters

void sippKickDma  (SippPipeline *pl);
void sippWaitDma  (UInt32 waitMask);


//===================================================================
void sippKickShave(SippPipeline *pl);
void sippWaitShave(SippPipeline *pl);

//===================================================================

typedef struct {
    UInt32  frmDim; //internal
    UInt32  cfg;
}MedParam;

//===================================================================
typedef struct{
    UInt32  frmDim;   //intrenal
    UInt32  fraction; //intrenal
    UInt32  gmDim;    //intrenal
    UInt32  cfg;
    UInt16 *gmBase;     //[Gain Map] base
    UInt32  gmWidth;    //[Gain Map] width
    UInt32  gmHeight;   //[Gain Map] height
    UInt32  dataFormat; //Planar(0), Bayer(1)
    UInt32  dataWidth;  //8-16 bit
}LscParam;


//===================================================================
//Raw block params
typedef struct {
    UInt32 frmDim;
    UInt32 grgbPlat;  //(bright<<16) | (dark)
    UInt32 grgbDecay; //(bright<<16) | (dark)
    UInt32 badPixCfg;
    UInt32 cfg;
    UInt32 gainSat[2][4]; //4 gains for each 2 lines
}RawParam;

//===================================================================
//Debayer params
typedef struct {
    UInt32 frmDim;
    UInt32 cfg;
    UInt32 thresh;
    UInt32 dewormCfg;
}DbyrParam;

//===================================================================
//Debayer Post Processing params
typedef struct {
    UInt32 frmDim;
    UInt32 cfg;
}DbyrPpParam;

//===================================================================
//Sharpen params
typedef struct {
    UInt32 frmDim;
    UInt32 cfg;
    UInt32 strenAlpha;
    UInt32 clip;
    UInt32 limit;
    UInt32 rgnStop01;
    UInt32 rgnStop23;
    UInt32 coef01;
    UInt32 coef23;
}UsmParam;

//===================================================================
//Luma denoise params
typedef struct {
    UInt32  frmDim;
    UInt32  cfg;   //bitpos, alpha
    UInt32  gaussLut[4];
    UInt32  f2;
}YDnsParam;

//===================================================================
//Chroma denoise
typedef struct {
    UInt32 frmDim;
    UInt32 cfg;
    UInt32 thr[2];
}ChrDnsParam;

//===================================================================
//Chroma denoise
typedef struct {
    UInt32 frmDim;
    UInt32 cfg;
    UInt32 sizeA;
    UInt32 sizeB;
    void  *lut; //u8 or fp16
}LutParam;

//===================================================================
//Color Combination params
typedef struct
{
  UInt32 frmDim; //internal
  UInt32 cfg;
  UInt32 krgb[2]; //4.8
  UInt32 ccm [5]; //6.10
}ColCombParam;

//===================================================================
//Convolution params
typedef struct 
{
    UInt32  frmDim;    //internal
    UInt32  cfg;
    UInt32  kernel[15];//fp16 values
}ConvParam;

//#######################################################
//Harris corners detect params
typedef struct {
    UInt32 frmDim;
    UInt32 cfg;
    float  kValue;
}HarrisParam;

//#######################################################
//Polyphase Scaler params

typedef enum {
    POLY_MODE_AUTO    = 0,
    POLY_MODE_ADVANCE = 1
}PolyModes;

typedef enum {
    POLY_LANCZOS    = 0,
    POLY_BICUBIC    = 1,
    POLY_BILINEAR   = 2
}PolyScalerType;

typedef struct {
    UInt32         cfgReg; //Internal
    UInt32         kerSz;  //Internal
    UInt32         frmDimPar;
    UInt32         frmDimFlt;

    PolyModes      mode;
    PolyScalerType autoType;
    // This parameters should be set just for advance mode
    // for auto mode will be calculated internaly by sipp model
    UInt32 clamp;//   : 1;
    UInt32 horzD;//   : 6; Horizontal Denominator factor
    UInt32 horzN;//   : 5; Horizontal Numerator   factor
    UInt32 vertD;//   : 6; Vertical   Denominator factor
    UInt32 vertN;//   : 5; Vertical   Numerator   factor
    UInt16 *horzCoefs;
    UInt16 *vertCoefs;
}PolyFirParam;


//#######################################################
//Edge operator params
typedef struct {
    UInt32 frmDim;
    UInt32 cfg;
    UInt32 xCoeff;
    UInt32 yCoeff;
}EdgeParam;

//##################################################
//Filter Inits
void sippInitConv   (SippFilter *fptr);
void sippInitMed    (SippFilter *fptr);
void sippInitDbyrPP (SippFilter *fptr);
void sippInitDbyr   (SippFilter *fptr);
void sippInitSharpen(SippFilter *fptr);
void sippInitEdgeOp (SippFilter *fptr);
void sippInitLumaDns(SippFilter *fptr);
void sippInitChrDns (SippFilter *fptr);
void sippInitLsc    (SippFilter *fptr);
void sippInitHarris (SippFilter *fptr);
void sippInitRaw    (SippFilter *fptr);
void sippInitLut    (SippFilter *fptr);
void sippInitDma    (SippFilter *fptr);
void sippInitPolyFir(SippFilter *fptr);
void sippInitColComb(SippFilter *fptr);

//##################################################
// Filter loads
void sippLoadConv   (SippFilter *fptr, UInt32 ctx);
void sippLoadMed    (SippFilter *fptr, UInt32 ctx);
void sippLoadDbyrPP (SippFilter *fptr, UInt32 ctx);
void sippLoadDbyr   (SippFilter *fptr, UInt32 ctx);
void sippLoadSharpen(SippFilter *fptr, UInt32 ctx);
void sippLoadEdgeOp (SippFilter *fptr, UInt32 ctx);
void sippLoadLumaDns(SippFilter *fptr, UInt32 ctx);
void sippLoadChrDns (SippFilter *fptr, UInt32 ctx);
void sippLoadLsc    (SippFilter *fptr, UInt32 ctx);
void sippLoadHarris (SippFilter *fptr, UInt32 ctx);
void sippLoadRaw    (SippFilter *fptr, UInt32 ctx);
void sippLoadLut    (SippFilter *fptr, UInt32 ctx);
void sippLoadPolyFir(SippFilter *fptr, UInt32 ctx);
void sippLoadColComb(SippFilter *fptr, UInt32 ctx);

//##################################################
// Filter line init setups
void sippSetupRawLine (SippFilter *fptr, UInt32 ctx);
void sippSetupPolyLine(SippFilter *fptr, UInt32 ctx);

#endif // !__SIPP_HW_DEFS_H__ 
