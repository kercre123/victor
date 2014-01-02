/*****************************************************************************
   @file   <TxHdcp.c>
   @author Max.Kao@ite.com.tw
   @date   2011/11/18
   @fileversion: ITE_HDMI1.4_AVR_SAMPLECODE_2.04
 *****************************************************************************/


/******************************************************************************
 1: Included types first then Apis from other modules
******************************************************************************/

#include "TxHdcp.h"
#include "HdmiTxIteApiDefines.h"
#include <stdio.h>
#include "HdmiTxItePrivate.h"

/******************************************************************************
 2:  Source File Specific #defines
******************************************************************************/
#define  TX_REAL_HDCP_REPEATER
#define     TX_HDCP_TIMEOUT                     MS_TimeOut  (6000)

#ifdef TX_REAL_HDCP_REPEATER
BYTE      TX_cDownStream;
BYTE      TX_BKSV[5];
BYTE      TX_BCaps;
WORD      TX_BStatus;
BYTE      TX_KSVList[30];
BYTE      SHABuff[64];

BYTE      Vr[20];
BYTE      M0[8];
BYTE      Ri[2];//mingchih add
BYTE      V[20];
ULONG     sha[5];
#endif

extern TxDevPara    TxDev[1];

/******************************************************************************
 3:  Local variables
******************************************************************************/

/******************************************************************************
 4:  Local Functions
******************************************************************************/


/******************************************************************************
 5: Local Functions Implementation
******************************************************************************/
/*=============================================================================
                       Authentication
 =============================================================================*/
void HDMITXITEFULL_CODE_SECTION(HDCP_ClearAuthInterrupt) (void)
{
    HDMITX_WriteI2C_Byte (REG_TX_INT_CLR0,   B_CLR_AUTH_FAIL | B_CLR_AUTH_DONE | B_CLR_KSVLISTCHK);
    HDMITX_WriteI2C_Byte (REG_TX_INT_CLR1,   0);
    HDMITX_WriteI2C_Byte (REG_TX_SYS_STATUS, B_INTACTDONE);
}

/*=============================================================================
   Function: HDCP_EnableEncryption
   Remark: Set regC1 as zero to enable continue authentication.
   Side-Effect: register bank will reset to zero.
  =============================================================================*/
void HDMITXITEFULL_CODE_SECTION(HDCP_Abort) (void)
{
    Switch_HDMITX_Bank(0);
    HDMITX_WriteI2C_Byte(REG_TX_HDCP_DESIRE,0x0);
    HDMITX_OrReg_Byte(REG_TX_SW_RST,B_HDCP_RST);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,B_MASTERHOST);
    AbortDDC();
}

/*=============================================================================
   Function: HDCP_Auth_Fire()
   Remark: write anything to reg21 to enable HDCP authentication by HW
   Side-Effect: N/A
  =============================================================================*/
void HDMITXITEFULL_CODE_SECTION(HDCP_Auth_Fire) (void)
{
    HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,B_MASTERDDC|B_MASTERHDCP);// MASTERHDCP,no need command but fire.
    HDMITX_WriteI2C_Byte(REG_TX_AUTHFIRE,1);
}

/*=============================================================================
   Function: HDCP_StartAnCipher
   Parameter: N/A
   Return: N/A
   Remark: Start the Cipher to free run for random number. When stop,An is
           ready in Reg30.
   Side-Effect: N/A
  =============================================================================*/
void HDMITXITEFULL_CODE_SECTION(HDCP_StartAnCipher) (void)
{
    HDMITX_WriteI2C_Byte(REG_TX_AN_GENERATE,B_START_CIPHER_GEN);
}

/*=============================================================================
   Function: HDCP_StopAnCipher
   Remark: Stop the Cipher,and An is ready in Reg30.
//=============================================================================*/
void HDMITXITEFULL_CODE_SECTION(HDCP_StopAnCipher) (void)
{
    HDMITX_WriteI2C_Byte(REG_TX_AN_GENERATE,B_STOP_CIPHER_GEN);
}

/*=============================================================================
   Function: HDCP_GenerateAn
   Remark: start An ciper random run at first,then stop it. Software can get
           an in reg30~reg38,the write to reg28~2F
  =============================================================================*/
void HDMITXITEFULL_CODE_SECTION(HDCP_GenerateAn) (void)
{
    BYTE Data[8];
    #ifdef _TXHDCP_DEBUG_PRINTF_
    BYTE i;
    #endif // _TXHDCP_DEBUG_PRINTF_

    HDCP_StartAnCipher();
    HDCP_StopAnCipher();

    Switch_HDMITX_Bank(0);

    // new An is ready in reg30
    HDMITX_ReadI2C_ByteN(REG_TX_AN_GEN,Data,8);
    HDMITX_WriteI2C_ByteN(REG_TX_AN,Data,8);
}

/*=============================================================================
   Function: HDCP_GetBCaps
   Parameter: pBCaps - pointer of BYTE to get BCaps.
              pBStatus - pointer of two bytes to get BStatus
   Return: ER_SUCCESS if successfully got BCaps and BStatus.
   Remark: get B status and capability from HDCP reciever via DDC bus.
  =============================================================================*/
