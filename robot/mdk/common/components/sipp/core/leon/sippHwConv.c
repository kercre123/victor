#include <sipp.h>

#if (defined(SIPP_PC) || defined(MYRIAD2))

//#######################################################################################
void sippInitConv(SippFilter *fptr)
{
   ConvParam    *param    = (ConvParam*)fptr->params;
   
   sippIbufSetup(fptr, 0); //Compact Input buffer
   sippObufSetup(fptr);    //Compact Output buffer

 //One time constants updates
   param->frmDim = (fptr->parents[0]->outputH << SIPP_IMGDIM_SIZE)|fptr->parents[0]->outputW;
}

//#######################################################################################
//Filter load is optimized for context-switch: it just copies packed data to destination
//reg sets. Previous approach derived params at load-time; such approach was appropriate
//for single-context case, where the Load was a SETUP-element, not a RUNTIME one
//#######################################################################################
void sippLoadConv(SippFilter *fptr, UInt32 ctx)
{
   SippHwBuf *targetI;
   SippHwBuf *targetO;
   UInt32     paramSet;
   SippHwBuf *srcI  = fptr->iBuf[0];
   SippHwBuf *srcO  = fptr->oBuf;
   ConvParam *param = (ConvParam*)fptr->params;

 //Figure out which set we need to use: Main(ctx=0) or Shadow(ctx=1)
   if(0 == ctx)
   {
       targetI = (SippHwBuf*)I_BASE(SIPP_CONV_ID);
       targetO = (SippHwBuf*)O_BASE(SIPP_CONV_ID);
       paramSet = SIPP_CONV_BASE_ADR;
   }
   else{
       targetI = (SippHwBuf*)I_SHADOW_BASE(SIPP_CONV_ID);
       targetO = (SippHwBuf*)O_SHADOW_BASE(SIPP_CONV_ID);
       paramSet = SIPP_CONV_SHADOW_ADR;
   }

 //Program I & O buffers
   PROG_IO_BUFF(targetI, srcI);
   PROG_IO_BUFF(targetO, srcO);

 //Convolution specific params
   SET_REG_WORD(paramSet + 0x00,     param->frmDim);     //SIPP_CONV_FRM_DIM_ADR
   SET_REG_WORD(paramSet + 0x04,     param->cfg);        //SIPP_CONV_CFG_ADR

 //The 5x5 coefs matrix 
 //(need to set all params, if the matrix is 3x3, it will be surrounded by zeros)
   SET_REG_WORD(paramSet + 0x10,     param->kernel[ 0]); //SIPP_CONV_COEFF_01_00_ADR
   SET_REG_WORD(paramSet + 0x14,     param->kernel[ 1]); //SIPP_CONV_COEFF_03_02_ADR
   SET_REG_WORD(paramSet + 0x18,     param->kernel[ 2]); //SIPP_CONV_COEFF_04_ADR   
   SET_REG_WORD(paramSet + 0x1c,     param->kernel[ 3]); //SIPP_CONV_COEFF_11_10_ADR
   SET_REG_WORD(paramSet + 0x20,     param->kernel[ 4]); //SIPP_CONV_COEFF_13_12_ADR
   SET_REG_WORD(paramSet + 0x24,     param->kernel[ 5]); //SIPP_CONV_COEFF_14_ADR   
   SET_REG_WORD(paramSet + 0x28,     param->kernel[ 6]); //SIPP_CONV_COEFF_21_20_ADR
   SET_REG_WORD(paramSet + 0x2c,     param->kernel[ 7]); //SIPP_CONV_COEFF_23_22_ADR
   SET_REG_WORD(paramSet + 0x30,     param->kernel[ 8]); //SIPP_CONV_COEFF_24_ADR   
   SET_REG_WORD(paramSet + 0x34,     param->kernel[ 9]); //SIPP_CONV_COEFF_31_30_ADR
   SET_REG_WORD(paramSet + 0x38,     param->kernel[10]); //SIPP_CONV_COEFF_33_32_ADR
   SET_REG_WORD(paramSet + 0x3c,     param->kernel[11]); //SIPP_CONV_COEFF_34_ADR   
   SET_REG_WORD(paramSet + 0x40,     param->kernel[12]); //SIPP_CONV_COEFF_41_40_ADR
   SET_REG_WORD(paramSet + 0x44,     param->kernel[13]); //SIPP_CONV_COEFF_43_42_ADR
   SET_REG_WORD(paramSet + 0x48,     param->kernel[14]); //SIPP_CONV_COEFF_44_ADR   
}
#endif
