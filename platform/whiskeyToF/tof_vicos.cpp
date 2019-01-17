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
#include <queue>
#include <iomanip>

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

#define USE_BOTH_SENSORS 0

namespace Anki {
namespace Cozmo {

namespace {
  int tofR_fd = -1;
  int tofL_fd = -1;

  // thread and mutex for setting up and reading from the sensors
  std::thread _processor;
  std::mutex _mutex;
  bool _stopProcessing = false;

  std::mutex _commandLock;
  enum Command
    {
     StartRanging,
     StopRanging,
     SetupSensors,
     PerformCalibration,
     LoadCalibration,
    };
  std::queue<std::pair<Command, ToFSensor::CommandCallback>> _commandQueue;

  std::atomic<bool> _rangingEnabled;
  std::atomic<bool> _isCalibrating;

  // The latest available tof data
  RangeDataRaw _latestData;

  // Enum for which sensor we are trying to operate on
  // Values map directly to corresponsing /dev/stmvl53l1_ranging
  // and /dev/stmvl53l1_ranging1 devices
  enum Sensor
  {
   #if USE_BOTH_SENSORS
   RIGHT = 0,
   LEFT = 1,
   #else
   RIGHT = 1,
   LEFT = 0,
   #endif
  };

  bool _dataUpdatedSinceLastGetCall = false;

  VL53L1_CalibrationData_t _origCalib_left;
  VL53L1_CalibrationData_t _origCalib_right;

  uint32_t _distanceToCalibTarget_mm = 0;
  float _calibTargetReflectance = 0;

