/*****************************************************************************
     @file   <TxHdcp.h>
     @author Max.Kao@ite.com.tw
     @date   2011/11/18
     @fileversion: ITE_HDMI1.4_AVR_SAMPLECODE_2.04
     Authentication
 *****************************************************************************/

#ifndef _HDCP_h_
#define _HDCP_h_

/******************************************************************************
 1: Included types first then Apis from other modules
******************************************************************************/
#include "HdmiTxIteApi.h"
#include "HdmiTxIteDrv.h"

/******************************************************************************
 2:  Source File Specific #defines
******************************************************************************/

#ifndef MAX_REPEATER_DOWNSTREAM_COUNT
//#pragma message ("not predefined MAX_REPEATER_COUNT, defined as 6")
#define MAX_REPEATER_DOWNSTREAM_COUNT 6
#else
//#pragma message ("Predefined MAX_REPEATER_COUNT")
#endif

/******************************************************************************
 3:  Exported variables
******************************************************************************/

extern HdmiDriverStatus    HdmiDriverInfo;

extern  BYTE    TX_cDownStream;
extern  WORD    TX_BStatus;
extern  BYTE    TX_BKSV[5];
extern  BYTE    TX_KSVList[30];
extern  BYTE    Vr[20];
extern  BYTE    M0[8];
extern  BYTE    SHABuff[64];
extern  BYTE    V[20];
extern  BYTE    Ri[2];//mingchih add
extern  ULONG   sha[5];

/******************************************************************************
 4:  Functions
******************************************************************************/
void    GenerateDDCSCLK (void);
void    HDCP_GenerateAn (void);
void    HDCP_Auth_Fire (void);
void    HDCP_Abort (void);
void    HDCP_ResumeRepeaterAuthenticate (void);
void    HDCP_CancelRepeaterAuthenticate (void);
void    SHATransform (ULONG * h);
void    SHA_Simple (void *p,WORD len,BYTE *output);
void    HDCP_ClearAuthInterrupt (void);
void    TxHDCP_Handler (void);
void    TX_HDCP_Handler (void);
void    Tx_SwitchHDCPState (Tx_HDCP_State_Type state);
void    SwitchTXHDCPState (BYTE state);
void    HDCP_ResetAuthenticate (void);

BYTE    countbit (BYTE b);

SYS_STATUS  HDCP_GetM0      (BYTE *pM0);
SYS_STATUS  HDCP_CheckSHA   (BYTE pM0[], USHORT BStatus, BYTE pKSVList[], BYTE cDownStream, BYTE cVr[]);
SYS_STATUS  HDCP_GetBCaps   (PBYTE pBCaps, PUSHORT pBStatus);
SYS_STATUS  HDCP_GetBKSV    (BYTE *pBKSV);
SYS_STATUS  HDCP_GetKSVList (BYTE *pKSVList,BYTE cDownStream);
SYS_STATUS  HDCP_GetVr      (BYTE *pVr);
SYS_STATUS  HDCP_EnableEncryption (void);


#endif //_HDCP_h_
