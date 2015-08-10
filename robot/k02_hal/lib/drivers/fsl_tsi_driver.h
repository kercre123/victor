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
#ifndef __FSL_TSI_DRIVER_H__
#define __FSL_TSI_DRIVER_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "fsl_os_abstraction.h"
#include "fsl_tsi_hal.h"

/*!
 * @addtogroup tsi_driver
 * @{
 */

/*!
 * @file
 *
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*!
* @brief Call back routine of TSI driver.
*
* The function is called on end of the measure of the TSI driver. The function
* can be called from interrupt, so the code inside the callback should be short
* and fast.
* @param instance - instance of the TSI peripheral
* @param usrData - user data (type is void*), the user data are specified by function @ref TSI_DRV_SetCallBackFunc
* @return - none
*/
typedef void (*tsi_callback_t)(uint32_t instance, void* usrData);

/*!
* @brief User configuration structure for TSI driver.
*
* Use an instance of this structure with TSI_DRV_Init(). This allows you to configure the
* most common settings of the TSI peripheral with a single function call. Settings include:
*
*/
typedef struct TsiUserConfig {
    tsi_config_t                        *config;        /**< A pointer to hardware configuration. Can't be NULL. */
    tsi_callback_t                      pCallBackFunc;  /**< A pointer to call back function of end of mesurement. */
    void                                * usrData;      /**< A user data of call back function. */
} tsi_user_config_t;

/*!
* @brief Driver operation mode definition.
*
* The operation name definition used for TSI driver.
*
*/
typedef enum TsiModes
{
  tsi_OpModeNormal = 0,        /**< The normal mode of TSI. */
  tsi_OpModeProximity,         /**< The proximity sensing mode of TSI. */
  tsi_OpModeLowPower,          /**< The low power mode of TSI. */
  tsi_OpModeNoise,             /**< The noise mode of TSI. This mode is not valid with TSI HW, valid only fot TSIL HW. */
  tsi_OpModeCnt,               /**< Count of TSI modes - for internal use. */
  tsi_OpModeNoChange           /**< The special value of operation mode that allows call for example @ref TSI_DRV_DisableLowPower function without change of operation mode. */
}tsi_modes_t;

/*!
* @brief Driver operation mode data hold structure.
*
* This is operation mode data hold structure. The structure is keep all needed data
* to be driver able to switch the operation modes and properly set up HW peripheral.
*
*/
typedef struct TsiOperationMode
{
  uint16_t                              enabledElectrodes;  /**< The back up of enabled electrodes for operation mode */
  tsi_config_t                          config;             /**< A hardware configuration. */
}tsi_operation_mode_t;

/*!
* @brief Driver data storage place.
*
* It must be created by application code and the pointer is handled by @ref TSI_DRV_Init function
* to driver. The driver keeps all contex data for itself run. Settings include:
*
*/
typedef struct TsiState {
  tsi_status_t                          status;             /**< Current status of the driver. */
  tsi_callback_t                        pCallBackFunc;      /**< A pointer to call back function of end of measurement. */
  void                                  *usrData;           /**< A user data pointer handled by call back function. */
  semaphore_t                           irqSync;            /**< Used to wait for ISR to complete its measure business. */
  mutex_t                               lock;               /**< Used by whole driver to secure the context data integrity. */
  mutex_t                               lockChangeMode;     /**< Used by chenge mode function to secure the context data integrity. */
  bool                                  isBlockingMeasure;  /**< Used to internal indication of type of measurement. */
  tsi_modes_t                           opMode;             /**< Storage of current operation mode. */
  tsi_operation_mode_t                  opModesData[tsi_OpModeCnt]; /**< Data storage of individual operational modes. */
  uint16_t                              counters[FSL_FEATURE_TSI_CHANNEL_COUNT]; /**< The mirrow of last state of counter registers */
}tsi_state_t;


/*! @brief Table of base addresses for tsi instances. */
extern const uint32_t g_tsiBaseAddr[];

/*! @brief Table to save tsi IRQ enum numbers defined in CMSIS header file. */
extern const IRQn_Type g_tsiIrqId[HW_TSI_INSTANCE_COUNT];