  std::string _logPath = "/";

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

/// Read last device error
int last_error_get(const int dev) {
  struct stmvl53l1_parameter params;
  params.is_read = 1;
  params.name = VL53L1_LASTERROR_PAR;
  int rc = ioctl(dev, VL53L1_IOCTL_PARAMETER, &params);
  PRINT_NAMED_ERROR("","ERROR %u %d", rc, params.value);
  if(rc < 0)
  {
    return rc;
  }
  
  return params.value;
}

/// Stop all ranging activity
int stop_ranging(const int dev_no) {
  const int rc = ioctl(dev_no, VL53L1_IOCTL_STOP, NULL);
  if (errno == EBUSY) {
    PRINT_NAMED_WARNING("","already stopped");
    return 0;
  }
  return rc;
}

/// Start ranging
int start_ranging(const int dev) {
  int rc = ioctl(dev, VL53L1_IOCTL_START, NULL);
  if (errno == EBUSY) {
    PRINT_NAMED_WARNING("","already started or calibrating");
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
// 0 : Device is put under reset when stopped
// 1 : Device is not put under reset when stopped
int reset_on_stop_set(const int dev, const int val) {
  return 0;
  
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

int offset_correction_mode_set(const int dev, const int mode)
{
  struct stmvl53l1_parameter params;

  params.is_read = 0;
  params.name = VL53L1_OFFSETCORRECTIONMODE_PAR;
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

#if FACTORY_TEST

int perform_refspad_calibration(const int dev)
{
  struct stmvl53l1_ioctl_perform_calibration_t ioctl_cal = {VL53L1_CALIBRATION_REF_SPAD, 0, 0, 0};
  int rc = ioctl(dev, VL53L1_IOCTL_PERFORM_CALIBRATION, &ioctl_cal);
  if(rc < 0)
  {
    int deviceErr = last_error_get(dev);
    PRINT_NAMED_WARNING("","perform refspad calibration failed, device error %d", deviceErr);
  }
  return rc;
}

int perform_xtalk_calibration(const int dev)
{
  // Contrary to the comments in stmvl53l1_if.h, param2 is in fact used and is the preset mode to use
  // for crosstalk calibration
  // Note: For SINGLE_TARGET calibration, only AUTONOMOUS, LOWPOWER_AUTONOMOUS, and LITE_RANGING preset modes are
  // supported
  struct stmvl53l1_ioctl_perform_calibration_t ioctl_cal = {VL53L1_CALIBRATION_CROSSTALK,
                                                            VL53L1_XTALKCALIBRATIONMODE_NO_TARGET,
                                                            VL53L1_PRESETMODE_MULTIZONES_SCANNING,
                                                            0};
  int rc = ioctl(dev, VL53L1_IOCTL_PERFORM_CALIBRATION, &ioctl_cal);

  return rc;  
}

int get_calibration_data(const int dev, VL53L1_CalibrationData_t& calib)
{
  struct stmvl53l1_ioctl_calibration_data_t calibData;
  calibData.is_read = 1;
  int rc = ioctl(dev, VL53L1_IOCTL_CALIBRATION_DATA, &calibData);
  if(rc >= 0)
  {
    calib = calibData.data;
  }
  return rc;
}

int get_zone_calibration_data(const int dev, stmvl531_zone_calibration_data_t& calib)
{
  stmvl53l1_ioctl_zone_calibration_data_t calibData;
  calibData.is_read = 1;
  int rc = ioctl(dev, VL53L1_IOCTL_ZONE_CALIBRATION_DATA, &calibData);
  if(rc >= 0)
  {
    calib = calibData.data;
  }
  return rc;
}

int set_calibration_data(const int dev, VL53L1_CalibrationData_t& calib)
{
  struct stmvl53l1_ioctl_calibration_data_t calibData;
  calibData.data = calib;
  calibData.is_read = 0;
  return ioctl(dev, VL53L1_IOCTL_CALIBRATION_DATA, &calibData);
}

int set_zone_calibration_data(const int dev, stmvl531_zone_calibration_data_t& calib)
{
  struct stmvl53l1_ioctl_zone_calibration_data_t calibData;
  calibData.data = calib;
  calibData.is_read = 0;
  return ioctl(dev, VL53L1_IOCTL_ZONE_CALIBRATION_DATA, &calibData);
}

int save_calibration_to_disk(VL53L1_CalibrationData_t& calib,
                             int dev,
                             std::string meta,
                             std::string path)
{
  PRINT_NAMED_ERROR("","SAVING %u %u %u", dev, tofR_fd, tofL_fd);
  char buf[128];
  sprintf(buf, "%stof_%s%s.bin", path.c_str(), (dev == tofR_fd ? "right" : "left"), meta.c_str());
  PRINT_NAMED_ERROR("","%s", buf);
  int rc = -1;
  FILE* f = fopen(buf, "w+");
  if(f != nullptr)
  {
    rc = fwrite(&calib, sizeof(calib), 1, f);
    (void)fclose(f);
  }
  else
  {
    PRINT_NAMED_ERROR("","FAILED TO OPEN FILE %u", errno);
  }
  return rc;

}

int save_calibration_to_disk(VL53L1_CalibrationData_t& calib,
                             int dev,
                             std::string meta = "")
{
  (void)save_calibration_to_disk(calib, dev, meta, _logPath);
  return save_calibration_to_disk(calib, dev, meta, "/factory/");
}

int load_calibration_from_disk(VL53L1_CalibrationData_t& calib,
                               const std::string& path)
{
  int rc = -1;
  FILE* f = fopen(path.c_str(), "r");
  if(f != nullptr)
  {
    rc = fread(&calib, sizeof(calib), 1, f);
    (void)fclose(f);
  }
  return rc;
}

int save_zone_calibration_to_disk(stmvl531_zone_calibration_data_t& calib,
                                  int dev,
                                  std::string meta,
                                  std::string path)
{
  PRINT_NAMED_ERROR("","SAVING %u %u %u", dev, tofR_fd, tofL_fd);
  char buf[128];
  sprintf(buf, "%stofZone_%s%s.bin", path.c_str(), (dev == tofR_fd ? "right" : "left"), meta.c_str());
  PRINT_NAMED_ERROR("","%s", buf);
  int rc = -1;
  FILE* f = fopen(buf, "w+");
  if(f != nullptr)
  {
    rc = fwrite(&calib, sizeof(calib), 1, f);
    (void)fclose(f);
  }
  else
  {
    PRINT_NAMED_ERROR("","FAILED TO OPEN FILE %u", errno);
  }
  return rc;

}

int save_zone_calibration_to_disk(stmvl531_zone_calibration_data_t& calib,
                                  int dev,
                                  std::string meta = "")
{
  (void)save_zone_calibration_to_disk(calib, dev, meta, _logPath);
  return save_zone_calibration_to_disk(calib, dev, meta, "/factory/");
}

int load_zone_calibration_from_disk(stmvl531_zone_calibration_data_t& calib,
                                    const std::string& path)
{
  int rc = -1;
  FILE* f = fopen(path.c_str(), "r");
  if(f != nullptr)
  {
    rc = fread(&calib, sizeof(calib), 1, f);
    (void)fclose(f);
  }
  return rc;
}

void print_offset_calibration(const VL53L1_CalibrationData_t& calib)
{
  PRINT_NAMED_ERROR("OFFSET CALIBRATION DATA",
                    "range_offset_mm:%d\n"
                    "inner_offset:%d\n"
                    "outer_offset:%d\n"
                    "inner_act_eff_spads:%u\n"
                    "outer_act_eff_spads:%u\n"
                    "innter_peak_count:%u\n"
                    "outer_peak_count:%u\n"
                    "act_eff_spads:%u\n"
                    "peak_signal_count:%u\n"
                    "distance:%u\n"
                    "reflet:%u\n"
                    "glass_trans:%u\n",
                    calib.customer.algo__part_to_part_range_offset_mm,
                    calib.customer.mm_config__inner_offset_mm,
                    calib.customer.mm_config__outer_offset_mm,
                    calib.add_off_cal_data.result__mm_inner_actual_effective_spads,
                    calib.add_off_cal_data.result__mm_outer_actual_effective_spads,
                    calib.add_off_cal_data.result__mm_inner_peak_signal_count_rtn_mcps,
                    calib.add_off_cal_data.result__mm_outer_peak_signal_count_rtn_mcps,
                    calib.cust_dmax_cal.ref__actual_effective_spads,
                    calib.cust_dmax_cal.ref__peak_signal_count_rate_mcps,
                    calib.cust_dmax_cal.ref__distance_mm,
                    calib.cust_dmax_cal.ref_reflectance_pc,
                    calib.cust_dmax_cal.coverglass_transmission);
}

void print_refspad_calibration(const VL53L1_CalibrationData_t& calib)
{
  PRINT_NAMED_ERROR("REFSPAD CALIBRATION DATA",
                    "ref_0: %u\n ref_1:%u\n ref_2:%u\n ref_3:%u\n ref_4:%u\n ref_5:%u\n requested:%u\n ref_loc:%u\n",
                    calib.customer.global_config__spad_enables_ref_0,
                    calib.customer.global_config__spad_enables_ref_1,
                    calib.customer.global_config__spad_enables_ref_2,
                    calib.customer.global_config__spad_enables_ref_3,
                    calib.customer.global_config__spad_enables_ref_4,
                    calib.customer.global_config__spad_enables_ref_5,
                    calib.customer.ref_spad_man__num_requested_ref_spads,
                    calib.customer.ref_spad_man__ref_location);

}

void print_xtalk_calibration(const VL53L1_CalibrationData_t& calib)
{
  PRINT_NAMED_ERROR("XTALK CALIBRATION DATA",
                    "plane_offset: %u\n"
                    "x_plane_offset:%d\n" 
                    "y_plane_offset:%d\n"
                    "zone_id:%u\n"
                    "time_stamp:%u\n" 
                    "first_bin:%u\n" 
                    "buffer_size:%u\n" 
                    "num_bins:%u\n" 
                    "bin0:%u\n" 
                    "bin1:%u\n" 
                    "bin2:%u\n" 
                    "bin3:%u\n" 
                    "bin4:%u\n" 
                    "bin5:%u\n"
                    "bin6:%u\n" 
                    "bin7:%u\n" 
                    "bin8:%u\n" 
                    "bin9:%u\n" 
                    "bin10:%u\n" 
                    "bin11:%u\n" 
                    "ref_phase:%u\n" 
                    "phase_vcsel_start:%u\n" 
                    "cal_vcsel_start:%u\n"
                    "vcsel_width:%u\n"
                    "osc:%u\n"
                    "dist_phase:%u\n",
                    calib.customer.algo__crosstalk_compensation_plane_offset_kcps,
                    calib.customer.algo__crosstalk_compensation_x_plane_gradient_kcps,
                    calib.customer.algo__crosstalk_compensation_y_plane_gradient_kcps,                    
                    calib.xtalkhisto.xtalk_shape.zone_id,
                    calib.xtalkhisto.xtalk_shape.time_stamp,
                    calib.xtalkhisto.xtalk_shape.VL53L1_PRM_00019,
                    calib.xtalkhisto.xtalk_shape.VL53L1_PRM_00020,
                    calib.xtalkhisto.xtalk_shape.VL53L1_PRM_00021,
                    calib.xtalkhisto.xtalk_shape.bin_data[0],
                    calib.xtalkhisto.xtalk_shape.bin_data[1],
                    calib.xtalkhisto.xtalk_shape.bin_data[2],
                    calib.xtalkhisto.xtalk_shape.bin_data[3],
                    calib.xtalkhisto.xtalk_shape.bin_data[4],
                    calib.xtalkhisto.xtalk_shape.bin_data[5],
                    calib.xtalkhisto.xtalk_shape.bin_data[6],
                    calib.xtalkhisto.xtalk_shape.bin_data[7],
                    calib.xtalkhisto.xtalk_shape.bin_data[8],
                    calib.xtalkhisto.xtalk_shape.bin_data[9],
                    calib.xtalkhisto.xtalk_shape.bin_data[10],
                    calib.xtalkhisto.xtalk_shape.bin_data[11],
                    calib.xtalkhisto.xtalk_shape.phasecal_result__reference_phase,
                    calib.xtalkhisto.xtalk_shape.phasecal_result__vcsel_start,
                    calib.xtalkhisto.xtalk_shape.cal_config__vcsel_start,
                    calib.xtalkhisto.xtalk_shape.vcsel_width,
                    calib.xtalkhisto.xtalk_shape.VL53L1_PRM_00022,
                    calib.xtalkhisto.xtalk_shape.zero_distance_phase);
}

void zero_xtalk_calibration(VL53L1_CalibrationData_t& calib)
{
  calib.customer.algo__crosstalk_compensation_plane_offset_kcps = 0;
  calib.customer.algo__crosstalk_compensation_x_plane_gradient_kcps = 0;
  calib.customer.algo__crosstalk_compensation_y_plane_gradient_kcps = 0;                  
  memset(&calib.xtalkhisto, 0, sizeof(calib.xtalkhisto));
}


int run_refspad_calibration(const int dev)
{
  VL53L1_CalibrationData_t calib;
  memset(&calib, 0, sizeof(calib));
  int rc = get_calibration_data(dev, calib);
  return_if_not(rc >= 0, rc, "1 Get calibration data failed: %d %d", rc, errno);

  PRINT_NAMED_ERROR("ORIG REFSPAD CALIBRATION DATA",
                    "ref_0: %u\n ref_1:%u\n ref_2:%u\n ref_3:%u\n ref_4:%u\n ref_5:%u\n requested:%u\n ref_loc:%u\n",
                    calib.customer.global_config__spad_enables_ref_0,
                    calib.customer.global_config__spad_enables_ref_1,
                    calib.customer.global_config__spad_enables_ref_2,
                    calib.customer.global_config__spad_enables_ref_3,
                    calib.customer.global_config__spad_enables_ref_4,
                    calib.customer.global_config__spad_enables_ref_5,
                    calib.customer.ref_spad_man__num_requested_ref_spads,
                    calib.customer.ref_spad_man__ref_location);
  

  rc = perform_refspad_calibration(dev);
  return_if_not(rc >= 0, rc, "RefSPAD calibration failed: %d %d", rc, errno);

  //usleep(50000);
  
  memset(&calib, 0, sizeof(calib));
  rc = get_calibration_data(dev, calib);
  return_if_not(rc >= 0, rc, "2 Get calibration data failed: %d %d", rc, errno);

  PRINT_NAMED_ERROR("REFSPAD CALIBRATION DATA",
                    "ref_0: %u\n ref_1:%u\n ref_2:%u\n ref_3:%u\n ref_4:%u\n ref_5:%u\n requested:%u\n ref_loc:%u\n",
                    calib.customer.global_config__spad_enables_ref_0,
                    calib.customer.global_config__spad_enables_ref_1,
                    calib.customer.global_config__spad_enables_ref_2,
                    calib.customer.global_config__spad_enables_ref_3,
                    calib.customer.global_config__spad_enables_ref_4,
                    calib.customer.global_config__spad_enables_ref_5,
                    calib.customer.ref_spad_man__num_requested_ref_spads,
                    calib.customer.ref_spad_man__ref_location);

  rc = save_calibration_to_disk(calib, dev);
  return_if_not(rc >= 0, rc, "Save calibration to disk failed: %d %d", rc, errno);

  rc = set_calibration_data(dev, calib);
  return_if_not(rc >= 0, rc, "Set calibration data failed: %d %d", rc, errno);

  //usleep(50000);
  
  return rc;
}

int run_xtalk_calibration(const int dev)
{
  VL53L1_CalibrationData_t calib;
  memset(&calib, 0, sizeof(calib));
  int rc = get_calibration_data(dev, calib);
  return_if_not(rc >= 0, rc, "1 Get calibration data failed: %d %d", rc, errno);

  print_xtalk_calibration(calib);

  rc = perform_xtalk_calibration(dev);
  //return_if_not(rc >= 0, rc, "Xtalk calibration failed: %d %d", rc, errno);
  bool noXtalk = false;
  if(rc < 0 && errno == EIO)
  {
    // An error -22 may be issued after calibration if the system failed to find
    // a valid cross talk value. This is due to the fact that the coverglass
    // quality is very good. In this case, no crosstalk data should be applied.
    int deviceErr = last_error_get(dev);    
    PRINT_NAMED_WARNING("","perform xtalk calibration failed, device error %d", deviceErr);
    if(deviceErr == VL53L1_ERROR_XTALK_EXTRACTION_NO_SAMPLE_FAIL)
    {
      PRINT_NAMED_ERROR("","NO CROSSTALK FOUND");
      noXtalk = true;
    }
  }
  
  //usleep(50000);

  memset(&calib, 0, sizeof(calib));
  rc = get_calibration_data(dev, calib);
  return_if_not(rc >= 0, rc, "2 Get calibration data failed: %d %d", rc, errno);

  if(!noXtalk)
  {
    print_xtalk_calibration(calib);
  }
  else
  {
    zero_xtalk_calibration(calib);
  }
    
  rc = save_calibration_to_disk(calib, dev);
  return_if_not(rc >= 0, rc, "Save calibration to disk failed: %d %d", rc, errno);

  rc = set_calibration_data(dev, calib);
  return_if_not(rc >= 0, rc, "Set calibration data failed: %d %d", rc, errno);
  
  return rc;
}



int perform_offset_calibration(const int dev, uint32_t distanceToTarget_mm, float targetReflectance)
{
  // I think per zone offset calibration would be more desireable but was unable to get it to work, kept getting
  // ctrl_perform_zone_calibration_offset_lock: VL53L1_PerformOffsetCalibration fail => -35
  // target might have been too reflective...
  PRINT_NAMED_INFO("","running offset calibration at %u with reflectance %f", distanceToTarget_mm, targetReflectance);
  
  struct stmvl53l1_ioctl_perform_calibration_t ioctl_cal = {VL53L1_CALIBRATION_OFFSET_PER_ZONE,
                                                            VL53L1_OFFSETCALIBRATIONMODE_MULTI_ZONE,
                                                            distanceToTarget_mm,
                                                            (FixPoint1616_t)(targetReflectance * (2^16))};

  // struct stmvl53l1_ioctl_perform_calibration_t ioctl_cal = {VL53L1_CALIBRATION_OFFSET,
  //                                                           VL53L1_OFFSETCALIBRATIONMODE_PRERANGE_ONLY,
  //                                                           distanceToTarget_mm,
  //                                                           (FixPoint1616_t)(targetReflectance * (2^16))};

  int rc = ioctl(dev, VL53L1_IOCTL_PERFORM_CALIBRATION, &ioctl_cal);
  if(rc < 0)
  {
    int deviceErr = last_error_get(dev);
    PRINT_NAMED_ERROR("","perform offset calibration failed, device error %d", deviceErr);
  }
  return rc;
}

int run_offset_calibration(const int dev, uint32_t distanceToTarget_mm, float targetReflectance)
{
  VL53L1_CalibrationData_t calib;
  memset(&calib, 0, sizeof(calib));
  int rc = get_calibration_data(dev, calib);
  return_if_not(rc >= 0, rc, "1 Get calibration data failed: %d %d", rc, errno);

  PRINT_NAMED_ERROR("","ORIG");
  print_offset_calibration(calib);

  rc = setup_roi_grid(dev, 4, 4);
  return_if_not(rc == 0, -1, "ioctl error setting up preset grid: %d", errno);

  rc = perform_offset_calibration(dev, distanceToTarget_mm, targetReflectance);
  return_if_not(rc >= 0, rc, "offset calibration failed: %d %d", rc, errno);

  //usleep(50000);
  
  memset(&calib, 0, sizeof(calib));
  rc = get_calibration_data(dev, calib);
  return_if_not(rc >= 0, rc, "2 Get calibration data failed: %d %d", rc, errno);

  PRINT_NAMED_ERROR("","NEW");
  print_offset_calibration(calib);
  
  rc = save_calibration_to_disk(calib, dev);
  return_if_not(rc >= 0, rc, "Save calibration to disk failed: %d %d", rc, errno);

  rc = set_calibration_data(dev, calib);
  return_if_not(rc >= 0, rc, "Set calibration data failed: %d %d", rc, errno);

  stmvl531_zone_calibration_data_t calibZone;
  memset(&calibZone, 0, sizeof(calibZone));
  rc = get_zone_calibration_data(dev, calibZone);
  return_if_not(rc >= 0, rc, "Get zone calib failed: %d %d", rc, errno);
  
  rc = save_zone_calibration_to_disk(calibZone, dev);
  return_if_not(rc >= 0, rc, "Save zone calibration to disk failed: %d %d", rc, errno);

  rc = set_zone_calibration_data(dev, calibZone);
  return_if_not(rc >= 0, rc, "Set zone calibration data failed: %d %d", rc, errno);

  return rc;
}

#endif

int set_live_crosstalk(const int dev, bool enable)
{
  struct stmvl53l1_parameter params;

  params.is_read = 0;
  params.name = VL53L1_SMUDGECORRECTIONMODE_PAR;
  params.value = (enable ? VL53L1_SMUDGE_CORRECTION_CONTINUOUS : VL53L1_SMUDGE_CORRECTION_NONE);

  return ioctl(dev, VL53L1_IOCTL_PARAMETER, &params);  
}


int perform_calibration(int dev,
                        uint32_t distanceToTarget_mm,
                        float targetReflectance)
{
  // Stop all ranging so we can change settings
  PRINT_NAMED_ERROR("","stop ranging\n");
  int rc = stop_ranging(dev);
  return_if_not(rc == 0, -1, "ioctl error stopping ranging: %d", errno);

  // Have the device reset when ranging is stopped
  PRINT_NAMED_ERROR("","reset on stop\n");
  rc = reset_on_stop_set(dev, 0);
  return_if_not(rc == 0, -1, "ioctl error reset on stop: %d", errno);

  // Switch to multi-zone scanning mode
  PRINT_NAMED_ERROR("","Switch to multi-zone scanning\n");
  rc = preset_mode_set(dev, VL53L1_PRESETMODE_MULTIZONES_SCANNING);
  return_if_not(rc == 0, -1, "ioctl error setting preset_mode: %d", errno);

  // Setup ROIs
  PRINT_NAMED_ERROR("","Setup ROI grid\n");
  rc = setup_roi_grid(dev, 4, 4);
  return_if_not(rc == 0, -1, "ioctl error setting up preset grid: %d", errno);

  // Setup timing budget
  PRINT_NAMED_ERROR("","set timing budget\n");
  rc = timing_budget_set(dev, 8*2000);
  return_if_not(rc == 0, -1, "ioctl error setting timing budged: %d", errno);

  // Set distance mode
  PRINT_NAMED_ERROR("","set distance mode\n");
  rc = distance_mode_set(dev, VL53L1_DISTANCEMODE_SHORT);
  return_if_not(rc == 0, -1, "ioctl error setting distance mode: %d", errno);

  // Set output mode
  PRINT_NAMED_ERROR("","set output mode\n");
  rc = output_mode_set(dev, VL53L1_OUTPUTMODE_STRONGEST);
  return_if_not(rc == 0, -1, "ioctl error setting distance mode: %d", errno);

  // PRINT_NAMED_ERROR("","Enable live xtalk\n");
  // rc = set_live_crosstalk(dev, true);
  // return_if_not(rc == 0, -1, "ioctl error setting live xtalk: %d", errno);

  VL53L1_CalibrationData_t calib;
  memset(&calib, 0, sizeof(calib));
  set_calibration_data(dev, calib);
  
  rc = get_calibration_data(dev, calib);
  return_if_not(rc >= 0, rc, "1 Get calibration data failed: %d %d", rc, errno);

  rc = save_calibration_to_disk(calib, dev, "Orig");
  return_if_not(rc >= 0, rc, "Save calibration to disk failed: %d %d", rc, errno);
  

  rc = run_refspad_calibration(dev);
  return_if_not(rc >= 0, rc, "perform_calibration: run_refspad_calibration %d", rc);
  
  rc = run_xtalk_calibration(dev);
  return_if_not(rc >= 0, rc, "perform_calibration: run_xtalk_calibration %d", rc);

  rc = run_offset_calibration(dev, distanceToTarget_mm, targetReflectance);
  return_if_not(rc >= 0, rc, "perform_calibration: run_offset_calibration %d", rc);

  return rc;
}

void ToFSensor::SetLogPath(const std::string& path)
{
  _logPath = path;
}

ToFSensor::CommandResult run_calibration(uint32_t distanceToTarget_mm,
                                         float targetReflectance)
{
  int rcR = perform_calibration(tofR_fd, distanceToTarget_mm, targetReflectance);
  if(rcR < 0)
  {
    PRINT_NAMED_ERROR("ToFSensor.PerformCalibration.RightFailed",
                      "Failed to calibrate right sensor %d",
                      rcR);
  }

  int rcL = 0;
  #if USE_BOTH_SENSORS
  rcL = perform_calibration(tofL_fd, distanceToTarget_mm, targetReflectance);
  if(rcL < 0)
  {
    PRINT_NAMED_ERROR("ToFSensor.PerformCalibration.LeftFailed",
                      "Failed to calibrate left sensor %d",
                      rcL);
  }
  #endif
  _isCalibrating = false;

  ToFSensor::CommandResult res = ToFSensor::CommandResult::Success;
  if(rcR < 0)
  {
    res = ToFSensor::CommandResult::CalibrateRightFailed;
  }
  else if(rcL < 0)
  {
    res = ToFSensor::CommandResult::CalibrateLeftFailed;
  }
  return res;
}

int ToFSensor::PerformCalibration(uint32_t distanceToTarget_mm, float targetReflectance,
                                  const CommandCallback& callback)
{
  std::lock_guard<std::mutex> lock(_commandLock);
  _commandQueue.push({Command::PerformCalibration, callback});
  _distanceToCalibTarget_mm = distanceToTarget_mm;
  _calibTargetReflectance = targetReflectance;
  _isCalibrating = true;
  return 0;
}

/// Setup 4x4 multi-zone imaging
int setup(Sensor which) {
  int rc = 0;

  int fd = (which == LEFT ? tofL_fd : tofR_fd);

  // Stop all ranging so we can change settings
  PRINT_NAMED_ERROR("","stop ranging\n");
  rc = stop_ranging(fd);
  return_if_not(rc == 0, -1, "ioctl error stopping ranging: %d", errno);

  auto* origCalib = (which == LEFT ? &_origCalib_left : &_origCalib_right);
  memset(origCalib, 0, sizeof(*origCalib));
  rc = get_calibration_data(fd, *origCalib);
  return_if_not(rc == 0, -1, "ioctl error getting calibration: %d", errno);
  
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
  rc = output_mode_set(fd, VL53L1_OUTPUTMODE_STRONGEST);
  return_if_not(rc == 0, -1, "ioctl error setting distance mode: %d", errno);

  // PRINT_NAMED_ERROR("","Enable live xtalk\n");
  // rc = set_live_crosstalk(fd, true);
  // return_if_not(rc == 0, -1, "ioctl error setting live xtalk: %d", errno);

  PRINT_NAMED_ERROR("","set offset correction mode\n");
  rc = offset_correction_mode_set(fd, VL53L1_OFFSETCORRECTIONMODE_PERZONE);
  return_if_not(rc == 0, -1, "ioctl error setting offset correction mode: %d", errno);

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

  RangingData& roiData = rangeData.data[index];
  roiData.readings.clear();
  roiData.roi = index;
  roiData.numObjects = mz_data.NumberOfObjectsFound;
  roiData.roiStatus = mz_data.RoiStatus;
  roiData.spadCount = mz_data.EffectiveSpadRtnCount / 256.f;
  roiData.processedRange_mm = 0;

  if(mz_data.NumberOfObjectsFound > 0)
  {
    int16_t minDist = 1000;
    for(int r = 0; r < mz_data.NumberOfObjectsFound; r++)
    {
      RangeReading reading;
      reading.status = mz_data.RangeData[r].RangeStatus;
      // The following three readings come up in 16.16 fixed point so convert
      reading.signalRate_mcps = ((float)mz_data.RangeData[r].SignalRateRtnMegaCps * (float)(1/(2^16)));
      reading.ambientRate_mcps = ((float)mz_data.RangeData[r].AmbientRateRtnMegaCps * (float)(1/(2^16)));
      reading.sigma_mm = ((float)mz_data.RangeData[r].SigmaMilliMeter * (float)(1/(2^16)));
      reading.rawRange_mm = mz_data.RangeData[r].RangeMilliMeter;
      
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
      roiData.readings.push_back(reading);
    }
    roiData.processedRange_mm = minDist;
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
      else
      {
        PRINT_NAMED_ERROR("","FAILED TO GET_MZ_DATA %d\n", rc);
      }
    }
  }
}

RangeDataRaw ReadData()
{
  static RangeDataRaw rangeData;

  // Alternate reading from each sensor
  static bool b = true;
  if(b)
  {
    ReadDataFromSensor(RIGHT, rangeData);
  }
#if USE_BOTH_SENSORS
  else
  {
    ReadDataFromSensor(LEFT, rangeData);
  }
  b = !b;
#endif

  
  return rangeData;
}


void load_calibration()
{
  PRINT_NAMED_ERROR("","Load calibration");

  VL53L1_CalibrationData_t calib;
  memset(&calib, 0, sizeof(calib));
  int rc = load_calibration_from_disk(calib, "/factory/tof_right.bin");
  if(rc < 0)
  {
    PRINT_NAMED_ERROR("","Failed to load tof calibration");
  }
  else
  {
    rc = set_calibration_data(tofR_fd, calib);
    if(rc < 0)
    {
      PRINT_NAMED_ERROR("","Failed to set tof calibration");
    }
  }

  stmvl531_zone_calibration_data_t calibZone;
  memset(&calibZone, 0, sizeof(calibZone));
  rc = load_zone_calibration_from_disk(calibZone, "/factory/tofZone_right.bin");
  if(rc < 0)
  {
    PRINT_NAMED_ERROR("","Failed to load tof calibration");
  }
  else
  {
    rc = set_zone_calibration_data(tofR_fd, calibZone);
    if(rc < 0)
    {
      PRINT_NAMED_ERROR("","Failed to set tof calibration");
    }
  }          
}

int ToFSensor::StartRanging(const CommandCallback& callback)
{
  std::lock_guard<std::mutex> lock(_commandLock);
  _commandQueue.push({Command::StartRanging, callback});
  return 0;
}

int ToFSensor::StopRanging(const CommandCallback& callback)
{
  std::lock_guard<std::mutex> lock(_commandLock);
  _commandQueue.push({Command::StopRanging, callback});
  return 0;
}

int ToFSensor::SetupSensors(const CommandCallback& callback)
{
  std::lock_guard<std::mutex> lock(_commandLock);
  _commandQueue.push({Command::SetupSensors, callback});
  return 0;
}

int ToFSensor::LoadCalibration(const CommandCallback& callback)
{
  std::lock_guard<std::mutex> lock(_commandLock);
  _commandQueue.push({Command::StopRanging, nullptr});
  _commandQueue.push({Command::LoadCalibration, callback});
  return 0;
}

void ProcessLoop()
{
  while(!_stopProcessing)
  {
    _commandLock.lock();
    if(_commandQueue.size() > 0)
    {
      auto cmd = _commandQueue.front();
      _commandQueue.pop();
      _commandLock.unlock();

      ToFSensor::CommandResult res = ToFSensor::CommandResult::Success;
      switch(cmd.first)
      {
        case Command::StartRanging:
          {
            PRINT_NAMED_ERROR("","Command start ranging");
            int rc = start_ranging(tofR_fd);
            if(rc < 0)
            {
              res = ToFSensor::CommandResult::StartRangingRightFailed;
              break;
            }

            #if USE_BOTH_SENSORS
            rc = start_ranging(tofL_fd);
            if(rc < 0)
            {
              res = ToFSensor::CommandResult::StartRangingLeftFailed;
              break;
            }
            #endif
            _rangingEnabled = true;
          }
          break;
        case Command::StopRanging:
          {
            PRINT_NAMED_ERROR("","Command stop ranging");
            int rc = stop_ranging(tofR_fd);
            if(rc < 0)
            {
              res = ToFSensor::CommandResult::StopRangingRightFailed;
              break;
            }
            
            #if USE_BOTH_SENSORS
            rc = stop_ranging(tofL_fd);
            if(rc < 0)
            {
              res = ToFSensor::CommandResult::StopRangingLeftFailed;
              break;
            }
            #endif
            _rangingEnabled = false;
          }
          break;
        case Command::SetupSensors:
          {
            PRINT_NAMED_ERROR("","Command setup sensors");
            _rangingEnabled = false;

            if(tofR_fd < 0)
            {
              tofR_fd = open_dev(RIGHT);
              if(tofR_fd < 0)
              {
                res = ToFSensor::CommandResult::OpenRightDevFailed;
                PRINT_NAMED_ERROR("","FAILED TO OPEN RIGHT");
                break;
              }
            }
            #if USE_BOTH_SENSORS
            if(tofL_fd < 0)
            {
              tofL_fd = open_dev(LEFT);
              if(tofL_fd < 0)
              {
                res = ToFSensor::CommandResult::OpenLeftDevFailed;
                PRINT_NAMED_ERROR("","FAILED TO OPEN LEFT");
                break;
              }
            }
            #endif

            //            load_calibration();
            
            tofR_fd = setup(RIGHT);
            if(tofR_fd < 0)
            {
              res = ToFSensor::CommandResult::SetupRightFailed;
              PRINT_NAMED_ERROR("","FAILED TO OPEN TOF R");
            }

            #if USE_BOTH_SENSORS
            tofL_fd = setup(LEFT);
            if(tofL_fd < 0)
            {
              res = ToFSensor::CommandResult::SetupLeftFailed;
              PRINT_NAMED_ERROR("","FAILED TO OPEN TOF L");
            }
            #endif
          }
          break;
        case Command::PerformCalibration:
          {
            PRINT_NAMED_ERROR("","Command perform calibration");
            _rangingEnabled = false;
            res = run_calibration(_distanceToCalibTarget_mm, _calibTargetReflectance);
          }
          break;

        case Command::LoadCalibration:
          {
            load_calibration();
          }
          break;
      }

      // Call command callback
      if(cmd.second != nullptr)
      {
        cmd.second(res);
      }
    }
    else
    {
      _commandLock.unlock();
    }

    if(_rangingEnabled)
    {
      RangeDataRaw data = ReadData();

      static RangeDataRaw lastValid = data;
      
      // std::stringstream ss;
      // for(int i = 0; i < 4; i++)
      // {
      //   for(int j = 0; j < 8; j++)
      //   {
      //     if(data.data[i*8 + j].numObjects > 0 && data.data[i*8 + j].readings[0].status == 0)
      //     {
      //       ss << std::setw(7) << (uint32_t)(data.data[i*8 + j].processedRange_mm);
      //       lastValid.data[i*8 + j] = data.data[i*8 + j];
      //     }
      //     else
      //     {
      //       ss << std::setw(7) << (uint32_t)(lastValid.data[i*8 + j].processedRange_mm);
      //     }
      //   }
      //   ss << "\n";
      // }
      // PRINT_NAMED_ERROR("","%s", ss.str().c_str());

      
      {
        std::lock_guard<std::mutex> lock(_mutex);
        _dataUpdatedSinceLastGetCall = true;
        _latestData = data;
      }
    }
    else
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(32));
    }
  }

  stop_ranging(tofR_fd);
  stop_ranging(tofL_fd);
  close(tofR_fd);
  close(tofL_fd);
}

bool ToFSensor::IsRanging()
{
  return _rangingEnabled;
}

bool ToFSensor::IsCalibrating()
{
  return _isCalibrating;
}

ToFSensor::ToFSensor()
{
  _rangingEnabled = false;
  _isCalibrating = false;
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

RangeDataRaw ToFSensor::GetData(bool& hasDataUpdatedSinceLastCall)
{
  std::lock_guard<std::mutex> lock(_mutex);
  hasDataUpdatedSinceLastCall = _dataUpdatedSinceLastGetCall;
  _dataUpdatedSinceLastGetCall = false;
  return _latestData;
}

bool ToFSensor::IsValidRoiStatus(uint8_t status)
{
  return (status != VL53L1_ROISTATUS_NOT_VALID);
}

}
}
