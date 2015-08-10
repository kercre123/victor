/*
 * Copyright (c) 2013 - 2014, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_llwu_hal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN
/*FUNCTION**********************************************************************
 *
 * Function Name : LLWU_HAL_SetExternalInputPinMode
 * Description   : Set external input pin source mode
 * This function will set the external input pin source mode that will be used
 * as wake up source.
 * 
 *END**************************************************************************/
void LLWU_HAL_SetExternalInputPinMode(uint32_t baseAddr,
                                      llwu_external_pin_modes_t pinMode,
                                      llwu_wakeup_pin_t pinNumber)
{
    switch (pinNumber)
    {
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN0
    case 0:
        BW_LLWU_PE1_WUPE0(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN1
    case 1:
        BW_LLWU_PE1_WUPE1(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN2
    case 2:
        BW_LLWU_PE1_WUPE2(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN3
    case 3:
        BW_LLWU_PE1_WUPE3(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN4
    case 4:
        BW_LLWU_PE2_WUPE4(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN5
    case 5:
        BW_LLWU_PE2_WUPE5(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN6
    case 6:
        BW_LLWU_PE2_WUPE6(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN7
    case 7:
        BW_LLWU_PE2_WUPE7(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN8
    case 8:
        BW_LLWU_PE3_WUPE8(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN9
    case 9:
        BW_LLWU_PE3_WUPE9(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN10
    case 10:
        BW_LLWU_PE3_WUPE10(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN11
    case 11:
        BW_LLWU_PE3_WUPE11(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN12
    case 12:
        BW_LLWU_PE4_WUPE12(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN13
    case 13:
        BW_LLWU_PE4_WUPE13(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN14
    case 14:
        BW_LLWU_PE4_WUPE14(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN15
    case 15:
        BW_LLWU_PE4_WUPE15(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN16
    case 16:
        BW_LLWU_PE5_WUPE16(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN17
    case 17:
        BW_LLWU_PE5_WUPE17(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN18
    case 18:
        BW_LLWU_PE5_WUPE18(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN19
    case 19:
        BW_LLWU_PE5_WUPE19(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN20
    case 20:
        BW_LLWU_PE6_WUPE20(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN21
    case 21:
        BW_LLWU_PE6_WUPE21(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN22
    case 22:
        BW_LLWU_PE6_WUPE22(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN23
    case 23:
        BW_LLWU_PE6_WUPE23(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN24
    case 24:
        BW_LLWU_PE7_WUPE24(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN25
    case 25:
        BW_LLWU_PE7_WUPE25(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN26
    case 26:
        BW_LLWU_PE7_WUPE26(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN27
    case 27:
        BW_LLWU_PE7_WUPE27(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN28
    case 28:
        BW_LLWU_PE8_WUPE28(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN29
    case 29:
        BW_LLWU_PE8_WUPE29(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN30
    case 30:
        BW_LLWU_PE8_WUPE30(baseAddr, pinMode);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN31
    case 31:
        BW_LLWU_PE8_WUPE31(baseAddr, pinMode);
        break;
#endif
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LLWU_HAL_GetExternalInputPinMode
 * Description   : Get external input pin source mode
 * This function will get the external input pin source mode that will be used
 * as wake up source.
 * 
 *END**************************************************************************/
llwu_external_pin_modes_t LLWU_HAL_GetExternalInputPinMode(uint32_t baseAddr,
                                                           llwu_wakeup_pin_t pinNumber)
{
    llwu_external_pin_modes_t retValue;

    switch (pinNumber)
    {
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN0
    case 0:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE1_WUPE0(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN1
    case 1:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE1_WUPE1(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN2
    case 2:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE1_WUPE2(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN3
    case 3:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE1_WUPE3(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN4
    case 4:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE2_WUPE4(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN5
    case 5:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE2_WUPE5(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN6
    case 6:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE2_WUPE6(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN7
    case 7:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE2_WUPE7(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN8
    case 8:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE3_WUPE8(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN9
    case 9:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE3_WUPE9(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN10
    case 10:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE3_WUPE10(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN11
    case 11:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE3_WUPE11(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN12
    case 12:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE4_WUPE12(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN13
    case 13:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE4_WUPE13(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN14
    case 14:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE4_WUPE14(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN15
    case 15:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE4_WUPE15(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN16
    case 16:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE5_WUPE16(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN17
    case 17:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE5_WUPE17(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN18
    case 18:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE5_WUPE18(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN19
    case 19:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE5_WUPE19(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN20
    case 20:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE6_WUPE20(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN21
    case 21:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE6_WUPE21(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN22
    case 22:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE6_WUPE22(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN23
    case 23:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE6_WUPE23(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN24
    case 24:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE7_WUPE24(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN25
    case 25:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE7_WUPE25(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN26
    case 26:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE7_WUPE26(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN27
    case 27:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE7_WUPE27(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN28
    case 28:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE8_WUPE28(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN29
    case 29:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE8_WUPE29(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN30
    case 30:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE8_WUPE30(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN31
    case 31:
        retValue = (llwu_external_pin_modes_t)BR_LLWU_PE8_WUPE31(baseAddr);
        break;
#endif
    default:
        retValue = (llwu_external_pin_modes_t)0;
        break;
    }

    return retValue;
}
#endif

#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE
/*FUNCTION**********************************************************************
 *
 * Function Name : LLWU_HAL_SetInternalModuleCmd
 * Description   : Enable/disable internal module source
 * This function will enable/disable the internal module source mode that will 
 * be used as wake up source. 
 * 
 *END**************************************************************************/
void LLWU_HAL_SetInternalModuleCmd(uint32_t baseAddr, llwu_wakeup_module_t moduleNumber, bool enable)
{
    switch (moduleNumber)
    {
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE0
    case 0:
        BW_LLWU_ME_WUME0(baseAddr, enable);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE1
    case 1:
        BW_LLWU_ME_WUME1(baseAddr, enable);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE2
    case 2:
        BW_LLWU_ME_WUME2(baseAddr, enable);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE3
    case 3:
        BW_LLWU_ME_WUME3(baseAddr, enable);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE4
    case 4:
        BW_LLWU_ME_WUME4(baseAddr, enable);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE5
    case 5:
        BW_LLWU_ME_WUME5(baseAddr, enable);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE6
    case 6:
        BW_LLWU_ME_WUME6(baseAddr, enable);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE7
    case 7:
        BW_LLWU_ME_WUME7(baseAddr, enable);
        break;
#endif
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LLWU_HAL_GetInternalModuleCmd
 * Description   : Get internal module source enable setting
 * This function will enable/disable the internal module source mode that will 
 * be used as wake up source. 
 * 
 *END**************************************************************************/
bool LLWU_HAL_GetInternalModuleCmd(uint32_t baseAddr, llwu_wakeup_module_t moduleNumber)
{
    bool retValue;

    switch (moduleNumber)
    {
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE0
    case 0:
        retValue = (bool)BR_LLWU_ME_WUME0(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE1
    case 1:
        retValue = (bool)BR_LLWU_ME_WUME1(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE2
    case 2:
        retValue = (bool)BR_LLWU_ME_WUME2(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE3
    case 3:
        retValue = (bool)BR_LLWU_ME_WUME3(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE4
    case 4:
        retValue = (bool)BR_LLWU_ME_WUME4(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE5
    case 5:
        retValue = (bool)BR_LLWU_ME_WUME5(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE6
    case 6:
        retValue = (bool)BR_LLWU_ME_WUME6(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE7
    case 7:
        retValue = (bool)BR_LLWU_ME_WUME7(baseAddr);
        break;
#endif
    default:
        retValue = false;
        break;
    }

    return retValue;
}
#endif

#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN

/* For some platforms register LLWU_PF is used instead of LLWU_F. */
#if defined(BR_LLWU_PF1_WUF0)
#define BR_LLWU_PIN_FLAG(reg, index) BR_LLWU_PF##reg##_WUF##index
#else
#define BR_LLWU_PIN_FLAG(reg, index) BR_LLWU_F##reg##_WUF##index
#endif

#if defined(BW_LLWU_PF1_WUF0)
#define BW_LLWU_PIN_FLAG(reg, index) BW_LLWU_PF##reg##_WUF##index
#else
#define BW_LLWU_PIN_FLAG(reg, index) BW_LLWU_F##reg##_WUF##index
#endif

/*FUNCTION**********************************************************************
 *
 * Function Name : LLWU_HAL_GetExternalPinWakeupFlag
 * Description   : Get external wakeup source flag
 * This function will get the external wakeup source flag for specific pin. 
 * 
 *END**************************************************************************/
bool LLWU_HAL_GetExternalPinWakeupFlag(uint32_t baseAddr, llwu_wakeup_pin_t pinNumber)
{
    bool retValue = false;
    switch (pinNumber)
    {
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN0
    case 0:
        retValue = (bool)BR_LLWU_PIN_FLAG(1,0)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN1
    case 1:
        retValue = (bool)BR_LLWU_PIN_FLAG(1,1)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN2
    case 2:
        retValue = (bool)BR_LLWU_PIN_FLAG(1,2)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN3
    case 3:
        retValue = (bool)BR_LLWU_PIN_FLAG(1,3)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN4
    case 4:
        retValue = (bool)BR_LLWU_PIN_FLAG(1,4)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN5
    case 5:
        retValue = (bool)BR_LLWU_PIN_FLAG(1,5)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN6
    case 6:
        retValue = (bool)BR_LLWU_PIN_FLAG(1,6)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN7
    case 7:
        retValue = (bool)BR_LLWU_PIN_FLAG(1,7)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN8
    case 8:
        retValue = (bool)BR_LLWU_PIN_FLAG(2,8)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN9
    case 9:
        retValue = (bool)BR_LLWU_PIN_FLAG(2,9)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN10
    case 10:
        retValue = (bool)BR_LLWU_PIN_FLAG(2,10)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN11
    case 11:
        retValue = (bool)BR_LLWU_PIN_FLAG(2,11)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN12
    case 12:
        retValue = (bool)BR_LLWU_PIN_FLAG(2,12)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN13
    case 13:
        retValue = (bool)BR_LLWU_PIN_FLAG(2,13)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN14
    case 14:
        retValue = (bool)BR_LLWU_PIN_FLAG(2,14)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN15
    case 15:
        retValue = (bool)BR_LLWU_PIN_FLAG(2,15)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN16
    case 16:
        retValue = (bool)BR_LLWU_PIN_FLAG(3,16)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN17
    case 17:
        retValue = (bool)BR_LLWU_PIN_FLAG(3,17)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN18
    case 18:
        retValue = (bool)BR_LLWU_PIN_FLAG(3,18)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN19
    case 19:
        retValue = (bool)BR_LLWU_PIN_FLAG(3,19)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN20
    case 20:
        retValue = (bool)BR_LLWU_PIN_FLAG(3,20)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN21
    case 21:
        retValue = (bool)BR_LLWU_PIN_FLAG(3,21)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN22
    case 22:
        retValue = (bool)BR_LLWU_PIN_FLAG(3,22)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN23
    case 23:
        retValue = (bool)BR_LLWU_PIN_FLAG(3,23)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN24
    case 24:
        retValue = (bool)BR_LLWU_PIN_FLAG(4,24)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN25
    case 25:
        retValue = (bool)BR_LLWU_PIN_FLAG(4,25)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN26
    case 26:
        retValue = (bool)BR_LLWU_PIN_FLAG(4,26)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN27
    case 27:
        retValue = (bool)BR_LLWU_PIN_FLAG(4,27)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN28
    case 28:
        retValue = (bool)BR_LLWU_PIN_FLAG(4,28)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN29
    case 29:
        retValue = (bool)BR_LLWU_PIN_FLAG(4,29)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN30
    case 30:
        retValue = (bool)BR_LLWU_PIN_FLAG(4,30)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN31
    case 31:
        retValue = (bool)BR_LLWU_PIN_FLAG(4,31)(baseAddr);
        break;
#endif
    default:
        retValue = false;
        break;
    }

    return retValue;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LLWU_HAL_ClearExternalPinWakeupFlag
 * Description   : Clear external wakeup source flag
 * This function will clear the external wakeup source flag for specific pin.
 * 
 *END**************************************************************************/
void LLWU_HAL_ClearExternalPinWakeupFlag(uint32_t baseAddr, llwu_wakeup_pin_t pinNumber)
{
    switch (pinNumber)
    {
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN0
    case 0:
        BW_LLWU_PIN_FLAG(1,0)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN1
    case 1:
        BW_LLWU_PIN_FLAG(1,1)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN2
    case 2:
        BW_LLWU_PIN_FLAG(1,2)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN3
    case 3:
        BW_LLWU_PIN_FLAG(1,3)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN4
    case 4:
        BW_LLWU_PIN_FLAG(1,4)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN5
    case 5:
        BW_LLWU_PIN_FLAG(1,5)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN6
    case 6:
        BW_LLWU_PIN_FLAG(1,6)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN7
    case 7:
        BW_LLWU_PIN_FLAG(1,7)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN8
    case 8:
        BW_LLWU_PIN_FLAG(2,8)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN9
    case 9:
        BW_LLWU_PIN_FLAG(2,9)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN10
    case 10:
        BW_LLWU_PIN_FLAG(2,10)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN11
    case 11:
        BW_LLWU_PIN_FLAG(2,11)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN12
    case 12:
        BW_LLWU_PIN_FLAG(2,12)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN13
    case 13:
        BW_LLWU_PIN_FLAG(2,13)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN14
    case 14:
        BW_LLWU_PIN_FLAG(2,14)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN15
    case 15:
        BW_LLWU_PIN_FLAG(2,15)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN16
    case 16:
        BW_LLWU_PIN_FLAG(3,16)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN17
    case 17:
        BW_LLWU_PIN_FLAG(3,17)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN18
    case 18:
        BW_LLWU_PIN_FLAG(3,18)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN19
    case 19:
        BW_LLWU_PIN_FLAG(3,19)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN20
    case 20:
        BW_LLWU_PIN_FLAG(3,20)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN21
    case 21:
        BW_LLWU_PIN_FLAG(3,21)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN22
    case 22:
        BW_LLWU_PIN_FLAG(3,22)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN23
    case 23:
        BW_LLWU_PIN_FLAG(3,23)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN24
    case 24:
        BW_LLWU_PIN_FLAG(4,24)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN25
    case 25:
        BW_LLWU_PIN_FLAG(4,25)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN26
    case 26:
        BW_LLWU_PIN_FLAG(4,26)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN27
    case 27:
        BW_LLWU_PIN_FLAG(4,27)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN28
    case 28:
        BW_LLWU_PIN_FLAG(4,28)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN29
    case 29:
        BW_LLWU_PIN_FLAG(4,29)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN30
    case 30:
        BW_LLWU_PIN_FLAG(4,30)(baseAddr, 1);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN31
    case 31:
        BW_LLWU_PIN_FLAG(4,31)(baseAddr, 1);
        break;
#endif
    default:
        break;
    }
}
#endif

#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE

/* If LLWU_MF is used. */
#if FSL_FEATURE_LLWU_HAS_MF
#define BR_LLWU_MODULE_FLAG(index) BR_LLWU_MF5_MWUF##index
#else
#define BR_LLWU_MODULE_FLAG(index) BR_LLWU_F3_MWUF##index
#endif

/*FUNCTION**********************************************************************
 *
 * Function Name : LLWU_HAL_GetInternalModuleWakeupFlag
 * Description   : Get internal module wakeup source flag
 * This function will get the internal module wakeup source flag for specific 
 * module
 * 
 *END**************************************************************************/
bool LLWU_HAL_GetInternalModuleWakeupFlag(uint32_t baseAddr, llwu_wakeup_module_t moduleNumber)
{
    bool retValue = false;

    switch (moduleNumber)
    {
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE0
    case 0:
        retValue = (bool)BR_LLWU_MODULE_FLAG(0)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE1
    case 1:
        retValue = (bool)BR_LLWU_MODULE_FLAG(1)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE2
    case 2:
        retValue = (bool)BR_LLWU_MODULE_FLAG(2)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE3
    case 3:
        retValue = (bool)BR_LLWU_MODULE_FLAG(3)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE4
    case 4:
        retValue = (bool)BR_LLWU_MODULE_FLAG(4)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE5
    case 5:
        retValue = (bool)BR_LLWU_MODULE_FLAG(5)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE6
    case 6:
        retValue = (bool)BR_LLWU_MODULE_FLAG(6)(baseAddr);
        break;
#endif
#if FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE7
    case 7:
        retValue = (bool)BR_LLWU_MODULE_FLAG(7)(baseAddr);
        break;
#endif
    default:
        retValue = false;
        break;
    }

    return retValue;
}
#endif

#if FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN
/*FUNCTION**********************************************************************
 *
 * Function Name : LLWU_HAL_SetPinFilterMode
 * Description   : Set pin filter configuration
 * This function will set the pin filter configuration.
 * 
 *END**************************************************************************/
void LLWU_HAL_SetPinFilterMode(uint32_t baseAddr,
                               uint32_t filterNumber,
                               llwu_external_pin_filter_mode_t pinFilterMode)
{
    /* check filter and pin number */
    assert(filterNumber < FSL_FEATURE_LLWU_HAS_PIN_FILTER);

    /* branch to filter number */
    switch(filterNumber)
    {
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 1)
    case 0:
        BW_LLWU_FILT1_FILTSEL(baseAddr, pinFilterMode.pinNumber);
        BW_LLWU_FILT1_FILTE(baseAddr, pinFilterMode.filterMode);
        break;
#endif
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 2)
    case 1:
        BW_LLWU_FILT2_FILTSEL(baseAddr, pinFilterMode.pinNumber);
        BW_LLWU_FILT2_FILTE(baseAddr, pinFilterMode.filterMode);
        break;
#endif
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 3)
    case 2:
        BW_LLWU_FILT3_FILTSEL(baseAddr, pinFilterMode.pinNumber);
        BW_LLWU_FILT3_FILTE(baseAddr, pinFilterMode.filterMode);
        break;
#endif
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 4)
    case 3:
        BW_LLWU_FILT4_FILTSEL(baseAddr, pinFilterMode.pinNumber);
        BW_LLWU_FILT4_FILTE(baseAddr, pinFilterMode.filterMode);
        break;
#endif
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LLWU_HAL_GetPinFilterMode
 * Description   : Get pin filter configuration.
 * This function will get the pin filter configuration.
 * 
 *END**************************************************************************/
void LLWU_HAL_GetPinFilterMode(uint32_t baseAddr,
                               uint32_t filterNumber, 
                               llwu_external_pin_filter_mode_t *pinFilterMode)
{
    /* check filter and pin number */
    assert(filterNumber < FSL_FEATURE_LLWU_HAS_PIN_FILTER);

    /* branch to filter number */
    switch(filterNumber)
    {
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 1)
    case 0:
        pinFilterMode->pinNumber = (llwu_wakeup_pin_t)BR_LLWU_FILT1_FILTSEL(baseAddr);
        pinFilterMode->filterMode = (llwu_filter_modes_t)BR_LLWU_FILT1_FILTE(baseAddr);
        break;
#endif
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 2)
    case 1:
        pinFilterMode->pinNumber = (llwu_wakeup_pin_t)BR_LLWU_FILT2_FILTSEL(baseAddr);
        pinFilterMode->filterMode = (llwu_filter_modes_t)BR_LLWU_FILT2_FILTE(baseAddr);
        break;
#endif
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 3)
    case 2:
        pinFilterMode->pinNumber = (llwu_wakeup_pin_t)BR_LLWU_FILT3_FILTSEL(baseAddr);
        pinFilterMode->filterMode = (llwu_filter_modes_t)BR_LLWU_FILT3_FILTE(baseAddr);
        break;
#endif
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 4)
    case 3:
        pinFilterMode->pinNumber = (llwu_wakeup_pin_t)BR_LLWU_FILT4_FILTSEL(baseAddr);
        pinFilterMode->filterMode = (llwu_filter_modes_t)BR_LLWU_FILT4_FILTE(baseAddr);
        break;
#endif
    default:
        break;
    }
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LLWU_HAL_GetFilterDetectFlag
 * Description   : Get filter detect flag
 * This function will get the filter detect flag.
 * 
 *END**************************************************************************/
bool LLWU_HAL_GetFilterDetectFlag(uint32_t baseAddr, uint32_t filterNumber)
{
    bool retValue = false;

    /* check filter and pin number */
    assert(filterNumber < FSL_FEATURE_LLWU_HAS_PIN_FILTER);

    /* branch to filter number */
    switch(filterNumber)
    {
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 1)
    case 0:
        retValue = (bool)BR_LLWU_FILT1_FILTF(baseAddr);
        break;
#endif
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 2)
    case 1:
        retValue = (bool)BR_LLWU_FILT2_FILTF(baseAddr);
        break;
#endif
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 3)
    case 2:
        retValue = (bool)BR_LLWU_FILT3_FILTF(baseAddr);
        break;
#endif
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 4)
    case 3:
        retValue = (bool)BR_LLWU_FILT4_FILTF(baseAddr);
        break;
#endif
    default:
        retValue = false;
        break;
    }

    return retValue;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LLWU_HAL_ClearFilterDetectFlag
 * Description   : Clear filter detect flag
 * This function will clear the filter detect flag.
 * 
 *END**************************************************************************/
void LLWU_HAL_ClearFilterDetectFlag(uint32_t baseAddr, uint32_t filterNumber)
{
    /* check filter and pin number */
    assert(filterNumber < FSL_FEATURE_LLWU_HAS_PIN_FILTER);

    /* branch to filter number */
    switch(filterNumber)
    {
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 1)
    case 0:
        BW_LLWU_FILT1_FILTF(baseAddr, 1);
        break;
#endif
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 2)
    case 1:
        BW_LLWU_FILT2_FILTF(baseAddr, 1);
        break;
#endif
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 3)
    case 2:
        BW_LLWU_FILT3_FILTF(baseAddr, 1);
        break;
#endif
#if (FSL_FEATURE_LLWU_HAS_PIN_FILTER >= 4)
    case 3:
        BW_LLWU_FILT4_FILTF(baseAddr, 1);
        break;
#endif
    default:
        break;
    }
}
#endif

#if FSL_FEATURE_LLWU_HAS_RESET_ENABLE
/*FUNCTION**********************************************************************
 *
 * Function Name : LLWU_HAL_SetResetEnableMode
 * Description   : Set reset enable mode
 * This function will set the reset enable mode.
 * 
 *END**************************************************************************/
void LLWU_HAL_SetResetEnableMode(uint32_t baseAddr, llwu_reset_enable_mode_t resetEnableMode)
{
    BW_LLWU_RST_RSTFILT(baseAddr, resetEnableMode.digitalFilterMode);
    BW_LLWU_RST_LLRSTE(baseAddr, resetEnableMode.lowLeakageMode);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : LLWU_HAL_GetResetEnableMode
 * Description   : Get reset enable mode
 * This function will get the reset enable mode.
 * 
 *END**************************************************************************/
void LLWU_HAL_GetResetEnableMode(uint32_t baseAddr, llwu_reset_enable_mode_t *resetEnableMode)
{
    resetEnableMode->digitalFilterMode = (bool)BR_LLWU_RST_RSTFILT(baseAddr);
    resetEnableMode->lowLeakageMode = (bool)BR_LLWU_RST_LLRSTE(baseAddr);
}
#endif

/*******************************************************************************
 * EOF
 ******************************************************************************/

