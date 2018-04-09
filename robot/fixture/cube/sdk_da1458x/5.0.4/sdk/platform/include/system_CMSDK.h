/*
 *-----------------------------------------------------------------------------
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from ARM Limited.
 *
 *            (C) COPYRIGHT 2010-2011 ARM Limited.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from ARM Limited.
 *
 *      SVN Information
 *
 *      Checked In          : $Date: Mon Oct  1 11:08:52 2012 $
 *
 *      Revision            : $Revision: 1.1 $
 *
 *      Release Information : BP200-r0p0-00rel0
 *-----------------------------------------------------------------------------
 */
/**************************************************************************//**
 * @file     system_CMSDK.h
 * @brief    CMSIS compatible Cortex-M0 Device Peripheral Access Layer Header File
 *           for the CMSDK
 * @version  $State: Experimental $
 * @date     $Date: Mon Oct  1 11:08:52 2012 $
 *
 ******************************************************************************/


#ifndef __SYSTEM_CMSDK_H
#define __SYSTEM_CMSDK_H

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t SystemFrequency;    /*!< System Clock Frequency (Core Clock)  */
extern uint32_t SystemCoreClock;    /*!< Processor Clock Frequency            */
/**
 * Initialize the system
 *
 * @param  none
 * @return none
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System and update the SystemFrequency variable.
 */
extern void SystemInit (void);
/**
 * Update the SystemCoreClock variable
 *
 * @param  none
 * @return none
 *
 * @brief  Update the SystemCoreClock variable after clock setting changed.
 */
extern void SystemCoreClockUpdate (void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_CMSDK_H */

