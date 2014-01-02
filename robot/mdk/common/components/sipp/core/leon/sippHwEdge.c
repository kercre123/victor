#include <sipp.h>

#if (defined(SIPP_PC) || defined(MYRIAD2))

//#######################################################################################
void sippInitEdgeOp(SippFilter *fptr)
{
   EdgeParam *param = (EdgeParam*)fptr->params;
   
   sippIbufSetup(fptr, 0); //Compact Input buffer
   sippObufSetup(fptr);    //Compact Output buffer

 //One time constants updates
   param->frmDim = (fptr->parents[0]->outputH << SIPP_IMGDIM_SIZE)|
                    fptr->parents[0]->outputW;
}

//#######################################################################################
void sippLoadEdgeOp(SippFilter *fptr, UInt32 ctx)
{
   SippHwBuf *targetI;
   SippHwBuf *targetO;
   UInt32     paramSet;
   SippHwBuf *srcI  = fptr->iBuf[0];
   SippHwBuf *srcO  = fptr->oBuf;
   EdgeParam *param = (EdgeParam*)fptr->params;

 //Figure out which set we need to use: Main(ctx=0) or Shadow(ctx=1)
   if(0 == ctx)
   {
       targetI = (SippHwBuf*)I_BASE(SIPP_EDGE_OP_ID);
       targetO = (SippHwBuf*)O_BASE(SIPP_EDGE_OP_ID);
       paramSet = SIPP_EDGE_OP_BASE_ADR;
   }
   else{
       targetI = (SippHwBuf*)I_SHADOW_BASE(SIPP_EDGE_OP_ID);
       targetO = (SippHwBuf*)O_SHADOW_BASE(SIPP_EDGE_OP_ID);
       paramSet = SIPP_EDGE_OP_SHADOW_ADR;
   }

 //Program I & O buffers
   PROG_IO_BUFF(targetI, srcI);
   PROG_IO_BUFF(targetO, srcO);

 //Edge Operator specific params
   SET_REG_WORD(paramSet + 0x00, param->frmDim);   //SIPP_EDGE_OP_FRM_DIM_ADR  
   SET_REG_WORD(paramSet + 0x04, param->cfg   );   //SIPP_EDGE_OP_CFG_ADR      
   SET_REG_WORD(paramSet + 0x08, param->xCoeff);   //SIPP_EDGE_OP_XCOEFF_ADR   
   SET_REG_WORD(paramSet + 0x0C, param->yCoeff);   //SIPP_EDGE_OP_YCOEFF_ADR   
}

#endif