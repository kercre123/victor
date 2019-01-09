#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "vl53l1_api.h"
#include "vl53l1_platform_init.h"

#define trace_print(level, ...) \
	VL53L1_trace_print_module_function(VL53L1_TRACE_MODULE_CORE, \
	level, VL53L1_TRACE_FUNCTION_NONE, ##__VA_ARGS__)

void print_pal_error(VL53L1_Error Status){
	char buf[VL53L1_MAX_STRING_LENGTH];
	VL53L1_GetPalErrorString(Status, buf);
	printf("API Status: %i : %s\n", Status, buf);
}

void print_multiranging_data(
		int i, int j,
		VL53L1_TargetRangeData_t *pRangeData) {

	printf("%d: SignalRateRtnMegaCps[%d]= %f\n",
			i, j, pRangeData->SignalRateRtnMegaCps/65536.0);
	printf("%d: AmbientRateRtnMegaCps[%d]= %f\n",
			i, j, pRangeData->AmbientRateRtnMegaCps/65536.0);
	printf("%d: SigmaMilliMeter[%d]= %f\n",
			i, j, pRangeData->SigmaMilliMeter/65536.0);
	printf("%d: RangeMilliMeter[%d]= %d\n",
			i, j, pRangeData->RangeMilliMeter);
	printf("%d: RangeMinMilliMeter[%d]= %d\n",
			i, j, pRangeData->RangeMinMilliMeter);
	printf("%d: RangeMaxMilliMeter[%d]= %d\n",
			i, j, pRangeData->RangeMaxMilliMeter);
	printf("%d: RangeStatus[%d]= %d\n",
			i, j, pRangeData->RangeStatus);
}

int  RunRangingLoop(VL53L1_DEV Dev, int  no_of_measurements){
	int Status = VL53L1_ERROR_NONE;
	int i,j,k;
	int no_of_object_found;
	VL53L1_TargetRangeData_t *pRangeData;
	VL53L1_MultiRangingData_t MultiRangingData;
	VL53L1_MultiRangingData_t *pMultiRangingData = &MultiRangingData;

	if (Status == VL53L1_ERROR_NONE) {
		printf("run VL53L1_StartMeasurement\n");
		Status = VL53L1_StartMeasurement(Dev);
	}
	if( Status != VL53L1_ERROR_NONE){
		printf("fail to StartMeasurement\n");
		return -1;
	}

	for (i = 0 ; i < no_of_measurements ; i++) {

		/* Wait for range completion */
		if (Status == VL53L1_ERROR_NONE)
			Status = VL53L1_WaitMeasurementDataReady(Dev);

		if(Status == VL53L1_ERROR_NONE)
		{
			Status = VL53L1_GetMultiRangingData(Dev, pMultiRangingData);

			if (Status == VL53L1_ERROR_NONE)
				VL53L1_ClearInterruptAndStartMeasurement(Dev);

			no_of_object_found = pMultiRangingData->NumberOfObjectsFound;
			printf("Number of measurements = %d\n",i);
			printf("Number of Objects Found = %d\n",no_of_object_found);
			printf("%d: RoiNumber= %d\n",
					i, pMultiRangingData->RoiNumber);
			printf("%d: RoiStatus= %d\n",
					i, pMultiRangingData->RoiStatus);
			printf("%d: EffectiveSpadRtnCount= %d\n",
					i, pMultiRangingData->EffectiveSpadRtnCount);

			if (no_of_object_found <=1)
				k = 1;
			else
				k = no_of_object_found;

			for(j=0;j<k;j++) {
				pRangeData = &(pMultiRangingData->RangeData[j]);
				printf("RangingMeasurementData[%d]\n",j);

				printf("%d: Stream Count[%d]= %d\n",
						i, j, pMultiRangingData->StreamCount);
				print_multiranging_data(i, j, pRangeData);

			}
			printf("\n");
		} else {
			break;
		}
	}

	if (Status == VL53L1_ERROR_NONE) {
		printf("run VL53L1_StopMeasurement\n");
		Status = VL53L1_StopMeasurement(Dev);
	}

	return Status;
}


VL53L1_Error SetROIExample(VL53L1_DEV Dev)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;
	VL53L1_RoiConfig_t RoiConfig;
	uint8_t MaxNumberOfROI;

	Status = VL53L1_GetMaxNumberOfROI(Dev, &MaxNumberOfROI);
	printf("MaxNumberOfROI : %d\n", MaxNumberOfROI);


	if (Status == VL53L1_ERROR_NONE) {
		RoiConfig.NumberOfRoi = 3;
		RoiConfig.UserRois[0].TopLeftX = 3;
		RoiConfig.UserRois[0].TopLeftY = 10;
		RoiConfig.UserRois[0].BotRightX = 10;
		RoiConfig.UserRois[0].BotRightY = 3;
		RoiConfig.UserRois[1].TopLeftX = 5;
		RoiConfig.UserRois[1].TopLeftY = 12;
		RoiConfig.UserRois[1].BotRightX = 12;
		RoiConfig.UserRois[1].BotRightY = 5;
		RoiConfig.UserRois[2].TopLeftX = 6;
		RoiConfig.UserRois[2].TopLeftY = 13;
		RoiConfig.UserRois[2].BotRightX = 13;
		RoiConfig.UserRois[2].BotRightY = 6;

		Status = VL53L1_SetROI(Dev, &RoiConfig);
	}

	return Status;

}


