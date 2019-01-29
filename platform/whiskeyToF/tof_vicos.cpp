/**
 * File: tof_vicos.cpp
 *
 * Author: Al Chaussee
 * Created: 10/18/2018
 *
 * Description: Defines interface to a tof sensor
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "whiskeyToF/tof.h"

#include "whiskeyToF/tofUserspace_vicos.h"
#include "whiskeyToF/tofCalibration_vicos.h"

#include "util/console/consoleInterface.h"
#include "util/console/consoleSystem.h"
#include "util/logging/logging.h"

#include <thread>
#include <mutex>
#include <queue>
#include <iomanip>
#include <chrono>

#ifdef SIMULATOR
#error SIMULATOR should be defined by any target using tof_vicos.cpp
#endif

namespace Anki {
namespace Vector {

namespace {

  // Handle to the actual device
  VL53L1_Dev_t _dev;

  // Thread and mutex for setting up and reading from the sensor
  std::thread _processor;
  std::mutex _mutex;
  bool _stopProcessing = false;

  // Asynchonous commands in order to interact with the device on a thread
  std::mutex _commandLock;
  enum Command
    {
     StartRanging,
     StopRanging,
     SetupSensors,
     PerformCalibration,
    };
  std::queue<std::pair<Command, ToFSensor::CommandCallback>> _commandQueue;

  // Whether or not ranging is currently enabled
  std::atomic<bool> _rangingEnabled;

  // Whether or not calibration is currently running
  std::atomic<bool> _isCalibrating;

  // The latest available tof data
  RangeDataRaw _latestData;

  // Whether or not _latestData has updated since the last call to get it
  bool _dataUpdatedSinceLastGetCall = false;

  // Settings for calibration
  uint32_t _distanceToCalibTarget_mm = 0;
  float _calibTargetReflectance = 0;
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

// void print_offset_calibration(const VL53L1_CalibrationData_t& calib)
// {
//   PRINT_NAMED_ERROR("OFFSET CALIBRATION DATA",
//                     "range_offset_mm:%d\n"
//                     "inner_offset:%d\n"
//                     "outer_offset:%d\n"
//                     "inner_act_eff_spads:%u\n"
//                     "outer_act_eff_spads:%u\n"
//                     "innter_peak_count:%u\n"
//                     "outer_peak_count:%u\n"
//                     "act_eff_spads:%u\n"
//                     "peak_signal_count:%u\n"
//                     "distance:%u\n"
//                     "reflet:%u\n"
//                     "glass_trans:%u\n",
//                     calib.customer.algo__part_to_part_range_offset_mm,
//                     calib.customer.mm_config__inner_offset_mm,
//                     calib.customer.mm_config__outer_offset_mm,
//                     calib.add_off_cal_data.result__mm_inner_actual_effective_spads,
//                     calib.add_off_cal_data.result__mm_outer_actual_effective_spads,
//                     calib.add_off_cal_data.result__mm_inner_peak_signal_count_rtn_mcps,
//                     calib.add_off_cal_data.result__mm_outer_peak_signal_count_rtn_mcps,
//                     calib.cust_dmax_cal.ref__actual_effective_spads,
//                     calib.cust_dmax_cal.ref__peak_signal_count_rate_mcps,
//                     calib.cust_dmax_cal.ref__distance_mm,
//                     calib.cust_dmax_cal.ref_reflectance_pc,
//                     calib.cust_dmax_cal.coverglass_transmission);
// }

// void print_refspad_calibration(const VL53L1_CalibrationData_t& calib)
// {
//   PRINT_NAMED_ERROR("REFSPAD CALIBRATION DATA",
//                     "ref_0: %u\n ref_1:%u\n ref_2:%u\n ref_3:%u\n ref_4:%u\n ref_5:%u\n requested:%u\n ref_loc:%u\n",
//                     calib.customer.global_config__spad_enables_ref_0,
//                     calib.customer.global_config__spad_enables_ref_1,
//                     calib.customer.global_config__spad_enables_ref_2,
//                     calib.customer.global_config__spad_enables_ref_3,
//                     calib.customer.global_config__spad_enables_ref_4,
//                     calib.customer.global_config__spad_enables_ref_5,
//                     calib.customer.ref_spad_man__num_requested_ref_spads,
//                     calib.customer.ref_spad_man__ref_location);

// }

// void print_xtalk_calibration(const VL53L1_CalibrationData_t& calib)
// {
//   PRINT_NAMED_ERROR("XTALK CALIBRATION DATA",
//                     "plane_offset: %u\n"
//                     "x_plane_offset:%d\n" 
//                     "y_plane_offset:%d\n"
//                     "zone_id:%u\n"
//                     "time_stamp:%u\n" 
//                     "first_bin:%u\n" 
//                     "buffer_size:%u\n" 
//                     "num_bins:%u\n" 
//                     "bin0:%u\n" 
//                     "bin1:%u\n" 
//                     "bin2:%u\n" 
//                     "bin3:%u\n" 
//                     "bin4:%u\n" 
//                     "bin5:%u\n"
//                     "bin6:%u\n" 
//                     "bin7:%u\n" 
//                     "bin8:%u\n" 
//                     "bin9:%u\n" 
//                     "bin10:%u\n" 
//                     "bin11:%u\n" 
//                     "ref_phase:%u\n" 
//                     "phase_vcsel_start:%u\n" 
//                     "cal_vcsel_start:%u\n"
//                     "vcsel_width:%u\n"
//                     "osc:%u\n"
//                     "dist_phase:%u\n",
//                     calib.customer.algo__crosstalk_compensation_plane_offset_kcps,
//                     calib.customer.algo__crosstalk_compensation_x_plane_gradient_kcps,
//                     calib.customer.algo__crosstalk_compensation_y_plane_gradient_kcps,                    
//                     calib.xtalkhisto.xtalk_shape.zone_id,
//                     calib.xtalkhisto.xtalk_shape.time_stamp,
//                     calib.xtalkhisto.xtalk_shape.VL53L1_PRM_00019,
//                     calib.xtalkhisto.xtalk_shape.VL53L1_PRM_00020,
//                     calib.xtalkhisto.xtalk_shape.VL53L1_PRM_00021,
//                     calib.xtalkhisto.xtalk_shape.bin_data[0],
//                     calib.xtalkhisto.xtalk_shape.bin_data[1],
//                     calib.xtalkhisto.xtalk_shape.bin_data[2],
//                     calib.xtalkhisto.xtalk_shape.bin_data[3],
//                     calib.xtalkhisto.xtalk_shape.bin_data[4],
//                     calib.xtalkhisto.xtalk_shape.bin_data[5],
//                     calib.xtalkhisto.xtalk_shape.bin_data[6],
//                     calib.xtalkhisto.xtalk_shape.bin_data[7],
//                     calib.xtalkhisto.xtalk_shape.bin_data[8],
//                     calib.xtalkhisto.xtalk_shape.bin_data[9],
//                     calib.xtalkhisto.xtalk_shape.bin_data[10],
//                     calib.xtalkhisto.xtalk_shape.bin_data[11],
//                     calib.xtalkhisto.xtalk_shape.phasecal_result__reference_phase,
//                     calib.xtalkhisto.xtalk_shape.phasecal_result__vcsel_start,
//                     calib.xtalkhisto.xtalk_shape.cal_config__vcsel_start,
//                     calib.xtalkhisto.xtalk_shape.vcsel_width,
//                     calib.xtalkhisto.xtalk_shape.VL53L1_PRM_00022,
//                     calib.xtalkhisto.xtalk_shape.zero_distance_phase);
// }

void ToFSensor::SetLogPath(const std::string& path)
{
  #if FACTORY_TEST
  set_calibration_save_path(path);
  #endif
}

ToFSensor::CommandResult run_calibration(uint32_t distanceToTarget_mm,
                                         float targetReflectance)
{
  int rc = perform_calibration(&_dev, distanceToTarget_mm, targetReflectance);
  if(rc < 0)
  {
    PRINT_NAMED_ERROR("ToFSensor.PerformCalibration.RightFailed",
                      "Failed to calibrate right sensor %d",
                      rc);
  }

  _isCalibrating = false;

  ToFSensor::CommandResult res = (rc < 0 ?
                                  ToFSensor::CommandResult::CalibrateFailed :
                                  ToFSensor::CommandResult::Success);
  return res;
}

int ToFSensor::PerformCalibration(uint32_t distanceToTarget_mm,
                                  float targetReflectance,
                                  const CommandCallback& callback)
{
  #if FACTORY_TEST
  std::lock_guard<std::mutex> lock(_commandLock);
  _distanceToCalibTarget_mm = distanceToTarget_mm;
  _calibTargetReflectance = targetReflectance;
  _commandQueue.push({Command::PerformCalibration, callback});
  _isCalibrating = true;
  #endif
  
  return 0;
}


// Parses and converts VL53L1_MultiRangingData_t into RangeDataRaw
void ParseData(VL53L1_MultiRangingData_t& mz_data,
               RangeDataRaw& rangeData)
{
  const int index = mz_data.RoiNumber;

  RangingData& roiData = rangeData.data[index];
  // Populate singular data from this ROI
  roiData.readings.clear();
  roiData.roi = index;
  roiData.numObjects = mz_data.NumberOfObjectsFound;
  roiData.roiStatus = mz_data.RoiStatus;
  roiData.spadCount = mz_data.EffectiveSpadRtnCount / 256.f;
  roiData.processedRange_mm = 0;

  // Populate the list of "readings" for this ROIs RangingData for each object detected
  if(mz_data.NumberOfObjectsFound > 0)
  {
    int16_t minDist = 1000; // Assuming max distance of 1000mm
    for(int r = 0; r < mz_data.NumberOfObjectsFound; r++)
    {
      RangeReading reading;
      reading.status = mz_data.RangeData[r].RangeStatus;

      // The following three readings come up in 16.16 fixed point so convert
      reading.signalRate_mcps = ((float)mz_data.RangeData[r].SignalRateRtnMegaCps * (float)(1/(2 << 16)));
      reading.ambientRate_mcps = ((float)mz_data.RangeData[r].AmbientRateRtnMegaCps * (float)(1/(2 << 16)));
      reading.sigma_mm = ((float)mz_data.RangeData[r].SigmaMilliMeter * (float)(1/(2 << 16)));
      reading.rawRange_mm = mz_data.RangeData[r].RangeMilliMeter;

      // For all valid detected objects in this ROI keep track of which one was the closest and
      // use that for the overall processedRange_mm
      if(mz_data.RangeData[r].RangeStatus == VL53L1_RANGESTATUS_RANGE_VALID)
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

// Get the most recent ranging data and parse it to a useable format
int ReadDataFromSensor(RangeDataRaw& rangeData)
{
  int rc = 0;  
  VL53L1_MultiRangingData_t mz_data;
  rc = get_mz_data(&_dev, 1, &mz_data);
  if(rc == 0)
  {
    ParseData(mz_data, rangeData);
  }
  else
  {
    PRINT_NAMED_ERROR("ReadDataFromSensor", "Failed to get mz data %d", rc);
  }
  
  return rc;
}

int ToFSensor::StartRanging(const CommandCallback& callback)
{
  std::lock_guard<std::mutex> lock(_commandLock);
  _commandQueue.push({Command::StartRanging, callback});
  return 0;
}

int ToFSensor::StopRanging(const CommandCallback& callback)
{
  // There is currently an issue with bringing the sensor back online after stopping it.
  // I think calibration or some initial setting must be changing when the sensor is stopped
  // so for now StopRanging is not supported.
  PRINT_NAMED_INFO("ToFSensor.StopRanging.NotImplemented","");
  if(callback != nullptr)
  {
    callback(CommandResult::Success);
  }
  return 0;
      
  
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

void ProcessLoop()
{
  while(!_stopProcessing)
  {
    // Process the next command if there is one
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
            PRINT_NAMED_INFO("ToF.ProcessLoop.StartRanging", "");
            int rc = start_ranging(&_dev);
            if(rc < 0)
            {
              res = ToFSensor::CommandResult::StartRangingFailed;
              break;
            }

            _rangingEnabled = true;
          }
          break;
        case Command::StopRanging:
          {
            PRINT_NAMED_INFO("ToF.ProcessLoop.StopRanging","");
            int rc = stop_ranging(&_dev);
            if(rc < 0)
            {
              res = ToFSensor::CommandResult::StopRangingFailed;
              break;
            }
            
            _rangingEnabled = false;
          }
          break;
        case Command::SetupSensors:
          {
            PRINT_NAMED_INFO("ToF.ProcessLoop.SetupSensors","");
            _rangingEnabled = false;

            int rc = 0;
            // Only attempt to open the device if the file handle is invalid
            if(_dev.platform_data.i2c_file_handle <= 0)
            {
              rc = open_dev(&_dev);
              if(rc < 0)
              {
                res = ToFSensor::CommandResult::OpenDevFailed;
                PRINT_NAMED_ERROR("ToF.ProcessLoop.SetupSensors","Failed to open sensor");
                break;
              }
            }

            // Device is open so set it up for multizone ranging
            rc = setup(&_dev);
            if(rc < 0)
            {
              res = ToFSensor::CommandResult::SetupFailed;
              PRINT_NAMED_ERROR("ToF.ProcessLoop.SetupFailed","Failed to setup sensor");
            }
          }
          break;
        case Command::PerformCalibration:
          {
            PRINT_NAMED_INFO("ToF.ProcessLoop.PerformCalibration","");
            _rangingEnabled = false;
            res = run_calibration(_distanceToCalibTarget_mm, _calibTargetReflectance);
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

    // As long as ranging is enabled, try to read data from the sensor
    if(_rangingEnabled)
    {
      // Note: static is important here in order to preserve previous readings
      // as, typically, only one ROI is read at a time
      static RangeDataRaw data;
      const int rc = ReadDataFromSensor(data);

      // static RangeDataRaw lastValid = data;
      // std::stringstream ss;
      // for(int i = 0; i < 4; i++)
      // {
      //   for(int j = 0; j < 4; j++)
      //   {
      //     if(data.data[i*4 + j].numObjects > 0 && data.data[i*4 + j].readings[0].status == 0)
      //     {
      //       ss << std::setw(7) << (uint32_t)(data.data[i*4 + j].processedRange_mm);
      //       lastValid.data[i*4 + j] = data.data[i*4 + j];
      //     }
      //     else
      //     {
      //       ss << std::setw(7) << (uint32_t)(lastValid.data[i*4 + j].processedRange_mm);
      //     }
      //   }
      //   ss << "\n";
      // }
      // //PRINT_NAMED_ERROR("","%s", ss.str().c_str());
      // printf("Data\n%s\n", ss.str().c_str());

      // Got a valid reading so update our latest data
      if(rc >= 0)
      {
        std::lock_guard<std::mutex> lock(_mutex);
        _dataUpdatedSinceLastGetCall = true;
        _latestData = data;
      }
    }
    // Ranging is not enabled so sleep
    else
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(32));
    }
  }

  stop_ranging(&_dev);
  close_dev(&_dev);
}

bool ToFSensor::IsRanging() const
{
  return _rangingEnabled;
}

bool ToFSensor::IsCalibrating() const
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

bool ToFSensor::IsValidRoiStatus(uint8_t status) const
{
  return (status != VL53L1_ROISTATUS_NOT_VALID);
}

}
}
