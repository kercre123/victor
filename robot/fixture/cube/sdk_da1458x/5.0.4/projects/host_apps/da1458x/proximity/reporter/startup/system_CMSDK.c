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
 *      Checked In          : $Date: 2011-03-17 11:30:08 +0000 (Thu, 17 Mar 2011) $
 *
 *      Revision            : $Revision: 164920 $
 *
 *      Release Information : BP200-r0p0-00rel0
 *-----------------------------------------------------------------------------
 */
/******************************************************************************
 * @file:    system_CMSDK.c
 * @purpose: CMSIS compatible Cortex-M0 Device Peripheral Access Layer Source File
 *           for the CMSDK
 * @version  $State:$
 * @date     $Date: 2011-03-17 11:30:08 +0000 (Thu, 17 Mar 2011) $
 *----------------------------------------------------------------------------
 *
 ******************************************************************************/

#include <stdint.h>

#ifdef CORTEX_M0
#include "CMSDK_CM0.h"
#endif
#ifdef CORTEX_M3
#include "CMSDK_CM3.h"
#endif
#ifdef CORTEX_M4
#include "CMSDK_CM4.h"
#endif

/*
//-------- <<< Use Configuration Wizard in Context Menu >>> ------------------
*/

/*--------------------- Clock Configuration ----------------------------------
*/

/*
//-------- <<< end of configuration section >>> ------------------------------
*/

/*----------------------------------------------------------------------------
  DEFINES
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  Define clocks
 *----------------------------------------------------------------------------*/

#define XTAL    (100000000UL)            /* Oscillator frequency               */

/*----------------------------------------------------------------------------
  Clock Variable definitions
 *----------------------------------------------------------------------------*/
uint32_t SystemFrequency = XTAL;    /*!< System Clock Frequency (Core Clock)  */
uint32_t SystemCoreClock = XTAL;    /*!< Processor Clock Frequency            */


/**
 * Initialize the system
 *
 * @param  none
 * @return none
 *
 * @brief  Setup the microcontroller system.
 *         Initialize the System and update the SystemFrequency variable.
 */
void SystemInit (void)
{
  SystemCoreClock = XTAL;

  return;
}

/**
 * Update the SystemCoreClock variable
 *
 * @param  none
 * @return none
 *
 * @brief  Update the SystemCoreClock variable after clock setting changed.
 */
void SystemCoreClockUpdate (void)
{
 SystemCoreClock = XTAL;

}

