/**
 * File: tof_vicos.cpp
 *
 * Author: Al Chaussee
 * Created: 10/18/2018
 *
 * Description: Defines interface to a some number(2) of tof sensors
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "whiskeyToF/tof.h"

#include "whiskeyToF/vicos/vl53l1_tools/inc/vl53l1_def.h"
#include "whiskeyToF/vicos/vl53l1_tools/inc/stmvl53l1_if.h"
#include "whiskeyToF/vicos/vl53l1_tools/inc/stmvl53l1_internal_if.h"

#include "util/console/consoleInterface.h"
#include "util/console/consoleSystem.h"
#include "util/logging/logging.h"

#include <sys/ioctl.h>
#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <thread>
#include <mutex>

#ifdef SIMULATOR
#error SIMULATOR should be defined by any target using tof_vicos.cpp
#endif

#define SPAD_COLS (16)  ///< What's in the sensor
#define SPAD_ROWS (16)  ///< What's in the sensor
#define SPAD_MIN_ROI (4) ///< Minimum ROI size in spads
#define MAX_ROWS (SPAD_ROWS / SPAD_MIN_ROI)
#define MAX_COLS (SPAD_COLS / SPAD_MIN_ROI)

#define error(fmt, ...)  fprintf(stderr, "[E] " fmt"\n", ##__VA_ARGS__)
#define warn(fmt, ...)  fprintf(stderr, "[W] " fmt"\n", ##__VA_ARGS__)


namespace Anki {
namespace Vector {

namespace {
  int tof_fd = -1;

std::thread _processor;
std::mutex _mutex;
bool _stopProcessing = false;

RangeDataRaw _latestData;

CONSOLE_VAR(bool, kStartToF, "ToF", false);
}

ToFSensor* ToFSensor::_instance = nullptr;

ToFSensor* ToFSensor::getInstance()
{
  if(nullptr == _instance)
  {
    _instance = new ToFSensor();
  }
  return _instance;
}

void ToFSensor::removeInstance()
{
  if(_instance != nullptr)
  {
    delete _instance;
    _instance = nullptr;
  }
}

int open_dev(const int dev_no)
{
  char dev_fi_name[256];
  if (dev_no == 0) {
    strcpy(dev_fi_name, "/dev/" VL53L1_MISC_DEV_NAME);
  }
  else {
    snprintf(dev_fi_name, sizeof(dev_fi_name), "%s%d","/dev/" VL53L1_MISC_DEV_NAME, dev_no);
  }

  return open(dev_fi_name ,O_RDWR);
}

/// Stop all ranging activity
int stop_ranging(const int dev_no) {
  const int rc = ioctl(dev_no, VL53L1_IOCTL_STOP, NULL);
  if (errno == EBUSY) {
    warn("already stopped");
    return 0;
  }
  return rc;
}

/// Start ranging
int start_ranging(const int dev) {
  int rc = ioctl(dev, VL53L1_IOCTL_START, NULL);
  if (errno == EBUSY) {
    warn("Already started or calibrating");
    return 0;
  }
  return rc;
}

/// Change the preset mode of the device
int preset_mode_set(const int dev, const int mode) {
  struct stmvl53l1_parameter params;
  params.is_read = 0;
  params.name = VL53L1_DEVICEMODE_PAR;
  params.value = mode;
  return ioctl(dev, VL53L1_IOCTL_PARAMETER, &params);
}

/// Set the timing budget to perform each ranging
int timing_budget_set(const int dev, const int microseconds_per_ranging) {
  struct stmvl53l1_parameter params;

  params.is_read = 0;
  params.name = VL53L1_TIMINGBUDGET_PAR;
  params.value = microseconds_per_ranging;

  return ioctl(dev, VL53L1_IOCTL_PARAMETER, &params);
}

/// Set sensor distance mode
int distance_mode_set(const int dev, const int mode) {
  struct stmvl53l1_parameter params;

  params.is_read = 0;
  params.name = VL53L1_DISTANCEMODE_PAR;
  params.value = mode;

  return ioctl(dev, VL53L1_IOCTL_PARAMETER, &params);
}


int output_mode_set(const int dev, const int mode)
{
  struct stmvl53l1_parameter params;

  params.is_read = 0;
  params.name = VL53L1_OUTPUTMODE_PAR;
  params.value = mode;

  return ioctl(dev, VL53L1_IOCTL_PARAMETER, &params);  
}

/// Setup grid of ROIs for scanning
int setup_roi_grid(const int dev, const int rows, const int cols) {
  struct stmvl53l1_roi_full_t ioctl_roi;
  int i, r, c;
  const int n_roi = rows*cols;
  const int row_step = SPAD_ROWS / rows;
  const int col_step = SPAD_COLS / cols;

  if (rows > MAX_ROWS) {
    error("Cannot set %d rows (max %d)", rows, MAX_ROWS);
    return -1;
  }
  if (rows < 1) {
    error("Cannot set %d rows, min 1", rows);
    return -1;
  }
  if (cols > MAX_COLS) {
    error("Cannot set %d cols (max %d)", cols, MAX_ROWS);
    return -1;
  }
  if (cols < 1) {
    error("Cannot set %d cols, min 1", cols);
    return -1;
  }
  if (n_roi > VL53L1_MAX_USER_ZONES) {
    error("%drows * %dcols = %d > %d max user zones", rows, cols, n_roi, VL53L1_MAX_USER_ZONES);
    return -1;
  }

  i = 0;
  for (r=0; r<rows; ++r) {
    for (c=0; c<cols; ++c) {
      ioctl_roi.roi_cfg.UserRois[i].TopLeftX = c * col_step;
      ioctl_roi.roi_cfg.UserRois[i].TopLeftY = ((r+1) * row_step) - 1;
      ioctl_roi.roi_cfg.UserRois[i].BotRightX = ((c+1) * col_step) - 1;
      ioctl_roi.roi_cfg.UserRois[i].BotRightY = r * row_step;
      ++i;
    }
  }

  ioctl_roi.is_read = 0;
  ioctl_roi.roi_cfg.NumberOfRoi = n_roi;
  return ioctl(dev, VL53L1_IOCTL_ROI, &ioctl_roi);
}

#define return_if_not(cond, ret, fmt, ...) do { if (!(cond)) { \
      PRINT_NAMED_ERROR("", "[E] " fmt "\n", ##__VA_ARGS__);   \
  return ret; \
}} while(0)

/// Setup 4x4 multi-zone imaging
int setup(void) {
  int fd = -1;
  int rc = 0;

  PRINT_NAMED_ERROR("","open dev\n");
  fd = open_dev(0);
  return_if_not(fd >= 0, -1, "Could not open VL53L1 device");

  // Stop all ranging so we can change settings
  PRINT_NAMED_ERROR("","stop ranging\n");
  rc = stop_ranging(fd);
  return_if_not(rc == 0, -1, "ioctl error stopping ranging: %d", errno);

  // Switch to multi-zone scanning mode
  PRINT_NAMED_ERROR("","Switch to multi-zone scanning\n");
  rc = preset_mode_set(fd, VL53L1_PRESETMODE_MULTIZONES_SCANNING);
  return_if_not(rc == 0, -1, "ioctl error setting preset_mode: %d", errno);

  // Setup ROIs
  PRINT_NAMED_ERROR("","Setup ROI grid\n");
  rc = setup_roi_grid(fd, 4, 4);
  return_if_not(rc == 0, -1, "ioctl error setting up preset grid: %d", errno);

  // Setup timing budget
  PRINT_NAMED_ERROR("","set timing budget\n");
  rc = timing_budget_set(fd, 16*2000);
  return_if_not(rc == 0, -1, "ioctl error setting timing budged: %d", errno);

  // Set distance mode
  PRINT_NAMED_ERROR("","set distance mode\n");
  rc = distance_mode_set(fd, VL53L1_DISTANCEMODE_SHORT);
  return_if_not(rc == 0, -1, "ioctl error setting distance mode: %d", errno);

  // Set output mode
  PRINT_NAMED_ERROR("","set output mode\n");
  rc = output_mode_set(fd, VL53L1_OUTPUTMODE_STRONGEST);
  return_if_not(rc == 0, -1, "ioctl error setting distance mode: %d", errno);
  
  // Start the sensor
  PRINT_NAMED_ERROR("","start ranging\n");
  rc = start_ranging(fd);
  return_if_not(rc == 0, -1, "ioctl error starting ranging: %d", errno);

  return fd;
}


/// Get multi-zone ranging data measurements.
int get_mz_data(const int dev, const int blocking, VL53L1_MultiRangingData_t *data) {
  return ioctl(dev, blocking ? VL53L1_IOCTL_MZ_DATA_BLOCKING : VL53L1_IOCTL_MZ_DATA, data);
}


RangeDataRaw ReadData()
{
  RangeDataRaw rangeData;

  int rc = 0;
  VL53L1_MultiRangingData_t mz_data;
  for(int i = 0; i < 4; i++)
  {
    for(int j = 0; j < 4; j++)
    {
      rc = get_mz_data(tof_fd, 1, &mz_data);
      if(rc == 0)
      {
        rangeData.data[i*8 + j] = 0;
        rangeData.data[i*8 + j + 4] = 0;

        if(mz_data.NumberOfObjectsFound > 0)
        {
          int16_t minDist = 2000;
          for(int r = 0; r < mz_data.NumberOfObjectsFound; r++)
          {
            if(mz_data.RangeData[r].RangeStatus == 0)
            {
              const int16_t dist = mz_data.RangeData[r].RangeMilliMeter;
              PRINT_NAMED_WARNING("","I:%d R:%d D:%d", i*4 + j, r, dist);
              if(dist < minDist)
              {
                minDist = dist;
              }
            }
            else if(mz_data.RangeData[r].RangeStatus == VL53L1_RANGESTATUS_OUTOFBOUNDS_FAIL ||
                    mz_data.RangeData[r].RangeStatus == VL53L1_RANGESTATUS_RANGE_VALID_MIN_RANGE_CLIPPED)
            {
              minDist = 1999;
            }
          }
          if(minDist >= 2000)
          {
            minDist = 0;
          }
          rangeData.data[i*8 + j] = minDist / 1000.f;
          rangeData.data[i*8 + j + 4] = minDist / 1000.f;
        }
      }
    }
  }
  return rangeData;
}

void ProcessLoop()
{
  while(!kStartToF)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
    
  tof_fd = setup();
  if(tof_fd < 0)
  {
   PRINT_NAMED_ERROR("","FAILED TO OPEN TOF");
   return;
  }

  while(!_stopProcessing)
  {
    RangeDataRaw data = ReadData();
    
    {
      std::lock_guard<std::mutex> lock(_mutex);
      _latestData = data;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
  }

  stop_ranging(tof_fd);
  close(tof_fd);
}


ToFSensor::ToFSensor()
{
  _processor = std::thread(ProcessLoop);
  _processor.detach();
}

ToFSensor::~ToFSensor()
{
  _stopProcessing = true;
}

Result ToFSensor::Update()
{
  return RESULT_OK;
}

RangeDataRaw ToFSensor::GetData()
{
  std::lock_guard<std::mutex> lock(_mutex);
  return _latestData;
}
  
}
}
