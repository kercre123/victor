#include <sipp.h>

#if (defined(SIPP_PC) || defined(MYRIAD2))

//#######################################################################################
void sippInitRaw(SippFilter *fptr)
{
   RawParam    *param    = (RawParam*)fptr->params;
   
   sippIbufSetup(fptr, 0); //Compact Input buffer
   sippObufSetup(fptr);    //Compact Output buffer

 //One time constants updates
   param->frmDim = (fptr->parents[0]->outputH << SIPP_IMGDIM_SIZE)|fptr->parents[0]->outputW;
}

//#######################################################################################
void sippLoadRaw(SippFilter *fptr, UInt32 ctx)
{
   SippHwBuf *targetI;
   SippHwBuf *targetO;
   UInt32     paramSet;
   SippHwBuf *srcI  = fptr->iBuf[0];
   SippHwBuf *srcO  = fptr->oBuf;
   RawParam  *param = (RawParam*)fptr->params;

 //Figure out which set we need to use: Main(ctx=0) or Shadow(ctx=1)
   if(0 == ctx)
   {
       targetI = (SippHwBuf*)I_BASE(SIPP_RAW_ID);
       targetO = (SippHwBuf*)O_BASE(SIPP_RAW_ID);
       paramSet = SIPP_RAW_BASE_ADR;
   }
   else{
       targetI = (SippHwBuf*)I_SHADOW_BASE(SIPP_RAW_ID);
       targetO = (SippHwBuf*)O_SHADOW_BASE(SIPP_RAW_ID);
       paramSet = SIPP_RAW_SHADOW_ADR;
   }

 //Program I & O buffers
   PROG_IO_BUFF(targetI, srcI);
   PROG_IO_BUFF(targetO, srcO);

 //RAW specific params
   SET_REG_WORD(paramSet + 0x00,  param->frmDim);    //SIPP_RAW_FRM_DIM_ADR
                                                     //SIPP_STATS_FRM_DIM_ADR
   SET_REG_WORD(paramSet + 0x08,  param->cfg);       //SIPP_RAW_CFG_ADR
   SET_REG_WORD(paramSet + 0x0C,  param->grgbPlat);  //SIPP_GRGB_PLATO_ADR
   SET_REG_WORD(paramSet + 0x10,  param->grgbDecay); //SIPP_GRGB_SLOPE_ADR
   SET_REG_WORD(paramSet + 0x14,  param->badPixCfg); //SIPP_BAD_PIXEL_CFG_ADR
      
   //Note: gains change on each line even within same context
   //      so no point in updating here
   //   SIPP_GAIN_SATURATE_0_ADR
   //   SIPP_GAIN_SATURATE_1_ADR
   //   SIPP_GAIN_SATURATE_2_ADR
   //   SIPP_GAIN_SATURATE_3_ADR

   //TBD: statistics if enabled...
   //  SIPP_STATS_FRM_DIM_ADR 
   //  SIPP_STATS_PATCH_CFG_ADR  
   //  SIPP_STATS_PATCH_START_ADR
   //  SIPP_STATS_PATCH_SKIP_ADR 
   //  SIPP_RAW_STATS_PLANES_ADR 
}

//#######################################################################################
void sippSetupRawLine(SippFilter *fptr, UInt32 ctx)
{
    UInt32     *gainSat = ((RawParam*)fptr->params)->gainSat[fptr->exeNo & 1];
    UInt32      paramSet = (ctx==0) ? SIPP_RAW_BASE_ADR : SIPP_RAW_SHADOW_ADR;

  //Gains must be rewritten for each new line:
    SET_REG_WORD(paramSet + 0x18, gainSat[0]);  //SIPP_GAIN_SATURATE_0_ADR
    SET_REG_WORD(paramSet + 0x1c, gainSat[1]);  //SIPP_GAIN_SATURATE_1_ADR
    SET_REG_WORD(paramSet + 0x20, gainSat[2]);  //SIPP_GAIN_SATURATE_2_ADR
    SET_REG_WORD(paramSet + 0x24, gainSat[3]);  //SIPP_GAIN_SATURATE_3_ADR
}

#endif
