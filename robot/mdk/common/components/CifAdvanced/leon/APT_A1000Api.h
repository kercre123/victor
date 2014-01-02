///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup APT_A1000Api Aptina A1000 sensor API
/// @{
/// @brief    Aptina A1000 sensor API.
///
/// This is the API for Aptina A1000 sensor library implementing all functions.
///

#include "CIFGenericApiDefines.h"
#include "CIFGenericPrivateDefines.h"

//Public Defines, Enums

#define VAPLAT_CAM1_RESETPIN 85
#define VAPLAT_CAM2_RESETPIN 12
#define MV172_CAM2_RESETPIN  12
#define MV172_CAM1_RESETPIN  64
#define VCPLAT_CAM2_RESETPIN 12
#define VCPLAT_CAM1_RESETPIN 51
typedef enum
{
    INC_EXP,
    DEC_EXP,
} ExpAdjustDir;

typedef enum
{
	VIDEO,
	TRIGGER,
} SensorMode;

static u32 trigger_fps[] = {
		16666,  // 60 fps
		33333,  // 30 fps
		50000,  // 20 fps
		100000, // 10 fps
		200000  //  5 fps
};

/// Function to update camera exposure
/// @param[in] hndl    Pointer to the sensor handle
/// @param[in] camSpec Pointer to the sensor spec
/// @param[in] dir     Adjustment direction
/// @return     Struct with frame specifications
///
void CifaUpdateExposure(SensorHandle* hndl, CamSpec* camSpec, ExpAdjustDir dir);

/// Function to to set a specific exposure
/// @param[in] hndl    Pointer to the sensor handle
/// @param[in] camSpec Pointer to the sensor spec
/// @param[in] value   Exposure value
/// @return     Struct with frame specifications
///
void CifaSetExposure(SensorHandle* hndl, CamSpec* camSpec, int value);

/// Function to set flip/rotate
/// @param[in] hndl    Pointer to the sensor handle
/// @param[in] camSpec Pointer to the sensor spec
/// @param[in] setting Setting: 00,01,10,11
/// @return     Struct with frame specifications

void CifaFlipMirror(SensorHandle* hndl, CamSpec* camSpec, int setting);

/// Function to update camera exposure
/// @param[in] hndl    Pointer to the sensor handle
/// @param[in] camSpec Pointer to the sensor spec
/// @return     Struct with frame specifications
///
void CifaGetSensorStats(SensorHandle* hndl, CamSpec* camSpec);

/// Function to update camera mode
/// @param[in] hndl         Pointer to the sensor handle
/// @param[in] camSpec      Pointer to the sensor spec
/// @param[in] sensor_mode  New camera mode
/// @return     Current Camera Mode
///
u32 CifaSwitchCamMode(SensorHandle* hndl, CamSpec* camSpec, SensorMode sensor_mode);

/// Function to init Trigger timer
/// @param[in] trigger_fps_idx  FPS index
/// @return     Timer ID
int CifaInitCameraAutoTrigger(int trigger_fps_idx);

/// Function to init Trigger timer
/// @param[in] timerNr          Current timer ID
/// @param[in] trigger_fps_idx  FPS index
/// @return     Timer ID
int CifaUpdateCameraAutoTrigger(int timerNr,int trigger_fps_idx);

/// Function to Trigger Camera Manually
/// @return
void CifaTriggerCamManual();

/// Function to trigger camera standby for AR0330
/// @param[in] hndl     Pointer to the sensor handle
/// @param[in] camSpec  Pointer to the sensor spec
/// @return
void CifaStandbyAR0330(SensorHandle* hndl, CamSpec* camSpec);

/// Function to trigger camera resume for AR0330
/// @param[in] hndl     Pointer to the sensor handle
/// @param[in] camSpec  Pointer to the sensor spec
/// @return
void CifaResumeAR0330(SensorHandle* hndl, CamSpec* camSpec);

/// Function to trigger camera standby for A1000
/// @param[in] hndl    Pointer to the sensor handle
/// @param[in] camSpec Pointer to the sensor spec
/// @return
void CifaStandbyA1000(SensorHandle* hndl, CamSpec* camSpec);

/// Function to trigger camera resume for A1000
/// @param[in] hndl    Pointer to the sensor handle
/// @param[in] camSpec Pointer to the sensor spec
/// @return
void CifaResumeA1000(SensorHandle* hndl, CamSpec* camSpec);
/// @}