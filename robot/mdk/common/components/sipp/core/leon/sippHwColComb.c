#include <sipp.h>

#if (defined(SIPP_PC) || defined(MYRIAD2))

//########################################################################################
void sippInitColComb(SippFilter *fptr)
{
   ColCombParam *param = (ColCombParam*)fptr->params;
   
   sippIbufSetup(fptr, 0); //Compact Input buffer : Luma
   sippIbufSetup(fptr, 1); //Compact Input buffer : Chroma planes
   sippObufSetup(fptr);    //Compact Output buffer

 //One time constants updates
   param->frmDim = (fptr->outputH << SIPP_IMGDIM_SIZE)|fptr->outputW;
}

//########################################################################################
void sippLoadColComb(SippFilter *fptr, UInt32 ctx)
{
    //empty : there are no double buffered regs, so can't load anything in advance
}

//########################################################################################
//Color Comb has no double buffered regs, so need a fresh setup. 
//Will be slow but hopefully never need this.
void ctxSwitchColComb(SippPipeline *pl, SippFilter *newF, SippFilter *oldF, UInt32 unitID)
{
   SippHwBuf *srcI1    = newF->iBuf[0];
   SippHwBuf *srcI2    = newF->iBuf[1];
   SippHwBuf *srcO     = newF->oBuf;
   SippHwBuf *targetI1 = (SippHwBuf*)I_BASE(SIPP_CC_ID);
   SippHwBuf *targetI2 = (SippHwBuf*)I_BASE(SIPP_CC_CHROMA_ID);
   SippHwBuf *targetO  = (SippHwBuf*)O_BASE(SIPP_CC_ID);
   ColCombParam *param = (ColCombParam*)newF->params;

 //logically I'd put "Save old filter ctx" first, but breaks on SIM
 //Program new I & O buffers (this shouldn't change CTX)
   PROG_IO_BUFF(targetI1, srcI1); //Input-Img
   PROG_IO_BUFF(targetI2, srcI2); //Input-Ref
   PROG_IO_BUFF(targetO , srcO ); //Output

 //Save old filter context
   oldF->iBuf[0]->ctx    = GET_REG_WORD_VAL(pl->ictxAddr[unitID][0]);
   oldF->iBuf[1]->ctx    = GET_REG_WORD_VAL(pl->ictxAddr[unitID][1]);
   oldF->oBuf->ctx       = GET_REG_WORD_VAL(pl->octxAddr[unitID]);

 //Color Comb specific params:
   SET_REG_WORD(SIPP_CC_FRM_DIM_ADR, param->frmDim); //typical Resolution  setup (LUMA)
   SET_REG_WORD(SIPP_CC_CFG_ADR    , param->cfg);
   SET_REG_WORD(SIPP_CC_KRGB0_ADR  , param->krgb[0]);
   SET_REG_WORD(SIPP_CC_KRGB1_ADR  , param->krgb[1]);
   SET_REG_WORD(SIPP_CC_CCM10_ADR  , param->ccm[0]);
   SET_REG_WORD(SIPP_CC_CCM32_ADR  , param->ccm[1]);
   SET_REG_WORD(SIPP_CC_CCM54_ADR  , param->ccm[2]);
   SET_REG_WORD(SIPP_CC_CCM76_ADR  , param->ccm[3]);
   SET_REG_WORD(SIPP_CC_CCM8_ADR   , param->ccm[4]);

 //CTX-es last:
   SET_REG_WORD(pl->ictxAddr[unitID][0], (1<<31) | newF->iBuf[0]->ctx);
   SET_REG_WORD(pl->ictxAddr[unitID][1], (1<<31) | newF->iBuf[1]->ctx);
   SET_REG_WORD(pl->octxAddr[unitID]   , (1<<31) | newF->oBuf->ctx);
}

#endif