VL53L1_Error TimingBudgetExample(VL53L1_DEV Dev)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;

	uint32_t MeasurementTimingBudgetMicroSeconds = 12000;

	Status = VL53L1_GetMeasurementTimingBudgetMicroSeconds(Dev,
			&MeasurementTimingBudgetMicroSeconds);
	printf("Timing Budget is : %d us\n", MeasurementTimingBudgetMicroSeconds);

	if (Status == VL53L1_ERROR_NONE) {
		Status = VL53L1_SetMeasurementTimingBudgetMicroSeconds(Dev,
				MeasurementTimingBudgetMicroSeconds + 5000);
	}

	if (Status == VL53L1_ERROR_NONE) {
		Status = VL53L1_GetMeasurementTimingBudgetMicroSeconds(Dev,
				&MeasurementTimingBudgetMicroSeconds);
		printf("New Timing Budget is : %d us\n", MeasurementTimingBudgetMicroSeconds);
	}

	return Status;

}

int main(int argc, char **argv)
{
	VL53L1_Error Status = VL53L1_ERROR_NONE;
	VL53L1_Dev_t                   dev;
	VL53L1_DEV                     Dev = &dev;
	VL53L1_PresetModes             PresetMode;
	VL53L1_DeviceInfo_t            DeviceInfo;

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	printf ("VL53L1 Multi Zones Scanning example\n\n");
	SUPPRESS_UNUSED_WARNING(argc);
	SUPPRESS_UNUSED_WARNING(argv);

//	printf ("Press a Key to continue!\n\n");
//	getchar();

	/*
	* Configure logging - turn everything on
	*/

#ifdef VL53L1_LOG_ENABLE
	if (0)
		Status = VL53L1_trace_config(
			NULL,
			VL53L1_TRACE_MODULE_ALL,
			VL53L1_TRACE_LEVEL_ALL,
			VL53L1_TRACE_FUNCTION_ALL);
	else
		Status = VL53L1_trace_config(
			NULL,
			VL53L1_TRACE_MODULE_NONE,
			VL53L1_TRACE_LEVEL_NONE,
			VL53L1_TRACE_FUNCTION_NONE);

#endif

	/*
	* Initialize the platform interface
	*/

	dev.platform_data.i2c_file_handle = open("/dev/i2c-6", O_RDWR);

	if (Status == VL53L1_ERROR_NONE) {
		printf("Platform init\n");
		Status = VL53L1_platform_init(
			Dev,
			0x29,
			1, /* comms_type  I2C*/
			400);       /* comms_speed_khz - 400kHz recommended */
	}

	/*
	* Wait 2 sec for supplies to stabilize
	*/

	if (Status == VL53L1_ERROR_NONE) {
		printf("WaitMs\n");
		Status = VL53L1_WaitMs(Dev, 2000);
	}
	/*
	*  Wait for firmware to finish booting
	*/
	if (Status == VL53L1_ERROR_NONE) {
		printf("Wait device booted\n");
		Status = VL53L1_WaitDeviceBooted(Dev);
	}
	/*
	* Initialise Dev data structure
	*/
	if (Status == VL53L1_ERROR_NONE) {
		printf("Data init\n");
		Status = VL53L1_DataInit(Dev);
	}
	if(Status == VL53L1_ERROR_NONE)
	{
		Status = VL53L1_GetDeviceInfo(Dev, &DeviceInfo);
		printf("Get device info\n");
		if(Status == VL53L1_ERROR_NONE)
		{
		    printf("VL53L1_GetDeviceInfo:\n");
		    printf("Device Name : %s\n", DeviceInfo.Name);
		    printf("Device Type : %s\n", DeviceInfo.Type);
		    printf("Device ID : %s\n", DeviceInfo.ProductId);
		    printf("ProductRevisionMajor : %d\n", DeviceInfo.ProductRevisionMajor);
		    printf("ProductRevisionMinor : %d\n", DeviceInfo.ProductRevisionMinor);

		    if ((DeviceInfo.ProductRevisionMajor != 1) || (DeviceInfo.ProductRevisionMinor != 1)) {
			printf("Warning expected cut 1.1 but found cut %d.%d\n",
					DeviceInfo.ProductRevisionMajor, DeviceInfo.ProductRevisionMinor);
		    }
		}
		print_pal_error(Status);
	}

	if (Status == VL53L1_ERROR_NONE) {
		printf("Static init\n");
		Status = VL53L1_StaticInit(Dev);
	}
	/*
	* Run reference SPAD characterisation
	*/
	if (Status == VL53L1_ERROR_NONE) {
		printf("Perform spad management\n");
		Status = VL53L1_PerformRefSpadManagement(Dev);
	}

	/*
	* Run Xtalk calibration
	*/
//	if (Status == VL53L1_ERROR_NONE)
//		Status = VL53L1_PerformXTalkCalibration(Dev, CalibrationOption);


	/*
	* Initialize configuration data structures for the
	* given preset mode. Does *not* apply the settings
	* to the device just initializes the data structures
	*/

	if (Status == VL53L1_ERROR_NONE) {
		printf("Set Preset mode\n");
		PresetMode = VL53L1_PRESETMODE_MULTIZONES_SCANNING;
		Status = VL53L1_SetPresetMode(Dev, PresetMode);
	}

	if (Status == VL53L1_ERROR_NONE) {
		printf("set distance mode\n");
//		Status = VL53L1_SetDistanceMode(Dev, VL53L1_DISTANCEMODE_AUTO);
//		Status = VL53L1_SetDistanceMode(Dev, VL53L1_DISTANCEMODE_MEDIUM);
//		Status = VL53L1_SetDistanceMode(Dev, VL53L1_DISTANCEMODE_SHORT);
		Status = VL53L1_SetDistanceMode(Dev, VL53L1_DISTANCEMODE_LONG);
	}

	/*
	 * Set ROI before start
	*
	*/
	if (Status == VL53L1_ERROR_NONE) {
		printf("Set ROI\n");
		Status = SetROIExample(Dev);
	}

	/* Example for timing budget before start */
	if (Status == VL53L1_ERROR_NONE) {
		printf("Set timing budget\n");
		Status = TimingBudgetExample(Dev);
	}
	/*
	 * Ranging LOOP
	 *
	 * Run two times the Ranging loop to test start stop
	*
	*/
	/* The following ranging loop will use Vl53L1_GetMultiRangingData*/
	if (Status == VL53L1_ERROR_NONE) {
		printf("*********************************************\n");
		printf("    RUN first RunRangingLoop\n");
		printf("*********************************************\n");
		Status = RunRangingLoop(Dev, 20);
	}
	/* The following ranging loop will use Vl53L1_GetMultiRangingData*/
	if (Status == VL53L1_ERROR_NONE) {
		printf("*********************************************\n");
		printf("    RUN second RunRangingLoop\n");
		printf("*********************************************\n");
		Status = RunRangingLoop(Dev, 20);
	}


	if (Status == VL53L1_ERROR_NONE) {
		printf("Platform terminate\n");
		Status = VL53L1_platform_terminate(Dev);
	}

	print_pal_error(Status);

//	printf ("\nPress a Key to continue!");
//	getchar();

	return (Status);
}