SYS_STATUS HDMITXITEFULL_CODE_SECTION(HDCP_GetBCaps) (PBYTE pBCaps, PUSHORT pBStatus)
{
	BYTE ucdata;
	BYTE TimeOut;

	Switch_HDMITX_Bank(0);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,B_MASTERDDC|B_MASTERHOST);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_HEADER,DDC_HDCP_ADDRESS);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_REQOFF,0x40);// BCaps offset
	HDMITX_WriteI2C_Byte(REG_TX_DDC_REQCOUNT,3);
	HDMITX_WriteI2C_Byte(REG_TX_DDC_CMD,CMD_DDC_SEQ_BURSTREAD);

	for(TimeOut=200;TimeOut > 0;TimeOut --)
	{
		ucdata=HDMITX_ReadI2C_Byte(REG_TX_DDC_STATUS);
		if(ucdata & B_DDC_DONE)
		{
#ifdef _PRINT_HDMI_TX_HDCP_
//            //TXHDCP_DEBUG_PRINTF(("HDCP_GetBCaps(): DDC Done.\n"));
#endif
			break;
		}

		if(ucdata & B_DDC_ERROR)
		{
#ifdef _PRINT_HDMI_TX_HDCP_
			//TXHDCP_DEBUG_PRINTF(("HDCP_GetBCaps(): DDC fail by reg16=%X.\n",ucdata));
#endif
			return ER_FAIL;
		}
	}

	if(TimeOut==0)
	{
		return ER_FAIL;
	}

	// HDMITX_ReadI2C_ByteN(REG_TX_BSTAT,(PBYTE)pBStatus,2);
	*pBStatus &=0x0000;
	*pBStatus=(HDMITX_ReadI2C_Byte(REG_TX_BSTAT)&0xff);
	*pBStatus +=((HDMITX_ReadI2C_Byte(REG_TX_BSTAT1)&0xff)<<8);

	*pBCaps=HDMITX_ReadI2C_Byte(REG_TX_BCAP);
	return ER_SUCCESS;

}


/*=============================================================================
   Function: HDCP_GetBKSV
   Parameter: pBKSV - pointer of 5 bytes buffer for getting BKSV
   Return: ER_SUCCESS if successfuly got BKSV from Rx.
   Remark: Get BKSV from HDCP reciever.
  =============================================================================*/
SYS_STATUS HDMITXITEFULL_CODE_SECTION(HDCP_GetBKSV) (BYTE *pBKSV)
{
    BYTE ucdata;
    BYTE TimeOut;

    Switch_HDMITX_Bank(0);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,B_MASTERDDC|B_MASTERHOST);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_HEADER,DDC_HDCP_ADDRESS);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_REQOFF,0x00);// BKSV offset
    HDMITX_WriteI2C_Byte(REG_TX_DDC_REQCOUNT,5);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_CMD,CMD_DDC_SEQ_BURSTREAD);

    for(TimeOut=200;TimeOut > 0;TimeOut --)
    {
        ucdata=HDMITX_ReadI2C_Byte(REG_TX_DDC_STATUS);
        if(ucdata & B_DDC_DONE)
        {
#ifdef _PRINT_HDMI_TX_HDCP_
            // //TXHDCP_DEBUG_PRINTF(("HDCP_GetBCaps(): DDC Done.\n"));
#endif
            break;
        }

        if(ucdata & B_DDC_ERROR)
        {
#ifdef _PRINT_HDMI_TX_HDCP_
              // //TXHDCP_DEBUG_PRINTF(("HDCP_GetBCaps(): DDC No ack,maybe cable did not connected. Fail.\n"));
#endif
            return ER_FAIL;
        }
    }

    if(TimeOut==0)
    {
        return ER_FAIL;
    }

    HDMITX_ReadI2C_ByteN(REG_TX_BKSV,(PBYTE)pBKSV,5);
    return ER_SUCCESS;
}

void HDMITXITEFULL_CODE_SECTION(GenerateDDCSCLK) (void)
{
    HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,B_MASTERDDC|B_MASTERHOST);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_CMD,CMD_GEN_SCLCLK);
}

SYS_STATUS HDMITXITEFULL_CODE_SECTION(HDCP_GetKSVList) (BYTE *pKSVList,BYTE cDownStream)
{
    BYTE TimeOut=100;
    BYTE ucdata;

    if(cDownStream==0 || pKSVList==NULL)
    {
        return ER_FAIL;
    }

    HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,B_MASTERHOST);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_HEADER,0x74);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_REQOFF,0x43);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_REQCOUNT,cDownStream * 5);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_CMD,CMD_DDC_SEQ_BURSTREAD);


    for(TimeOut=200;TimeOut > 0;TimeOut --)
    {

        ucdata=HDMITX_ReadI2C_Byte(REG_TX_DDC_STATUS);

        if(ucdata & B_DDC_DONE)
        {
//#ifdef _PRINT_HDMI_TX_HDCP_
//            //TXHDCP_DEBUG_PRINTF(("HDCP_GetKSVList(): DDC Done.\n"));
//#endif
            break;
        }

        if(ucdata & B_DDC_ERROR)
        {
//#ifdef _PRINT_HDMI_TX_HDCP_
//            //TXHDCP_DEBUG_PRINTF((("HDCP_GetKSVList()): DDC Fail by REG_TX_DDC_STATUS=%x.\n",ucdata));
//#endif
            return ER_FAIL;
        }
    }

    if(TimeOut==0)
    {
        return ER_FAIL;
    }
