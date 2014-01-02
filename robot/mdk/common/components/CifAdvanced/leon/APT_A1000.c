///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Aptina A1000 Sensor Functions.
///
/// This is the implementation of Aptina A1000 Sensor library.
///
///

// 1: Includes
// ----------------------------------------------------------------------------
// Drivers includes
#include "APT_A1000Api.h"
#include "assert.h"
#include "DrvTimer.h"
#include "DrvGpio.h"
#include "DrvI2cMaster.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

//Register Address Defines
#define REGADR_RESET     0x301A
#define REGADR_STAT_AGDG 0x312A
#define REGADR_STAT_IT   0x3164
#define REGADR_COARSE_IT 0x3012
#define REGADR_FINE_IT   0x3014
#define REGADR_STAT_FE   0x30AC
#define REGADR_GLOBAL_GA 0x305E
#define REGADR_READ_MODE 0x3040

//Register Value Defines
#define TRIGGER_MODE	0x1BD8
#define VIDEO_MODE		0x1ADC

//Other Defines
#define EXPOSURE_STEP 8 // 445 works as Anti banding; else set 1

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
// 4: Static Local Data
// ----------------------------------------------------------------------------

u32 stats_CIT; //coarse integration time
u32 stats_GGA; //global gain

static int fps_idx;

static u8 camWriteProto[] = I2C_PROTO_WRITE_16BA;
static u8 camReadProto[]  = I2C_PROTO_READ_16BA;

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
u32 CifaTriggerCam();

// 6: Functions Implementation
// ----------------------------------------------------------------------------

//Collect Sensor Statistics
void CifaGetSensorStats(SensorHandle* hndl, CamSpec* camSpec)
{
	unsigned char readByte[2];

	DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_COARSE_IT ,camReadProto, readByte , 2);
	stats_CIT = (readByte[0] << 8) | readByte[1];
	DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_GLOBAL_GA ,camReadProto, readByte , 2);
	stats_GGA = (readByte[0] << 8) | readByte[1];
}

//Clamp Exposure to "legal" values
u32 CifaClampExposure(u32 value)
{
	if(value>=60000) value=0; // because of u32
	else
	if(value>=1780)  value=1780;
	return value;
}

//Update Exposure
void CifaUpdateExposure(SensorHandle* hndl, CamSpec* camSpec, ExpAdjustDir dir)
{
	unsigned char writeByte[2];
	u32 new_exposure;
	CifaGetSensorStats(hndl,camSpec);

	switch(dir)
	{
	case 0: writeByte[0] = CifaClampExposure(stats_CIT+EXPOSURE_STEP) >> 8;
			writeByte[1] = CifaClampExposure(stats_CIT+EXPOSURE_STEP) & 0xffff;
			DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_COARSE_IT ,camWriteProto, writeByte , 2);
			break;
	case 1: writeByte[0] = CifaClampExposure(stats_CIT-EXPOSURE_STEP) >> 8;
			writeByte[1] = CifaClampExposure(stats_CIT-EXPOSURE_STEP) & 0xffff;
			DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_COARSE_IT ,camWriteProto, writeByte , 2);
			break;
	default: break;
	}

	new_exposure = (writeByte[0] << 8) | writeByte[1];
}

//Set Exposure
void CifaSetExposure(SensorHandle* hndl, CamSpec* camSpec, int value)
{
	unsigned char writeByte[2];
	//@@@@ IMPORTANT NOTE HERE
	//The term "exposure" isnt realy correct here, because Exposure consists of Integration time + Gain, and this function sets Integration Time only, for now
	//!!!!!!!!!!!Analog GAin: To be added later

    writeByte[0] = CifaClampExposure(value) >> 8;
    writeByte[1] = CifaClampExposure(value) & 0xffff;
    DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_COARSE_IT ,camWriteProto, writeByte , 2);
}

//Set flip or mirror
void CifaFlipMirror(SensorHandle* hndl, CamSpec* camSpec, int setting)
{
	unsigned char writeByte[2];
	unsigned char readByte[2];
	int value;

	DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_RESET ,camReadProto, readByte , 2);
	readByte[1] = readByte[1] & 0xFB; // clear bit 2
	DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_RESET ,camWriteProto, readByte , 2);
	readByte[1] = readByte[1] | 0x4; // set back bit 2

	switch(setting)
	{
	default:
	case 0x0: value=0x0; break; //none
	case 0x1: value=0x8000; break; //only flip
	case 0x2: value=0x4000; break; //only mirror
	case 0x3: value=0xC000; break; //flip+mirror
	};

    writeByte[0] = value >> 8;
    writeByte[1] = value & 0xffff;
    DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_READ_MODE ,camWriteProto, writeByte , 2);
    DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_RESET ,camWriteProto, readByte , 2);
}

