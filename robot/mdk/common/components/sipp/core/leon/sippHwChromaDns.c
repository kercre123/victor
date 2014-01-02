#include <sipp.h>

#if (defined(SIPP_PC) || defined(MYRIAD2))

//#######################################################################################
void sippInitChrDns(SippFilter *fptr)
{
   ChrDnsParam *param = (ChrDnsParam*)fptr->params;
   
   sippIbufSetup(fptr, 0); //Compact Input buffer : Img
   if(fptr->nParents == 2)
      sippIbufSetup(fptr, 1); //Compact Input buffer : Ref

   sippObufSetup(fptr);    //Compact Output buffer

 //One time constants updates
   param->frmDim = (fptr->parents[0]->outputH << SIPP_IMGDIM_SIZE)|fptr->parents[0]->outputW;
}

//#######################################################################################
void sippLoadChrDns(SippFilter *fptr, UInt32 ctx)
{
   SippHwBuf   *targetI1, *targetI2;
   SippHwBuf   *targetO;
   UInt32       paramSet;
   SippHwBuf   *srcI1 = fptr->iBuf[0];
   SippHwBuf   *srcI2 = fptr->iBuf[1]; //can ben null
   SippHwBuf   *srcO  = fptr->oBuf;
   ChrDnsParam *param = (ChrDnsParam*)fptr->params;

 //Figure out which set we need to use: Main(ctx=0) or Shadow(ctx=1)
   if(0 == ctx)
   {
       targetI1   = (SippHwBuf*)I_BASE(SIPP_CHROMA_ID);
       if(fptr->nParents == 2)
       {
         targetI2 = (SippHwBuf*)I_BASE(SIPP_CHROMA_REF_ID);
       }
       targetO    = (SippHwBuf*)O_BASE(SIPP_CHROMA_ID);
       paramSet   = SIPP_CHROMA_BASE_ADR;
   }
   else{
       targetI1   = (SippHwBuf*)I_SHADOW_BASE(SIPP_CHROMA_ID);
       if(fptr->nParents == 2)
       {
         targetI2 = (SippHwBuf*)I_SHADOW_BASE(SIPP_CHROMA_REF_ID);
       }
       targetO    = (SippHwBuf*)O_SHADOW_BASE(SIPP_CHROMA_ID);
       paramSet   = SIPP_CHROMA_SHADOW_ADR;
   }

 //Program I & O buffers
   PROG_IO_BUFF(targetI1, srcI1); //Input-Img (mandatory)
   if(fptr->nParents == 2)
   {
      PROG_IO_BUFF(targetI2, srcI2); //Input-Ref (optional)
   }
   PROG_IO_BUFF(targetO , srcO ); //Output (mandatory)

 //Chroma Denoise specifics:
   SET_REG_WORD(paramSet + 0x00, param->frmDim); //SIPP_CHROMA_FRM_DIM_ADR
   SET_REG_WORD(paramSet + 0x04, param->cfg   ); //SIPP_CHROMA_CFG_ADR
   SET_REG_WORD(paramSet + 0x08, param->thr[0]); //SIPP_CHROMA_THRESH_ADR
   SET_REG_WORD(paramSet + 0x0C, param->thr[1]); //SIPP_CHROMA_THRESH2_ADR
}
#endif