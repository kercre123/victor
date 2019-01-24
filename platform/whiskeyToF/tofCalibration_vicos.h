
#ifndef __tof_calibration_vicos_h__
#define __tof_calibration_vicos_h__

#include "whiskeyToF/vicos/vl53l1/platform/inc/vl53l1_platform_user_data.h"

#include <string>

int perform_calibration(VL53L1_Dev_t* dev, uint32_t dist_mm, float reflectance);
int load_calibration(VL53L1_Dev_t* dev);
void set_calibration_save_path(const std::string& path);

#endif