/*! @brief Table to save pointers to context data. */
extern tsi_state_t * g_tsiStatePtr[HW_TSI_INSTANCE_COUNT];

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization
 * @{
 */

/*!
* @brief This function initializes a TSI instance for operation.
*
* This function will initialize the run-time state structure and prepare the
* whole peripheral for measure the capacitances on electrodes.
 @code

   static tsi_state_t myTsiDriverStateStructure;

   tsi_user_config_t myTsiDriveruserConfig =
   {
    .config =
       {
          ...
       },
     .pCallBackFunc = APP_myTsiCallBackFunc,
     .usrData = myData,
   };

   if(TSI_DRV_Init(0, &myTsiDriverStateStructure, &myTsiDriveruserConfig) != kStatus_TSI_Success)
   {
      // Error, the TSI is not initialized
   }
  @endcode
*
* @param instance The TSI module instance.
* @param tsiState A pointer to the TSI driver state structure memory. The user is only
*  responsible to pass in the memory for this run-time state structure where the TSI driver
*  will take care of filling out the members. This run-time state structure keeps track of the
*  current TSI peripheral and driver state.
* @param tsiUserConfig The user configuration structure of type tsi_user_config_t. The user
*  is responsbile to fill out the members of this structure and to pass the pointer of this struct
*  into this function.
* @return An error code or kStatus_TSI_Success.
*/
tsi_status_t TSI_DRV_Init(const uint32_t instance, tsi_state_t * tsiState, const tsi_user_config_t * tsiUserConfig);

/*!
* @brief This function shuts down the TSI by disabling interrupts and disable the peripheral itself.
*
* This function disables the TSI interrupts and disables the peripheral.
*
 @code
   if(TSI_DRV_DeInit(0) != kStatus_TSI_Success)
   {
      // Error, the TSI is not deinitialized
   }
  @endcode
* @param instance The TSI module instance.
* @return An error code or kStatus_TSI_Success.
*/
tsi_status_t TSI_DRV_DeInit(const uint32_t instance);

/*!
* @brief This function enables/disables one electrode of TSI module.
*
* Function must be called for each used electrodes after initialization of TSI driver.
*
  @code
        // On the TSI instance 0, enable electrode with index 5
    if(TSI_DRV_EnableElectrode(0, 5, TRUE) != kStatus_TSI_Success)
    {
        // Error, the TSI 5'th electrode is not enabled
    }
  @endcode
* @param instance   The TSI module instance.
* @param channel    Index of TSI channel.
* @param enable     TRUE - for channel enable, FALSE for disable.
* @return An error code or kStatus_TSI_Success.
*/
tsi_status_t TSI_DRV_EnableElectrode(const uint32_t instance, const uint32_t channel, const bool enable);

/*!
* @brief This function return mask of enabled electrodes of TSI module.
*
* Function returns mask of the enabled electrodes of current mode.
*
  @code
    uint32_t enabledElectrodeMask;
    enabledElectrodeMask = TSI_DRV_GetEnabledElectrodes(0);
  @endcode
* @param instance The TSI module instance.
* @return Mask of enebled electrodes for current mode.
*/
uint32_t TSI_DRV_GetEnabledElectrodes(const uint32_t instance);

/*!
* @brief This function starts the measure cycle of enabled electrodes.
*
* Function is non blocking, so for the result must be wait after driver finish the measure cycle.
*         The end of measure cycle can be checked by pooling @ref TSI_DRV_GetStatus function or wait for registered callback function by
*         @ref TSI_DRV_SetCallBackFunc or @ref TSI_DRV_Init.
*
  @code
    // Example of pooling style of use of TSI_DRV_Measure() function
    if(TSI_DRV_Measure(0) != kStatus_TSI_Success)
    {
        // Error, the TSI 5'th electrode is not enabled
    }

    while(TSI_DRV_GetStatus(0) != kStatus_TSI_Initialized)
    {
        // Do something useful - don't waste the CPU cycle time
    }

  @endcode
* @param instance The TSI module instance.
* @return An error code or kStatus_TSI_Success.
*/
tsi_status_t TSI_DRV_Measure(const uint32_t instance);

/*!
* @brief This function starts the measure cycle of enabled electrodes in blocking mode.
*
* Function is blocking, so the after finish this function call, the result of measured electrodes
*         is available immediately and can be get by @ref TSI_DRV_GetCounter function.
*
  @code
    // Example of pooling style of use of TSI_DRV_Measure() function
    if(TSI_DRV_MeasureBlocking(0) != kStatus_TSI_Success)
    {
        // Error, the TSI 5'th electrode is not enabled
    }else
    {
        // Get the result by TSI_DRV_GetCounter function
    }
  @endcode
* @param instance The TSI module instance.
* @return An error code or kStatus_TSI_Success.
*/
tsi_status_t TSI_DRV_MeasureBlocking(const uint32_t instance);

/*!
* @brief This function aborts the measure cycle in non standard situation use of driver.
*
* Function aborts currently running measure cycle, and it's designed for
*           unexpected situations. Is not targeted for standard use.
*
  @code
    // Start measure by @ref TSI_DRV_Measure() function
    if(TSI_DRV_Measure(0) != kStatus_TSI_Success)
    {
        // Error, the TSI 5'th electrode is not enabled
    }

    if(isNeededAbort) // I need abort measure from any application reason
    {
        TSI_DRV_AbortMeasure(0);
    }

  @endcode
* @param instance The TSI module instance.
* @return An error code or kStatus_TSI_Success.
*/
tsi_status_t TSI_DRV_AbortMeasure(const uint32_t instance);

/*!
* @brief This function return the last value of measurement values.
*
* Function return the last measure values of previous measure cycle of driver
*           the data are buffered inside of driver.
*
  @code
    // Get the counter value from TSI instance 0 and 5th channel

    uint32_t result;

    if(TSI_DRV_GetCounter(0, 5, &result) != kStatus_TSI_Success)
    {
        // Error, the TSI 5'th electrode is not readed
    }

  @endcode
* @param instance The TSI module instance.
* @param channel The TSI electrode index.
* @param counter The pointer to 16 bit value where will be stored channel counter value.
* @return An error code or kStatus_TSI_Success.
*/
tsi_status_t TSI_DRV_GetCounter(const uint32_t instance, const uint32_t channel, uint16_t * counter);

/*!
* @brief This function return the current status of the driver.
*
* Function return the current working status of the driver.
*
  @code
    // Get the current status of TSI driver

    tsi_status_t status;

    status = TSI_DRV_GetStatus(0);


  @endcode
* @param instance The TSI module instance.
* @return An current status of the driver.
*/
tsi_status_t TSI_DRV_GetStatus(const uint32_t instance);

/*!
* @brief This function enters to low power mode of TSI driver.
*
* Function switch the driver to low power mode and immediately enable
*           the low power functionality of TSI peripheral. Before calling this
*           function the low power mode must be configured - Enabled right electrode
*           and recalibrate the low power mode to get best performance for this mode.
*
  @code
    // Switch the driver to the low power mode
    uint16_t signal;

    // For fisrt time is needed configure the low power mode configuration

    (void)TSI_DRV_ChangeMode(0, tsi_OpModeLowPower); // I don't check the result because I believe in.
    // Enable the right one electrode for low power ake up operation
    (void)TSI_DRV_EnableElectrode(0, 5, true);
    // Recalibrate the mode to get the best performance for this one electrode
    (void)TSI_DRV_Recalibrate(0, &signal);

    if(TSI_DRV_EnableLowPower(0) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't go to low power mode
    }


  @endcode
* @param instance The TSI module instance.
* @return An error code or kStatus_TSI_Success.
*/
tsi_status_t TSI_DRV_EnableLowPower(const uint32_t instance);

/*!
* @brief This function returns back the TSI driver from the low power to standard operation
*
* Function switch the driver back form low power mode and it can immediately change
*           the operation mode to any other or keep the driver in low power
*           configuration, to be able go back to low power state.
*
  @code
    // Switch the driver from the low power mode

    if(TSI_DRV_DisableLowPower(0, tsi_OpModeNormal) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't go from low power mode
    }


  @endcode
* @param instance   The TSI module instance.
* @param mode       The new operation mode request
* @return An error code or kStatus_TSI_Success.
*/
tsi_status_t TSI_DRV_DisableLowPower(const uint32_t instance, const tsi_modes_t mode);

/*!
* @brief This function do automatic recalibration of all important setting of TSI
*
* Function force the the driver to start the recalibration procedure
*           for current operation mode to get best possible TSI hardware settings.
*           The computed setting will be store into the operation mode data and can be
*           Load & Save by @ref TSI_DRV_LoadConfiguration or @ref TSI_DRV_SaveConfiguration
*           functions.
*
* @warning The function could take bigger amount of time (it cannot be specified
*           exact value) and is blocking.
*
  @code
    // Recalibrate current mode
    uint16_t signal;

    // Recalibrate the mode to get the best performance for this one electrode

    if(TSI_DRV_Recalibrate(0, &signal) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't recalibrate this mdoe
    }


  @endcode
* @param instance   The TSI module instance.
* @param lowestSignal       The pointer to variable where will be store the lowest signal of all electrodes
* @return An error code or kStatus_TSI_Success.
*/
tsi_status_t TSI_DRV_Recalibrate(const uint32_t instance, uint32_t * lowestSignal);

/*!
* @brief This function sets the callback function that is called when the measure cycle ends.
*
* Function set up or clear (parameter pFuncCallBack  = NULL), the callback function pointer
*           which will be called after each measure cycle ends. The user can also set own user data,
*           that will be handled by parameter to call back function (can be used to call one function by more sources).
*
  @code
    // Clear previous call back function

    if(TSI_DRV_SetCallBackFunc(0, NULL, NULL) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't set up the call back function at the momment
    }

    // Set new call back function

    if(TSI_DRV_SetCallBackFunc(0, myFunction, (void*)0x12345678) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't set up the call back function at the momment
    }


  @endcode
* @param instance       The TSI module instance.
* @param pFuncCallBack  The pointer to application call back function
* @param usrData        The user data pointer
* @return An error code or kStatus_TSI_Success.
*/
tsi_status_t TSI_DRV_SetCallBackFunc(const uint32_t instance, const tsi_callback_t pFuncCallBack, void * usrData);

/*!
* @brief This function change the current working operation mode.
*
* Function change the working operation mode of driver.
*
  @code
    // Change operation mode to low power

    if(TSI_DRV_ChangeMode(0, tsi_OpModeLowPower) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't change the operation mode into low power
    }

  @endcode
* @param instance       The TSI module instance.
* @param mode           The requested new operation mode
* @return An error code or kStatus_TSI_Success.
*/
tsi_status_t TSI_DRV_ChangeMode(const uint32_t instance, const tsi_modes_t mode);

/*!
* @brief This function returns the current working operation mode.
*
* Function returns the current working operation mode of driver.
*
  @code
    // Gets current operation mode of TSI driver
    tsi_modes_t mode;

    mode = TSI_DRV_GetMode(0);

  @endcode
* @param instance       The TSI module instance.
* @return An current operation mode of TSI driver.
*/
tsi_modes_t TSI_DRV_GetMode(const uint32_t instance);

/*!
* @brief This function load to TSI driver configuration for specific mode.
*
* Function loads the new configuration into specific mode.
*           This can be used when the calibrated data are store in any NVM
*           to load after startup of MCU, to avoid run recalibration what takes a
*           bigger amount of time.
*
  @code
    // Load operation mode configuration

    extern const tsi_operation_mode_t * myTsiNvmLowPowerConfiguration;

    if(TSI_DRV_LoadConfiguration(0, tsi_OpModeLowPower, myTsiNvmLowPowerConfiguration) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't load the configuration
    }

  @endcode
* @param instance       The TSI module instance.
* @param mode           The requested new operation mode
* @param operationMode  The pointer to storage place of the configuration that should be loaded
* @return An error code or kStatus_TSI_Success.
*/
tsi_status_t TSI_DRV_LoadConfiguration(const uint32_t instance, const tsi_modes_t mode, const tsi_operation_mode_t * operationMode);

/*!
* @brief This function save the TSI driver configuration for specific mode.
*
* Function saves the configuration of specific mode.
*           This can be used when the calibrated data should be stored in any backup memory
*           to load after startup of MCU, to avoid run recalibration what takes a
*           bigger amount of time.
*
  @code
    // Save operation mode configuration

    extern tsi_operation_mode_t  myTsiNvmLowPowerConfiguration;

    if(TSI_DRV_SaveConfiguration(0, tsi_OpModeLowPower, &myTsiNvmLowPowerConfiguration) != kStatus_TSI_Success)
    {
        // Error, the TSI driver can't save the configuration
    }

  @endcode
* @param instance       The TSI module instance.
* @param mode           The requested new operation mode
* @param operationMode  The pointer to storage place of the configuration that should be save
* @return An error code or kStatus_TSI_Success.
*/
tsi_status_t TSI_DRV_SaveConfiguration(const uint32_t instance, const tsi_modes_t mode, tsi_operation_mode_t * operationMode);
/* @} */

/*!
 * @name Interrupt
 * @{
 */


/* @} */

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* __FSL_TSI_DRIVER_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/

