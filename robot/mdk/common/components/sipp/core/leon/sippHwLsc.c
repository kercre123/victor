#include <sipp.h>

#if (defined(SIPP_PC) || defined(MYRIAD2))

//#####################################################################################
//LSC has a single PARENT-FILTER, but 2 input buffers (one is the GAIN MAP)
// nParents = 1
void sippInitLsc(SippFilter *fptr)
{
   LscParam    *param    = (LscParam*)fptr->params;
   UInt32       gmPlanes;

   if(param->dataFormat == BAYER)
   {
       gmPlanes = 1;
       param->fraction = (((UInt32)(65536.0f*(param->gmHeight/2-1) / (0.5f * fptr->outputH - 1))) << 16)
                        |(((UInt32)(65536.0f*(param->gmWidth /2-1) / (0.5f * fptr->outputW - 1)))      );
   }
   else{
       gmPlanes = fptr->nPlanes;
       param->fraction = (((UInt32)(65536.0f*(param->gmHeight-1)  / (       fptr->outputH - 1))) << 16)
                        |(((UInt32)(65536.0f*(param->gmWidth -1)  / (       fptr->outputW - 1)))      );
   }

 //Compact Input buffer
   sippIbufSetup(fptr, 0); 

 //The second input buffer doesn't map on filter definition, 
 //so using some custom setup:
   fptr->iBuf[1]       = (SippHwBuf*)sippMemAlloc(mempool_sipp, sizeof(SippHwBuf));
   fptr->iBuf[1]->base = (UInt32)param->gmBase;
   fptr->iBuf[1]->ls   = param->gmWidth * 2;                 //16bit fixed point format 8.8
   fptr->iBuf[1]->ps   = param->gmWidth * param->gmHeight * 2;
   fptr->iBuf[1]->cfg  = param->gmHeight                     //number of lines
                        | ((gmPlanes-1) << SIPP_NP_OFFSET)   //number of planes
                        | (2            << SIPP_FO_OFFSET);  //16bit fixed point format 8.8

 //Compact Output buffer
   sippObufSetup(fptr);    

 //One time constants updates
   param->frmDim = (fptr->parents[0]->outputH << SIPP_IMGDIM_SIZE)| fptr->parents[0]->outputW;
   param->gmDim  = (param->gmHeight           << SIPP_IMGDIM_SIZE)| param->gmWidth;
   param->cfg    = param->dataFormat | ((param->dataWidth - 1)<<4);
}

//#####################################################################################
void sippLoadLsc(SippFilter *fptr, UInt32 ctx)
{
   SippHwBuf *targetI1, *targetI2;
   SippHwBuf *targetO;
   UInt32     paramSet;
   SippHwBuf *srcI1 = fptr->iBuf[0]; //image
   SippHwBuf *srcI2 = fptr->iBuf[1]; //gain map
   SippHwBuf *srcO  = fptr->oBuf;
   LscParam  *param = (LscParam*)fptr->params;

 //Figure out which set we need to use: Main(ctx=0) or Shadow(ctx=1)
   if(0 == ctx)
   {
       targetI1 = (SippHwBuf*)I_BASE(SIPP_LSC_ID);
       targetI2 = (SippHwBuf*)I_BASE(SIPP_LSC_GM_ID);
       targetO  = (SippHwBuf*)O_BASE(SIPP_LSC_ID);
       paramSet = SIPP_LSC_BASE_ADR;
   }
   else{
       targetI1 = (SippHwBuf*)I_SHADOW_BASE(SIPP_LSC_ID);
       targetI2 = (SippHwBuf*)I_SHADOW_BASE(SIPP_LSC_GM_ID);
       targetO  = (SippHwBuf*)O_SHADOW_BASE(SIPP_LSC_ID);
       paramSet = SIPP_LSC_SHADOW_ADR;
   }

 //Program I & O buffers
   PROG_IO_BUFF(targetI1, srcI1); //Input-Img
   PROG_IO_BUFF(targetI2, srcI2); //Input-Gain Map
   PROG_IO_BUFF(targetO , srcO ); //Output

 //Custom regs
   SET_REG_WORD(paramSet + 0x00, param->frmDim);
   SET_REG_WORD(paramSet + 0x04, param->fraction);
   SET_REG_WORD(paramSet + 0x08, param->gmDim);
   SET_REG_WORD(paramSet + 0x0C, param->cfg);
}

#endif