#ifdef _PRINT_HDMI_TX_HDCP_
    //TXHDCP_DEBUG_PRINTF((" \n(3) HDCP_GetKSVList()"));
#endif

    for(TimeOut=0;TimeOut < cDownStream * 5;TimeOut++)
    {
        pKSVList[TimeOut]=HDMITX_ReadI2C_Byte(REG_TX_DDC_READFIFO);
#ifdef _PRINT_HDMI_TX_HDCP_
        if(TimeOut%5==0)
            //TXHDCP_DEBUG_PRINTF(("\n"));

        //TXHDCP_DEBUG_PRINTF(("0x%02X ",(int)pKSVList[TimeOut]));
#endif
    }
#ifdef _PRINT_HDMI_TX_HDCP_
     //TXHDCP_DEBUG_PRINTF(("\n"));
#endif
    return ER_SUCCESS;
}

SYS_STATUS HDMITXITEFULL_CODE_SECTION(HDCP_GetVr) (BYTE *pVr)
{
    BYTE TimeOut;
    BYTE ucdata;

    if(pVr==NULL)
    {
        return NULL;
    }

    HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,B_MASTERHOST);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_HEADER,0x74);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_REQOFF,0x20);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_REQCOUNT,20);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_CMD,CMD_DDC_SEQ_BURSTREAD);


    for(TimeOut=200;TimeOut > 0;TimeOut --)
    {
        ucdata=HDMITX_ReadI2C_Byte(REG_TX_DDC_STATUS);
        if(ucdata & B_DDC_DONE)
        {
           // //TXHDCP_DEBUG_PRINTF(("HDCP_GetVr(): DDC Done.\n"));
           break;
        }

        if(ucdata & B_DDC_ERROR)
        {

            //TXHDCP_DEBUG_PRINTF(("HDCP_GetVr(): DDC fail by REG_TX_DDC_STATUS=%x.\n",ucdata));
            return ER_FAIL;
        }
    }

    if(TimeOut==0)
    {
#ifdef _PRINT_HDMI_TX_HDCP_
        //TXHDCP_DEBUG_PRINTF(("HDCP_GetVr(): DDC fail by timeout.\n",ucdata));
#endif
        return ER_FAIL;
    }

    Switch_HDMITX_Bank(0);

    //TXHDCP_DEBUG_PRINTF(("HDCP_GetVr() V'="));
    for(TimeOut=0;TimeOut < 5;TimeOut++)
    {
        HDMITX_WriteI2C_Byte(REG_TX_SHA_SEL,TimeOut);
        pVr[TimeOut*4+0]=(ULONG)HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE1);
        pVr[TimeOut*4+1]=(ULONG)HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE2);
        pVr[TimeOut*4+2]=(ULONG)HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE3);
        pVr[TimeOut*4+3]=(ULONG)HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE4);

        //TXHDCP_DEBUG_PRINTF((" %02X %02X %02X %02X\n", (int)pVr[TimeOut*4],(int)pVr[TimeOut*4+1],(int)pVr[TimeOut*4+2],(int)pVr[TimeOut*4+3]));

    }

    return ER_SUCCESS;
}

void HDMITXITEFULL_CODE_SECTION(HDCP_ResumeRepeaterAuthenticate) (void)
{
    HDMITX_WriteI2C_Byte(REG_TX_LISTCTRL,B_LISTDONE);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,B_MASTERDDC|B_MASTERHDCP);// MASTERHDCP,no need command but fire.
}

void HDMITXITEFULL_CODE_SECTION(HDCP_CancelRepeaterAuthenticate) (void)
{

#ifdef _PRINT_HDMI_TX_HDCP_
    //TXHDCP_DEBUG_PRINTF(("HDCP_CancelRepeaterAuthenticate\n"));
#endif

    HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,B_MASTERDDC|B_MASTERHOST);
    AbortDDC();
    HDMITX_WriteI2C_Byte(REG_TX_LISTCTRL,B_LISTFAIL|B_LISTDONE);
    HDCP_ClearAuthInterrupt();
}

