#include <sipp.h>

#if (defined(SIPP_PC) || defined(MYRIAD2))

//#######################################################################################
void sippInitLumaDns(SippFilter *fptr)
{
   YDnsParam    *param    = (YDnsParam*)fptr->params;
   
   sippIbufSetup(fptr, 0); //Compact Input buffer : Img
   sippIbufSetup(fptr, 1); //Compact Input buffer : Ref
   sippObufSetup(fptr);    //Compact Output buffer

 //One time constants updates
   param->frmDim = (fptr->parents[0]->outputH << SIPP_IMGDIM_SIZE)|fptr->parents[0]->outputW;
}
//#######################################################################################

void sippLoadLumaDns(SippFilter *fptr, UInt32 ctx)
{
   SippHwBuf *targetI1, *targetI2;
   SippHwBuf *targetO;
   UInt32     paramSet;
   SippHwBuf *srcI1 = fptr->iBuf[0];
   SippHwBuf *srcI2 = fptr->iBuf[1];
   SippHwBuf *srcO  = fptr->oBuf;
   YDnsParam *param = (YDnsParam*)fptr->params;

 //Figure out which set we need to use: Main(ctx=0) or Shadow(ctx=1)
   if(0 == ctx)
   {
       targetI1 = (SippHwBuf*)I_BASE(SIPP_LUMA_ID);
       targetI2 = (SippHwBuf*)I_BASE(SIPP_LUMA_REF_ID);
       targetO  = (SippHwBuf*)O_BASE(SIPP_LUMA_ID);
       paramSet = SIPP_LUMA_BASE_ADR;
   }
   else{
       targetI1 = (SippHwBuf*)I_SHADOW_BASE(SIPP_LUMA_ID);
       targetI2 = (SippHwBuf*)I_SHADOW_BASE(SIPP_LUMA_REF_ID);
       targetO  = (SippHwBuf*)O_SHADOW_BASE(SIPP_LUMA_ID);
       paramSet = SIPP_LUMA_SHADOW_ADR;
   }

 //Program I & O buffers
   PROG_IO_BUFF(targetI1, srcI1); //Input-Img
   PROG_IO_BUFF(targetI2, srcI2); //Input-Ref
   PROG_IO_BUFF(targetO , srcO ); //Output

 //Custom params:
   SET_REG_WORD(paramSet + 0x00, param->frmDim     );  //SIPP_LUMA_FRM_DIM_ADR
   SET_REG_WORD(paramSet + 0x04, param->cfg        );  //SIPP_LUMA_CFG_ADR    
   SET_REG_WORD(paramSet + 0x08, param->gaussLut[0]);  //SIPP_LUMA_LUT0700_ADR
   SET_REG_WORD(paramSet + 0x0c, param->gaussLut[1]);  //SIPP_LUMA_LUT1508_ADR
   SET_REG_WORD(paramSet + 0x10, param->gaussLut[2]);  //SIPP_LUMA_LUT2316_ADR
   SET_REG_WORD(paramSet + 0x14, param->gaussLut[3]);  //SIPP_LUMA_LUT3124_ADR
   SET_REG_WORD(paramSet + 0x18, param->f2         );  //SIPP_LUMA_F2LUT_ADR  
}
#endif