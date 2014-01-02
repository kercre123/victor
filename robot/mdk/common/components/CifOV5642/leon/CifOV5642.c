/*
 * CifOV5642.c
 *
 *  Created on: Jul 28, 2013
 *      Author: alseh
 */

#include "CifOV5642Api.h"
#include "CifOV5642PrivateDefines.h"
#include "DrvI2cMaster.h"
//Omnivision specific registers.
//Need to remove these somehow from CifGeneric
#define TIMING_TC_CONTROL_REG18 0x3818
#define ARRAY_CONTROL_01 0x3621

static u8 camWriteProto[] = I2C_PROTO_WRITE_16BA;

void CIFOV5642_CODE_SECTION(CifOV5642FlipMirror) (SensorHandle* hndl, CamSpec* camSpec, int sensorFlipMirror)
{
	unsigned int i;
	unsigned char nextByte[2];

	if (camSpec->regValues == NULL)
	{
		return;
	}

	for (i = 0; i < camSpec->regNumber; i++)
	{
		
		nextByte[0] = camSpec->regValues[i][1];
		
		if (camSpec->regValues[i][0] == TIMING_TC_CONTROL_REG18)
		{
			switch(sensorFlipMirror)
			{
			case 0: //flip
			{
				nextByte[0] = (nextByte[0] & 0x0F) | 0xA0;
				break;
			}
			case 1: //mirror
			{
				nextByte[0] = (nextByte[0] & 0x0F) | 0xC0;
				break;
			}
			default: break; // leave as is
			}

			DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, camSpec->regValues[i][0] ,camWriteProto, nextByte , 1);

		}
		else if (camSpec->regValues[i][0] == ARRAY_CONTROL_01)
		{
			switch(sensorFlipMirror)
			{
			case 0: //flip
			{
				nextByte[0] = nextByte[0] | 0x20;
				break;
			}
			case 1: //mirror
			{
				nextByte[0] = nextByte[0] & (~0x20);
				break;
			}
			default: break; // leave as is
			}

			DrvI2cMTransaction(hndl->pI2cHandle, camSpec->sensorI2CAddress, camSpec->regValues[i][0] ,camWriteProto, nextByte , 1);

		}

	}

}