static void HDMITXITEFULL_CODE_SECTION(HDCP_Reset) (void)
{
    BYTE uc;
    uc=HDMITX_ReadI2C_Byte(REG_TX_SW_RST) | B_HDCP_RST;
    HDMITX_WriteI2C_Byte(REG_TX_SW_RST,uc);
    HDMITX_WriteI2C_Byte(REG_TX_HDCP_DESIRE,0);
    HDMITX_WriteI2C_Byte(REG_TX_LISTCTRL,0);
    HDMITX_WriteI2C_Byte(REG_TX_DDC_MASTER_CTRL,B_MASTERHOST);
    ClearDDCFIFO();
    AbortDDC();
}

/*=============================================================================
   Function:HDCP_Authenticate
   Return: ER_SUCCESS if Authenticated without error.
   Remark: do Authentication with Rx
   Side-Effect:
    1. bTxAuthenticated global variable will be TRUE when authenticated.
    2. Auth_done interrupt and AUTH_FAIL interrupt will be enabled.
  =============================================================================*/


BYTE HDMITXITEFULL_CODE_SECTION(countbit)(BYTE b)
{
    BYTE i,count;
    for(i=0,count=0;i < 8;i++)
    {
        if(b & (1<<i))
        {
            count++;
        }
    }
    return count;
}


SYS_STATUS HDMITXITEFULL_CODE_SECTION(HDCP_GetM0) (BYTE *pM0)
{

    if(!pM0)
    {
        return ER_FAIL;
    }
    Switch_HDMITX_Bank(0);

    HDMITX_WriteI2C_Byte(REG_TX_SHA_SEL,5);// read m0[31:0] from reg51~reg54
    pM0[0]=HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE1);
    pM0[1]=HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE2);
    pM0[2]=HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE3);
    pM0[3]=HDMITX_ReadI2C_Byte(REG_TX_SHA_RD_BYTE4);
    HDMITX_WriteI2C_Byte(REG_TX_SHA_SEL,0);// read m0[39:32] from reg55
    pM0[4]=HDMITX_ReadI2C_Byte(REG_TX_AKSV_RD_BYTE5);
    HDMITX_WriteI2C_Byte(REG_TX_SHA_SEL,1);// read m0[47:40] from reg55
    pM0[5]=HDMITX_ReadI2C_Byte(REG_TX_AKSV_RD_BYTE5);
    HDMITX_WriteI2C_Byte(REG_TX_SHA_SEL,2);// read m0[55:48] from reg55
    pM0[6]=HDMITX_ReadI2C_Byte(REG_TX_AKSV_RD_BYTE5);
    HDMITX_WriteI2C_Byte(REG_TX_SHA_SEL,3);// read m0[63:56] from reg55
    pM0[7]=HDMITX_ReadI2C_Byte(REG_TX_AKSV_RD_BYTE5);

    //TXHDCP_DEBUG_PRINTF(("HDCP_GetM0() : M[]=0x%02X, 0x%02X, 0x%02X, 0x%02X,",(int)pM0[0],(int)pM0[1],(int)pM0[2],(int)pM0[3]));
    //TXHDCP_DEBUG_PRINTF(("0x%02X, 0x%02X, 0x%02X, 0x%02X\n",(int)pM0[4],(int)pM0[5],(int)pM0[6],(int)pM0[7]));
    return ER_SUCCESS;
}


SYS_STATUS HDMITXITEFULL_CODE_SECTION(HDCP_CheckSHA) (BYTE pM0[],WORD BStatus,BYTE pKSVList[],BYTE cDownStream,BYTE cVr[])
{
    WORD i,n;
    pM0=pM0;
    pKSVList=pKSVList;
    for(i=0;i < cDownStream*5;i++)
    {
        //SHABuff[i]=pKSVList[i];
        SHABuff[i]=TX_KSVList[i];
    }
    SHABuff[i++]=BStatus & 0xFF;
    SHABuff[i++]=(BStatus>>8) & 0xFF;
    //TXHDCP_DEBUG_PRINTF(("\n M[]=")) ;
    for(n=0;n < 8;n++,i++)
    {
        //TXHDCP_DEBUG_PRINTF(("0x%02X ",(int)M0[n]));
        SHABuff[i]=M0[n];
    }
    //TXHDCP_DEBUG_PRINTF(("\n"));
    n=i;

    for(; i < 64;i++)
    {
        SHABuff[i]=0;
    }

    SHA_Simple(SHABuff,n,V);
    #ifdef _PRINT_HDMI_TX_HDCP_
    //TXHDCP_DEBUG_PRINTF(("V[]  =")) ;
    for(i=0;i < 20;i++)
    {
        //TXHDCP_DEBUG_PRINTF((" %02X",(int)V[i])) ;
    }
    //TXHDCP_DEBUG_PRINTF(("\nV'[] =")) ;
    for(i=0;i < 20;i++)
    {
        //TXHDCP_DEBUG_PRINTF((" %02X",(int)cVr[i])) ;

    }
    //TXHDCP_DEBUG_PRINTF(("\n")) ;
    #endif

    for(i=0;i < 20;i++)
    {
        if(V[i] !=cVr[i])
        {
            return ER_FAIL;
        }
    }
    return ER_SUCCESS;
}