//Switch Sensor Mode (Video or Trigger)
u32 CifaSwitchCamMode(SensorHandle* hndl, CamSpec* camSpec, SensorMode sensor_mode)
{
unsigned char writeByte[2];

	switch(sensor_mode)
		{
		case 0: writeByte[0] = (unsigned char)(VIDEO_MODE >> 8);
				writeByte[1] = (unsigned char)(VIDEO_MODE & 0xffff);
				DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_RESET ,camWriteProto, writeByte , 2);
				sensor_mode = 0;
				break;
		case 1: writeByte[0] = (unsigned char)(TRIGGER_MODE >> 8);
				writeByte[1] = (unsigned char)(TRIGGER_MODE & 0xffff);
				DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_RESET ,camWriteProto, writeByte , 2);
				sensor_mode = 1;
				break;
		};
return sensor_mode;
}

//Camera Trigger timer init
int CifaInitCameraAutoTrigger(int trigger_fps_idx)
{
	DrvTimerInit();
	int timerNr = DrvTimerCallAfterMicro(trigger_fps[trigger_fps_idx]-1000, CifaTriggerCam, 0, 1);
	fps_idx = trigger_fps_idx;
	return timerNr;
}

//Update trigger
int CifaUpdateCameraAutoTrigger(int timerNr,int trigger_fps_idx)
{
	DrvTimerDisable(timerNr);
	DrvTimerInit();
	timerNr = DrvTimerCallAfterMicro(trigger_fps[trigger_fps_idx]-1000, CifaTriggerCam, 0, 1);
	fps_idx = trigger_fps_idx;
	return timerNr;
}

//Trigger timer callback
u32 CifaTriggerCam()
{
	DrvGpioSetPinHi(20);
	DrvGpioSetPinHi(18);
	if(fps_idx != 0)
	{
		SleepMs(1);
		DrvGpioSetPinLo(20);
		DrvGpioSetPinLo(18);
	};
	return 0;
}

//Trigger cam in manual mode
void CifaTriggerCamManual()
{
	DrvGpioSetPinHi(20);
	DrvGpioSetPinHi(18);
	SleepMs(20);
	DrvGpioSetPinLo(20);
	DrvGpioSetPinLo(18);
}

//Trigger camera standby for AR0330
void CifaStandbyAR0330(SensorHandle* hndl, CamSpec* camSpec)
{
	unsigned char writeByte[2];

	DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_RESET ,camReadProto, writeByte , 2);
	writeByte[1] = writeByte[1] & 0x00FB;
	DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_RESET ,camWriteProto, writeByte , 2);
}

//Trigger camera resume for AR0330
void CifaResumeAR0330(SensorHandle* hndl, CamSpec* camSpec)
{
	unsigned char writeByte[2];

	DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_RESET ,camReadProto, writeByte , 2);
	writeByte[1] = writeByte[1] | 0x0004;
	DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_RESET ,camWriteProto, writeByte , 2);
}

//Trigger camera standby for AR0330
void CifaStandbyA1000(SensorHandle* hndl, CamSpec* camSpec)
{
    // TODO: hard-standby ... below does not work for trigger mode (already in low power mode)
//	unsigned char writeByte[2];
//
//	DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_RESET ,camReadProto, writeByte , 2);
//	writeByte[1] = writeByte[1] & 0x00FB;
//	DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_RESET ,camWriteProto, writeByte , 2);
}

//Trigger camera resume for AR0330
void CifaResumeA1000(SensorHandle* hndl, CamSpec* camSpec)
{
    // TODO: hard-standby ... below does not work for trigger mode (already in low power mode)
//	unsigned char writeByte[2];
//
//	DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_RESET ,camReadProto, writeByte , 2);
//	writeByte[1] = writeByte[1] | 0x0004;
//	DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, REGADR_RESET ,camWriteProto, writeByte , 2);
}
