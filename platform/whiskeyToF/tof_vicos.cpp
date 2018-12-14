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
namespace Cozmo {

namespace {
  int tofR_fd = -1;
  int tofL_fd = -1;

  // thread and mutex for setting up and reading from the sensors
  std::thread _processor;
  std::mutex _mutex;
  bool _stopProcessing = false;

  // The latest available tof data
  RangeDataRaw _latestData;

  // Enum for which sensor we are trying to operate on
  // Values map directly to corresponsing /dev/stmvl53l1_ranging
  // and /dev/stmvl53l1_ranging1 devices
  enum Sensor
  {
   RIGHT = 0,
   LEFT = 1,
  };

  // Don't setup and read from the sensors until this console var is true
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

/// Set device reset on stop
// 0 : Device is ut under reset when stopped
// 1 : Device is not put under reset when stopped
int reset_on_stop_set(const int dev, const int val) {
  struct stmvl53l1_parameter params;

  params.is_read = 0;
  params.name = VL53L1_FORCEDEVICEONEN_PAR;
  params.value = val;

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
int setup(Sensor which) {
  int fd = -1;
  int rc = 0;

  PRINT_NAMED_ERROR("","open dev\n");
  fd = open_dev(which);
  return_if_not(fd >= 0, -1, "Could not open VL53L1 device");

  // Stop all ranging so we can change settings
  PRINT_NAMED_ERROR("","stop ranging\n");
  rc = stop_ranging(fd);
  return_if_not(rc == 0, -1, "ioctl error stopping ranging: %d", errno);

  // Have the device reset when ranging is stopped
  PRINT_NAMED_ERROR("","reset on stop\n");
  rc = reset_on_stop_set(fd, 0);
  return_if_not(rc == 0, -1, "ioctl error reset on stop: %d", errno);

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
  rc = output_mode_set(fd, VL53L1_OUTPUTMODE_NEAREST);
  return_if_not(rc == 0, -1, "ioctl error setting distance mode: %d", errno);

  // Start the sensor
  PRINT_NAMED_ERROR("","start ranging\n");
  rc = start_ranging(fd);
  return_if_not(rc == 0, -1, "ioctl error starting ranging: %d", errno);

  usleep(1000000);
  
  return fd;
}


/// Get multi-zone ranging data measurements.
int get_mz_data(const int dev, const int blocking, VL53L1_MultiRangingData_t *data) {
  return ioctl(dev, blocking ? VL53L1_IOCTL_MZ_DATA_BLOCKING : VL53L1_IOCTL_MZ_DATA, data);
}


void ParseData(Sensor whichSensor,
               VL53L1_MultiRangingData_t& mz_data,
               RangeDataRaw& rangeData)
{
  // Right sensor is flipped compared to left
  if(whichSensor == RIGHT)
  {
    mz_data.RoiNumber = 15 - mz_data.RoiNumber;
  }

  const int offset = (whichSensor == LEFT ? 0 : 4);
  // Convert roi number to index in 8x4 rangeData output array
  const int index = (mz_data.RoiNumber / 4 * 8) + (mz_data.RoiNumber % 4) + offset;

  rangeData.data[index] = 0;

  if(mz_data.NumberOfObjectsFound > 0)
  {
    int16_t minDist = 2000;
    for(int r = 0; r < mz_data.NumberOfObjectsFound; r++)
    {
      // The right sensor reports a lot of invalid RangeStatuses, usually
      // WRAP_TARGET_FAIL. The range data still appears valid though so we ignore the
      // invalid status.
      // Other common invalid status are OUTOFBOUNDS_FAIL and TARGET_PRESENT_LACK_OF_SIGNAL
      if(mz_data.RangeData[r].RangeStatus == VL53L1_RANGESTATUS_RANGE_VALID ||
         mz_data.RangeData[r].RangeStatus == VL53L1_RANGESTATUS_WRAP_TARGET_FAIL)
      {
        const int16_t dist = mz_data.RangeData[r].RangeMilliMeter;
        if(dist < minDist)
        {
          minDist = dist;
        }
      }
      // else
      // {
      //   printf("%u %u RangeStatus %u %u %u\n",
      //          whichSensor,
      //          r,
      //          mz_data.RangeData[r].RangeStatus,
      //          mz_data.RoiStatus,
      //          mz_data.EffectiveSpadRtnCount);
      // }
    }
    rangeData.data[index] = minDist / 1000.f;
  }
}

void ReadDataFromSensor(Sensor which, RangeDataRaw& rangeData)
{
  const int fd = (which == LEFT ? tofL_fd : tofR_fd);
  
  int rc = 0;
  
  for(int i = 0; i < 4; i++)
  {
    for(int j = 0; j < 4; j++)
    {
      VL53L1_MultiRangingData_t mz_data;
      rc = get_mz_data(fd, 1, &mz_data);
      if(rc == 0)
      {
        ParseData(which, mz_data, rangeData);
      }
    }
  }
}

RangeDataRaw ReadData()
{
  static RangeDataRaw rangeData;

  // Alternate reading from each sensor
  static bool b = false;
  if(b)
  {
    ReadDataFromSensor(RIGHT, rangeData);
  }
  else
  {
    ReadDataFromSensor(LEFT, rangeData);
  }
  b = !b;
  
  return rangeData;
}

void ProcessLoop()
{
  // Only setup the sensors when the console var is true
  // There used to be issues with starting ranging on startup
  // due to an unstable driver/daemon (I think these have been fixed...)
  while(!kStartToF)
  {
    if(_stopProcessing)
    {
      return;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  tofR_fd = setup(RIGHT);
  if(tofR_fd < 0)
  {
    PRINT_NAMED_ERROR("","FAILED TO OPEN TOF R");
    return;
  }
  
  tofL_fd = setup(LEFT);
  if(tofL_fd < 0)
  {
    PRINT_NAMED_ERROR("","FAILED TO OPEN TOF L");
    return;
  }

  while(kStartToF && !_stopProcessing)
  {
    RangeDataRaw data = ReadData();
    
    {
      std::lock_guard<std::mutex> lock(_mutex);
      _latestData = data;
    }

   std::this_thread::sleep_for(std::chrono::milliseconds(32));
  }

  stop_ranging(tofR_fd);
  stop_ranging(tofL_fd);
  close(tofR_fd);
  close(tofL_fd);
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