void HDMITXITEFULL_CODE_SECTION(Tx_SwitchHDCPState) (Tx_HDCP_State_Type state)
{
   //TXHDCP_DEBUG_PRINTF(("Tx_SwitchHDCPState %d -> %d\n",(int) TxDev[TX0DEV].TxHDCPState , (int)state)) ;
   if( TxDev[TX0DEV].TxVState !=TxVStateVideoOn )
    {
        state=TxHDCP_Off;
    }
    if( TxDev[TX0DEV].TxHDCPState==state )
    {
        return;
    }
    TxDev[TX0DEV].TxHDCPStateCounter=0;

    HdmiDriverInfo.TxHdcpState = state;

    switch(state)
    {
    case TxHDCP_Off:
        if( TxDev[TX0DEV].bTxHDCP )
        {
            if( TxDev[TX0DEV].TxHDCPState==TxHDCP_AuthFail || TxDev[TX0DEV].TxHDCPState==TxHDCP_RepeaterFail )
            {

             TxDev[TX0DEV].TxHDCPStateCounter=MS_TimeOut(500);
            }
        TxDev[TX0DEV].bTxAuthenticated=FALSE;

        SetTxAVMute();
        }
        else
        {
            TxDev[TX0DEV].bTxAuthenticated=TRUE;
        }
        HDCP_Abort();
        break;
    case TxHDCP_AuthRestart :
        TxDev[TX0DEV].TxHDCPStateCounter=MS_TimeOut(500);
        break;
    case TxHDCP_AuthStart :
        TxDev[TX0DEV].TxHDCPStateCounter=MS_TimeOut(1500);;
        break;
    case TxHDCP_Receiver :
        TxDev[TX0DEV].TxHDCPStateCounter=TX_HDCP_TIMEOUT; //_20080918_ for Allion
        break;
    case TxHDCP_Repeater:
        TxDev[TX0DEV].TxHDCPStateCounter=TX_HDCP_TIMEOUT; //_20080918_ for Allion
        break;
    case TxHDCP_CheckFIFORDY :
        TxDev[TX0DEV].TxHDCPStateCounter=TX_HDCP_TIMEOUT;
        HDMITX_OrReg_Byte(REG_TX_INT_MASK2,B_KSVLISTCHK_MASK);
        break;
    case TxHDCP_VerifyRevocationList:
        break;
    case TxHDCP_CheckSHA:
        break;
    case TxHDCP_AuthFail:
        HDCP_Abort();                 //_20080918_ for Allion
        break;

    case TxHDCP_RepeaterFail:
        HDCP_CancelRepeaterAuthenticate();
        break;

    case TxHDCP_RepeaterSuccess:
        HDCP_ResumeRepeaterAuthenticate();
    case TxHDCP_Authenticated:

#ifdef DEBUG_TIMER
        TxAuthDoneTickCount=ucTickCount;
#endif
        //TXHDCP_DEBUG_PRINTF(("TxHDCP_Authenticated.\n"));  //_20080918_ for Allion
        ClrTxAVMute();                         //_20080918_ for Allion

        HDMITX_OrReg_Byte(REG_TX_INT_MASK2,B_KSVLISTCHK_MASK|B_T_AUTH_DONE_MASK);
        TxDev[TX0DEV].bTxAuthenticated=TRUE;
        state=TxHDCP_Authenticated;
        break;
    }
    //TXHDCP_DEBUG_PRINTF(("TxDev[TX0DEV].TxHDCPState=%d->%d\n",(int)TxDev[TX0DEV].TxHDCPState,(int)state));

    TxDev[TX0DEV].TxHDCPState=state;
}

