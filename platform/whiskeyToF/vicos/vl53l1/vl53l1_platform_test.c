#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "vl53l1_api.h"
#include "vl53l1_platform_init.h"

#define IDENTIFICATION__MODEL_ID_ADR 0x010F
#define IDENTIFICATION__MODEL_ID_VAL 0xEA

#define SPARE_ADR 0x64

int rd_write_verification(VL53L1_DEV Dev, uint16_t addr, uint32_t expected_value)
{
	uint8_t bytes[4],mbytes[4];
	uint16_t words[2];
	uint32_t dword;
	int i;

	VL53L1_ReadMulti(Dev, addr, mbytes, 4);
	for (i=0; i<4; i++){ VL53L1_RdByte(Dev, addr+i, &bytes[i]); }
	for (i=0; i<2; i++){ VL53L1_RdWord(Dev, addr+i*2, &words[i]); }
	VL53L1_RdDWord(Dev, addr, &dword);

	printf("expected   = %8x,\n",expected_value);
	printf("read_multi = %2x, %2x, %2x, %2x\n", mbytes[0],mbytes[1],mbytes[2],mbytes[3]);
	printf("read_bytes = %2x, %2x, %2x, %2x\n", bytes[0],bytes[1],bytes[2],bytes[3]);
	printf("read words = %4x, %4x\n",words[0],words[1]);
	printf("read dword = %8x\n",dword);

	if((mbytes[0]<<24 | mbytes[1]<<16 | mbytes[2]<<8 | mbytes[3]) != expected_value) return (-1);
	if((bytes[0]<<24 | bytes[1]<<16 | bytes[2]<<8 | bytes[3]) != expected_value) return (-1);
	if((words[0]<<16 | words[1]) != expected_value) return (-1);
	if(dword != expected_value) return(-1);

	return(0);

}

void i2c_test(VL53L1_DEV Dev)
{
	int err_count = 0;
	int expected_value = 0;

	uint8_t buff[4] = {0x11,0x22,0x33,0x44};
	uint8_t ChipID[4];
	int i=0;

	for (i=0; i<4; i++){ VL53L1_RdByte(Dev, (uint16_t)IDENTIFICATION__MODEL_ID_ADR+i, &ChipID[i]); }
#ifdef BIGENDIAN
	expected_value = ChipID[3]<<24 | ChipID[2]<<16 | ChipID[1]<<8 | ChipID[0];
#else
	expected_value = ChipID[0]<<24 | ChipID[1]<<16 | ChipID[2]<<8 | ChipID[3];
#endif

	if(rd_write_verification(Dev, (uint16_t)IDENTIFICATION__MODEL_ID_ADR, expected_value) <0) err_count++;	// check the chip ID

	VL53L1_WriteMulti(Dev, SPARE_ADR,  buff, 4);				// check WriteMulti
	if(rd_write_verification(Dev, SPARE_ADR, 0x11223344) <0) err_count++;

	VL53L1_WrDWord(Dev, SPARE_ADR, 0xffeeddcc);				// check WrDWord
	if(rd_write_verification(Dev, SPARE_ADR, 0xffeeddcc) <0) err_count++;


	VL53L1_WrWord(Dev, SPARE_ADR, 0x5566);					// check WrWord
	VL53L1_WrWord(Dev, SPARE_ADR+2, 0x7788);
	if(rd_write_verification(Dev, SPARE_ADR, 0x55667788) <0) err_count++;


	for (i=0; i<4; i++){ VL53L1_WrByte (Dev, SPARE_ADR+i, buff[i]); }
	if(rd_write_verification(Dev, SPARE_ADR,0x11223344) <0) err_count++;

	if(err_count>0)
	{
		printf("i2c test failed - please check it\n");
	} else {
		printf("i2c test succeeded !\n");
	}
}

int main(int argc, char **argv)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;
	VL53L1_Dev_t                   dev;
	VL53L1_DEV                     Dev = &dev;
	VL53L1_PresetModes             PresetMode;
	VL53L1_DeviceInfo_t            DeviceInfo;

//	setvbuf(stdout, NULL, _IONBF, 0);
//	setvbuf(stderr, NULL, _IONBF, 0);


	/*
	* Initialize the platform interface
	*/

	dev.platform_data.i2c_file_handle = open("/dev/i2c-6", O_RDWR);

	if (Status == VL53L1_ERROR_NONE)
	Status = VL53L1_platform_init(
		Dev,
		0x29,
		1,
		400);       /* comms_speed_khz - 400kHz recommended */

	/*
	* Wait 2 sec for supplies to stabilize
	*/

	if (Status == VL53L1_ERROR_NONE)
		Status = VL53L1_WaitMs(Dev, 500);

	/*
	*  Wait for firmware to finish booting
	*/
	if (Status == VL53L1_ERROR_NONE)
		Status = VL53L1_WaitDeviceBooted(Dev);

	if (Status == VL53L1_ERROR_NONE) {
		printf("Test of platform.c adaptation\n");
		i2c_test(Dev);
	}

	return (Status);
}




/*
 * vl53l1_platform_test.c
 *
 *  Created on: 16 janv. 2018
 *      Author: taloudpy
 */

#include "vl53l1_platform.h"

VL53L1_Dev_t MyDevice;
