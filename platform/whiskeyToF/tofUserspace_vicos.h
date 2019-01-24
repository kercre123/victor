
#ifndef __tof_userspace_vicos_h__
#define __tof_userspace_vicos_h__

#include "whiskeyToF/vicos/vl53l1/platform/inc/vl53l1_platform_user_data.h"

int open_dev(VL53L1_Dev_t* dev);
int close_dev(VL53L1_Dev_t* dev);
int setup(VL53L1_Dev_t* dev);
int setup_roi_grid(VL53L1_Dev_t* dev, const int rows, const int cols);
int get_mz_data(VL53L1_Dev_t* dev, const int blocking, VL53L1_MultiRangingData_t *data);
int start_ranging(VL53L1_Dev_t* dev);
int stop_ranging(VL53L1_Dev_t* dev);

#endif