void HDMITXITEFULL_CODE_SECTION(TxHDCP_Handler) (void)
{
	//WORD FailBStatus;
	BYTE  ucdata;
	BYTE  revoked=FALSE;
	BYTE     uc;
	BYTE  i;
	eNBRESULT    nbFlag=nbPENDING;

	switch(TxDev[TX0DEV].TxHDCPState)
	{
	case TxHDCP_Off:
		if( TxDev[TX0DEV].TxVState==TxVStateVideoOn )
		{
              #ifdef _DELAY_REAUTHENTICATE_
			      if(TxDev[TX0DEV].TxHDCPStateCounter==0)
              #endif

              #ifdef _DELAY_REAUTHENTICATE_
				  else
				  {
					  //TxDev[TX0DEV].TxHDCPStateCounter --;//mingchih remove
				  }
              #endif
		}
		break;

	case TxHDCP_AuthRestart:
		if( TxDev[TX0DEV].TxHDCPStateCounter > 0 )
		{
			//TxDev[TX0DEV].TxHDCPStateCounter --;//mingchih remove
			break;
		}
		else
			Tx_SwitchHDCPState(TxHDCP_AuthStart);

	 case TxHDCP_AuthStart:
     //  start to Auth check
	 {
		TX_cDownStream=0;
		Switch_HDMITX_Bank(0);// switch bank action should start on direct register writting of each function.
		//_20080917_ for pass 3a-1 get clear frame=>
		SetTxAVMute();

		TxDev[TX0DEV].bTxAuthenticated=FALSE;
		// Authenticate should be called after AFE setup up.
		HDCP_Reset();
        #ifdef _PRINT_HDMI_TX_HDCP_
		//TXHDCP_DEBUG_PRINTF((" TxHDCP_AuthStart %d\n",TxDev[TX0DEV].TxHDCPStateCounter));
		/*
            uc = HDMITX_ReadI2C_Byte(0x04) ;//TXHDCP_DEBUG_PRINTF(("reg04 = %02X ",(int)uc)) ;
            uc = HDMITX_ReadI2C_Byte(0x0E) ;//TXHDCP_DEBUG_PRINTF(("reg0E = %02X ",(int)uc)) ;
            uc = HDMITX_ReadI2C_Byte(0x10) ;//TXHDCP_DEBUG_PRINTF(("reg10 = %02X ",(int)uc)) ;
            uc = HDMITX_ReadI2C_Byte(0x20) ;//TXHDCP_DEBUG_PRINTF(("reg20 = %02X ",(int)uc)) ;
            uc = HDMITX_ReadI2C_Byte(0x22) ;//TXHDCP_DEBUG_PRINTF(("reg22 = %02X ",(int)uc)) ;
            uc = HDMITX_ReadI2C_Byte(0x46) ;//TXHDCP_DEBUG_PRINTF(("reg46 = %02X\n",(int)uc)) ;*/
         #endif

		if(HDCP_GetBCaps(&TX_BCaps,&TX_BStatus) !=ER_SUCCESS)
		{
			//TXHDCP_DEBUG_PRINTF(("HDCP_GetBCaps fail.\n"));
			Tx_SwitchHDCPState(TxHDCP_AuthRestart);
			break;

			nbFlag=nbERROR;
			break;
		}

		if( TxDev[TX0DEV].TxHDCPStateCounter >  0 )
		{
			if( TxDev[TX0DEV].bTxHDMIMode )
			{
				if( (TX_BStatus & (1<<12))==0 )
				{
					//TXHDCP_DEBUG_PRINTF(("TX_BCaps=%X,TX_BStatus=%04X,not a HDMI mode hdcp status.\n",(int)TX_BCaps,TX_BStatus));
					return;
				}
			}
			else
			{
				if( (TX_BStatus & (1<<12)) !=0 )
				{
					//TXHDCP_DEBUG_PRINTF(("TX_BCaps=%X,TX_BStatus=%x,a HDMI mode hdcp status.\n",(int)TX_BCaps,TX_BStatus));
					return;
				}
			}
		}

		HDCP_GetBKSV(TX_BKSV);

        #ifdef DEBUG_TIMER
		    TxAuthStartTickCount=ucTickCount ;
        #endif


        #ifdef _PRINT_HDMI_TX_HDCP_
		       //TXHDCP_DEBUG_PRINTF((" (1) TX_BKSV[0]="));
		    for(i=0;i<5;i++)
			    //TXHDCP_DEBUG_PRINTF(("%X  ",TX_BKSV[i]));
			    //TXHDCP_DEBUG_PRINTF(("\n"));
        #endif
			// InitHDMITX_HDCPROM();

		if( TxDev[TX0DEV].bChipRevision < REV_CAT6613 )
		{
			for( i = 0 ;i < 5 ; i++ )
			{
				if( TX_BKSV[i] == 0xFF )
				{
					Tx_SwitchHDCPState(TxHDCP_Authenticated);
					break ;
				}
			}
		}

		// check bit count
		for( i=0,uc=0; i < 5; i++ )
		{
			uc +=countbit(TX_BKSV[i]);
		}

		if(uc !=20)
		{
			Tx_SwitchHDCPState(TxHDCP_AuthRestart);
			break;
		}

        #ifdef _DSS_SHA_
		    HDCP_VerifyRevocationList(SRM1,BKSV,&revoked);
            #ifdef _PRINT_HDMI_TX_HDCP_
		         ////TXHDCP_DEBUG_PRINTF(("BKSV %X %X %X %X %X is %srevoked\n",BKSV[0],BKSV[1],BKSV[2],BKSV[3],BKSV[4],revoked?"not ":""));
            #endif
        #endif // _DSS_SHA_

		Switch_HDMITX_Bank(0);
		// switch bank action should start on direct register writting of each function.
		// adjust ri/r0 mathod

		HDMITX_AndReg_Byte(REG_TX_SW_RST,~(B_HDCP_RST));
		HDMITX_WriteI2C_Byte(REG_TX_HDCP_DESIRE,B_CPDESIRE);

		//HDMITX_WriteI2C_Byte(REG_TX_INT_CLR0,B_CLR_AUTH_DONE|B_CLR_AUTH_FAIL|B_CLR_KSVLISTCHK);
		//HDMITX_WriteI2C_Byte(REG_TX_INT_CLR1,0);// don't clear other settings.
		//ucdata=HDMITX_ReadI2C_Byte(REG_TX_SYS_STATUS);
		//ucdata=(ucdata & M_CTSINTSTEP) | B_INTACTDONE;
		//HDMITX_WriteI2C_Byte(REG_TX_SYS_STATUS,ucdata);// clear action.

		HDMITX_AndReg_Byte(REG_TX_INT_MASK2,~(B_KSVLISTCHK_MASK|B_T_AUTH_DONE_MASK|B_AUTH_FAIL_MASK));   // enable GetBCaps Interrupt
		HDCP_ClearAuthInterrupt();

		#ifdef _PRINT_HDMI_TX_HDCP_
		    //TXHDCP_DEBUG_PRINTF(("int2=%X DDC_Status=%X\n",HDMITX_ReadI2C_Byte(REG_TX_INT_STAT2),HDMITX_ReadI2C_Byte(REG_TX_DDC_STATUS)));
        #endif

		HDCP_GenerateAn();
		HDMITX_WriteI2C_Byte(REG_TX_LISTCTRL,0);
		// Dump_HDMITXReg();
		HDCP_Auth_Fire();
		// Wait for status;
		// Dump_HDMITXReg();

		if((TX_BCaps & B_CAP_HDMI_REPEATER)==0)
		{
            #ifdef _PRINT_HDMI_TX_HDCP_
			      //TXHDCP_DEBUG_PRINTF(("\n receiver mode"));
             #endif

			// receiver mode
			Tx_SwitchHDCPState(TxHDCP_Receiver);
			TxDev[TX0DEV].bSinkIsRepeater=FALSE;
			// ForceReplySourceAuthentication();
		}
		else
		{
            #ifdef _PRINT_HDMI_TX_HDCP_
			    //TXHDCP_DEBUG_PRINTF(("\n repeater mode"));
            #endif
			// repeater mode
			Tx_SwitchHDCPState(TxHDCP_Repeater);
			TxDev[TX0DEV].bSinkIsRepeater=TRUE;
			//////////////////////////////////////
			// Authenticate Fired
			//////////////////////////////////////
		}
	}
	break;

	case TxHDCP_Receiver:    //receiver mode
		ucdata=HDMITX_ReadI2C_Byte(REG_TX_AUTH_STAT);
		//TXHDCP_DEBUG_PRINTF(("(%d)reg[%x]=%x\n", TxDev[TX0DEV].TxHDCPStateCounter,(int)REG_TX_AUTH_STAT,(int)ucdata));
		if(ucdata & B_T_AUTH_DONE)
		{
			// nbFlag=nbSUCCESS;
			//ForceReplySourceAuthentication();
			Tx_SwitchHDCPState(TxHDCP_Authenticated);
			break;
		}
		break;

	case TxHDCP_Repeater:    // repeater mode
		if( TxDev[TX0DEV].TxHDCPStateCounter==0 )
		{
			//TXHDCP_DEBUG_PRINTF(("TxHDCP_CheckFIFORDY timeout, switch to TxHDCP_CheckFIFORDY\n"));
			Tx_SwitchHDCPState(TxHDCP_CheckFIFORDY);
		}
		break;

	case TxHDCP_CheckFIFORDY:
		///////////////////////////////////////
		//  B_CAP_KSV_FIFO_RDY check .
		///////////////////////////////////////
		if( TxDev[TX0DEV].TxHDCPStateCounter==0 )
		{
			//TXHDCP_DEBUG_PRINTF(("TxHDCP_CheckFIFORDY timeout\n"));
			Tx_SwitchHDCPState(TxHDCP_RepeaterFail);
			break;
		}

		if(HDCP_GetBCaps(&TX_BCaps,&TX_BStatus)==ER_FAIL)
		{
			//TXHDCP_DEBUG_PRINTF(("Get BCaps fail\n"));
			Tx_SwitchHDCPState(TxHDCP_AuthFail);
			break;
		}
		//TXHDCP_DEBUG_PRINTF(("TxHDCP_CheckFIFORDY (%d) TX_BCaps=%02X\n",(int)TxDev[TX0DEV].TxHDCPStateCounter,(int)TX_BCaps));

		if(TX_BCaps & B_CAP_KSV_FIFO_RDY)
		{
#ifdef DEBUG_TIMER
			TxAuthCheckFifoReadyEnd=ucTickCount;
#endif
			ClearDDCFIFO();
			GenerateDDCSCLK();

			TX_cDownStream=(TX_BStatus & M_DOWNSTREAM_COUNT);
			//+++++++++++++++++++++++++++++++++++++
			//TXHDCP_DEBUG_PRINTF(("DS=%X \n",TX_cDownStream));

			if( TX_cDownStream > (MAX_REPEATER_DOWNSTREAM_COUNT-1) || TX_BStatus & (B_MAX_CASCADE_EXCEEDED|B_DOWNSTREAM_OVER))
			{
				//TXHDCP_DEBUG_PRINTF(("Invalid Down stream count,fail\n"));

				// RxAuthSetBStatus(B_DOWNSTREAM_OVER|B_MAX_CASCADE_EXCEEDED);    //for ALLION HDCP 3C-2-06
				//ForceKSVFIFOReady(B_DOWNSTREAM_OVER|B_MAX_CASCADE_EXCEEDED) //this was used by rx

				Tx_SwitchHDCPState(TxHDCP_RepeaterFail);
				break;
			}
			Tx_SwitchHDCPState(TxHDCP_VerifyRevocationList);
		}
		else
			break;

	case TxHDCP_VerifyRevocationList:
		///////////////////////////////////////
		//  HDCP_VerifyRevocationList
		///////////////////////////////////////
		if(HDCP_GetKSVList(TX_KSVList,TX_cDownStream)==ER_FAIL)
		{
			//TXHDCP_DEBUG_PRINTF(("HDCP_GetKSVList\n"));
			Tx_SwitchHDCPState(TxHDCP_RepeaterFail);
			break;
		}

		for(i=0;i < TX_cDownStream;i++)
		{
			revoked=FALSE;
			uc=0;

#ifdef _CHECK_BKSV_BIT_COUNT_
			for(TimeOut=0;TimeOut < 5;TimeOut++)
			{
				// check bit count
				uc +=countbit(KSVList[i*5+TimeOut]);
			}
			if(uc !=20) revoked=TRUE;
#endif

#ifdef SUPPORT_REVOKE_KSV
			if( ! revoked )
			{
				HDCP_VerifyRevocationList(SRM1,&TX_KSVList[i*5],&revoked);
				if(revoked)
				{
					//TXHDCP_DEBUG_PRINTF(("KSVFIFO[%d]=%X %X %X %X %X is revoked\n",i,
					KSVList[i*5],KSVList[i*5+1],KSVList[i*5+2],KSVList[i*5+3],KSVList[i*5+4]));
				}
			}
#endif

			if( revoked )
			{
				Tx_SwitchHDCPState(TxHDCP_RepeaterFail);
				break;
			}
		}

		if(HDCP_GetVr(Vr)==ER_FAIL)
		{
			//TXHDCP_DEBUG_PRINTF(("HDCP_GetVr error \n"));
			Tx_SwitchHDCPState(TxHDCP_RepeaterFail);
			break;
		}

		if(HDCP_GetM0(M0)==ER_FAIL)
		{
			//TXHDCP_DEBUG_PRINTF(("HDCP_GetM0 error \n"));
			Tx_SwitchHDCPState(TxHDCP_RepeaterFail);
			break;
		}

		Tx_SwitchHDCPState(TxHDCP_CheckSHA);
		//break;
	case TxHDCP_CheckSHA:
	{
		///////////////////////////////////////
		//    do check SHA
		///////////////////////////////////////

		if(HDCP_CheckSHA(M0,TX_BStatus,TX_KSVList,TX_cDownStream,Vr)==ER_FAIL)
		{
			//TXHDCP_DEBUG_PRINTF(("HDCP_CheckSHA error \n"));
			Tx_SwitchHDCPState(TxHDCP_RepeaterFail);
			break;

		}


		//max7088 temp disable !!                    HDCP_ResumeRepeaterAuthenticate();

		//TXHDCP_DEBUG_PRINTF(("HDCP_Authenticate_Repeater=ER_SUCCESS\n"));

		//ForceReplySourceAuthentication();
		Tx_SwitchHDCPState(TxHDCP_RepeaterSuccess);
	}
	break;
	case TxHDCP_RepeaterFail:
		ucdata=HDMITX_ReadI2C_Byte(REG_TX_AUTH_STAT);
		//TXHDCP_DEBUG_PRINTF(("reg[%x]=%x\n",REG_TX_AUTH_STAT,ucdata));

		HDCP_Abort();
		Tx_SwitchHDCPState(TxHDCP_Off);
		break;
	case TxHDCP_RepeaterSuccess:
		Tx_SwitchHDCPState(TxHDCP_Authenticated);
		break;
	case TxHDCP_AuthFail:
	{
		// HDCP_Abort();
		Tx_SwitchHDCPState(TxHDCP_Off);
	}
	break;
	case TxHDCP_Authenticated:

		break;
	}


}

void HDMITXITEFULL_CODE_SECTION(HDCP_ResetAuthenticate) (void)
{

    // if (TxDev[TX0DEV].TxHDCPState==TxHDCP_Repeater)

    if(( TxDev[TX0DEV].TxHDCPState==TxHDCP_Authenticated)||(TxDev[TX0DEV].TxHDCPState==TxHDCP_Receiver))
    {
        Tx_SwitchHDCPState(TxHDCP_AuthFail);
    }
    else if( TxDev[TX0DEV].TxHDCPState !=TxHDCP_Off &&
        TxDev[TX0DEV].TxHDCPState !=TxHDCP_AuthRestart&&
        TxDev[TX0DEV].TxHDCPState !=TxHDCP_AuthStart )
    {
        Tx_SwitchHDCPState(TxHDCP_RepeaterFail);
    }
    else
    {
        Tx_SwitchHDCPState(TxHDCP_Off);
    }
}

