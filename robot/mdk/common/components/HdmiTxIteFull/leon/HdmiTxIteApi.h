/*****************************************************************************
   @file
   @copyright All code copyright Movidius Ltd 2012, all rights reserved
              For License Warranty see: common/license.txt
			  
   @defgroup HdmiTxIteApi2 HdmiTxIteFull component
   @{
   @brief     Hdmi Tx Ite Functions API.

 *****************************************************************************/
#ifndef _HDMI_TX_ITE_API_H_
#define _HDMI_TX_ITE_API_H_

/******************************************************************************
 1: Included types first then APIs from other modules
******************************************************************************/

#include "HdmiTxIteApiDefines.h"
#include "DrvI2cMasterDefines.h"

#define VIC 4

/*! !!! Important for Integration !!! */
/*! !!! in project ldscrip place 
"INCLUDE HdmiTxIteCodeFull.ldscript" 
"INCLUDE HdmiTxIteDataFull.ldscript"
in CMX (separate section)!!! */
/*! !!! in project makefile write: "DirLDScrCommon += -L ../../../common/components/HdmiTxIteFull" after inclusion of generic.mk   !!! */
/******************************************************************************
 2:  Exported Global Data (generally better to avoid)
******************************************************************************/


/******************************************************************************
  3:  Exported Functions (non-inline)
******************************************************************************/

///Configure the i2c for tx driver
///@param hdmiI2cHandle the I2C device handle
int HdmiSetup(I2CM_Device * hdmiI2cHandle);

///Configure the entire TX
void HdmiConfigure(void);
/// @}
#endif // _HDMI_TX_ITE_API_H_
