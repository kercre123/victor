#include <sipp.h>

#if (defined(SIPP_PC) || defined(MYRIAD2))

//#######################################################################################
void sippInitSharpen(SippFilter *fptr)
{
   UsmParam *param = (UsmParam*)fptr->params;
   
   sippIbufSetup(fptr, 0); //Compact Input buffer
   sippObufSetup(fptr);    //Compact Output buffer

 //One time constants updates
   param->frmDim = (fptr->parents[0]->outputH << SIPP_IMGDIM_SIZE)|fptr->parents[0]->outputW;
}

//#######################################################################################
void sippLoadSharpen(SippFilter *fptr, UInt32 ctx)
{
   SippHwBuf *targetI, *targetO;
   UInt32     paramSet;
   SippHwBuf *srcI  = fptr->iBuf[0];
   SippHwBuf *srcO  = fptr->oBuf;
   UsmParam  *param = (UsmParam*)fptr->params;
   UInt32     kerSz = fptr->nLinesUsed[0];

 //Figure out which set we need to use: Main(ctx=0) or Shadow(ctx=1)
   if(0 == ctx)
   {
       targetI = (SippHwBuf*)I_BASE(SIPP_SHARPEN_ID);
       targetO = (SippHwBuf*)O_BASE(SIPP_SHARPEN_ID);
       paramSet = SIPP_SHARPEN_BASE_ADR;
   }
   else{
       targetI = (SippHwBuf*)I_SHADOW_BASE(SIPP_SHARPEN_ID);
       targetO = (SippHwBuf*)O_SHADOW_BASE(SIPP_SHARPEN_ID);
       paramSet = SIPP_SHARPEN_SHADOW_ADR;
   }

 //Program I & O buffers
   PROG_IO_BUFF(targetI, srcI);
   PROG_IO_BUFF(targetO, srcO);

 //Unsharp specific:
   SET_REG_WORD(paramSet + 0x00, param->frmDim);     //SIPP_SHARPEN_FRM_DIM_ADR     
   SET_REG_WORD(paramSet + 0x04, param->cfg   );     //SIPP_SHARPEN_CFG_ADR         
   SET_REG_WORD(paramSet + 0x08, param->strenAlpha); //SIPP_SHARPEN_STREN_ADR       
   SET_REG_WORD(paramSet + 0x0c, param->clip);       //SIPP_SHARPEN_CLIP_ADR        
   SET_REG_WORD(paramSet + 0x10, param->limit);      //SIPP_SHARPEN_LIMIT_ADR       
   SET_REG_WORD(paramSet + 0x14, param->rgnStop01);  //SIPP_SHARPEN_RANGETOP_1_0_ADR
   SET_REG_WORD(paramSet + 0x18, param->rgnStop23);  //SIPP_SHARPEN_RANGETOP_3_2_ADR

   switch(kerSz)
   {
    case 7: SET_REG_WORD(paramSet + 0x1C, param->coef01); //SIPP_SHARPEN_GAUSIAN_1_0_ADR
            SET_REG_WORD(paramSet + 0x20, param->coef23); //SIPP_SHARPEN_GAUSIAN_3_2_ADR
            break;

    default://TBD... unimplemented for now...
            sippError(100);
            break;
   }
}

#endif