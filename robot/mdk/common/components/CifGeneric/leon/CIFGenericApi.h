///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup CIFGenericApi CIF sensor API.
/// @{
/// @brief    CIF sensor API.
///
/// This is the API for CIF sensor library implementing all configurations.
///


#ifndef _CIF_GENERIC_API_H_
#define _CIF_GENERIC_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include "CIFGenericApiDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------
// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

/// Resets the Cif interface
/// @param  hndl Pointer to the sensor handle
/// @return     void
///
void CifStop (SensorHandle *hndl);

/// This will initialize the CIF interface
/// @param  hndl               Pointer to the sensor handle
/// @param  camSpec            Camera configuration
/// @param  fpSp               Frame spec
/// @param  pI2cHandle         I2C handle, NULL is no I2C configuration required
/// @param  sensorNumber       0 for left and 1 for right camera
/// @return     1 if sensor started and -1 if not
///
int CifInit(SensorHandle *hndl, CamSpec *camSpec, frameSpec *fpSp, I2CM_Device *pI2cHandle, unsigned int sensorNumber);

/// This will start the CIF interface
/// @param  hndl     Pointer to the sensor handle
/// @param  resetpin The reset pin
/// @return     1 if sensor started and -1 if not
///
int CifStart(SensorHandle *hndl, int resetpin);

/// Get frame counter
/// @param  hndl     Pointer to the sensor handle
/// @return     Frame number
///
unsigned int CifGetFrameCounter (SensorHandle *hndl);

/// Set callbacks for assign new frame and frame ready.
/// @param  hndl          Pointer to the sensor handle
/// @param  getFrame      Function pointer to get new frame or NULL if not needed
/// @param  frameReady    Function pointer for frame ready or NULL if not needed
/// @param  getBlock      Function pointer to get new block or NULL if not needed
/// @param  blockReady    Function pointer for block ready or NULL if not needed
/// @return     void
///
void CifSetupCallbacks(SensorHandle* hndl, allocateFn*  getFrame, frameReadyFn* frameReady, allocateBlockFn*  getBlock, blockReadyFn* blockReady);


/// Enables callback for EOF
/// @param  hndl          Pointer to the sensor handle
/// @param  callbackEOF   Function pointer to EOF callback
/// @return     void
///
void CifEnableEOFCallback(SensorHandle* hndl, EOFFn* callbackEOF);

/// Function to get current camera frame specification
/// @param  hndl          Pointer to the sensor handle
/// @
/// @return     Struct with frame specifications
///
const frameSpec* CifGetFrameSpec(SensorHandle* hndl);

/// Generate Hsync and Vsync clock signals for the specified sensor
/// @param hndl Sensor handle
/// @param HSW Hsync rate
/// @param VSW Vsync rate
void camSim(SensorHandle* hndl,u32 HSW,u32 VSW);
/// @}
#endif //_CIF_GENERIC_API_H_
