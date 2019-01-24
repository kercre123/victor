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

#include "whiskeyToF/tofUserspace_vicos.h"
#include "whiskeyToF/tofCalibration_vicos.h"

#include "util/console/consoleInterface.h"
#include "util/console/consoleSystem.h"
#include "util/logging/logging.h"

#include <thread>
#include <mutex>
#include <queue>
//#include <iomanip>

#ifdef SIMULATOR
#error SIMULATOR should be defined by any target using tof_vicos.cpp
#endif

namespace Anki {
namespace Cozmo {

namespace {

  VL53L1_Dev_t _dev;

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

  bool _dataUpdatedSinceLastGetCall = false;

  // VL53L1_CalibrationData_t _origCalib_left;
  // VL53L1_CalibrationData_t _origCalib_right;

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
  set_calibration_save_path(path);
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
                                  ToFSensor::CommandResult::CalibrateRightFailed :
                                  ToFSensor::CommandResult::Success);
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

void ParseData(VL53L1_MultiRangingData_t& mz_data,
               RangeDataRaw& rangeData)
{
  const int index = mz_data.RoiNumber;

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

void ReadDataFromSensor(RangeDataRaw& rangeData)
{
  int rc = 0;
  
  // for(int i = 0; i < 4; i++)
  // {
  //   for(int j = 0; j < 4; j++)
  //   {
      VL53L1_MultiRangingData_t mz_data;
      rc = get_mz_data(&_dev, 1, &mz_data);
      if(rc == 0)
      {
        ParseData(mz_data, rangeData);
      }
      else
      {
        PRINT_NAMED_ERROR("","FAILED TO GET_MZ_DATA %d\n", rc);
      }
  //   }
  // }
}

RangeDataRaw ReadData()
{
  static RangeDataRaw rangeData;

  ReadDataFromSensor(rangeData);

  return rangeData;
}


int ToFSensor::StartRanging(const CommandCallback& callback)
{
  std::lock_guard<std::mutex> lock(_commandLock);
  _commandQueue.push({Command::LoadCalibration, callback});
  _commandQueue.push({Command::StartRanging, callback});
  return 0;
}

int ToFSensor::StopRanging(const CommandCallback& callback)
{
  // if(callback != nullptr)
  // {
  //   callback(CommandResult::Success);
  // }
  // return 0;
  
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
            //load_calibration(&_dev);
            
            PRINT_NAMED_ERROR("","Command start ranging");
            int rc = start_ranging(&_dev);
            if(rc < 0)
            {
              res = ToFSensor::CommandResult::StartRangingRightFailed;
              break;
            }

            _rangingEnabled = true;
          }
          break;
        case Command::StopRanging:
          {
            PRINT_NAMED_ERROR("","Command stop ranging");
            int rc = stop_ranging(&_dev);
            if(rc < 0)
            {
              res = ToFSensor::CommandResult::StopRangingRightFailed;
              break;
            }
            
            _rangingEnabled = false;
          }
          break;
        case Command::SetupSensors:
          {
            PRINT_NAMED_ERROR("","Command setup sensors");
            _rangingEnabled = false;

            int rc = 0;
            if(_dev.platform_data.i2c_file_handle <= 0)
            {
              rc = open_dev(&_dev);
              if(rc < 0)
              {
                res = ToFSensor::CommandResult::OpenRightDevFailed;
                PRINT_NAMED_ERROR("","FAILED TO OPEN RIGHT");
                break;
              }
            }

            rc = setup(&_dev);
            if(rc < 0)
            {
              res = ToFSensor::CommandResult::SetupRightFailed;
              PRINT_NAMED_ERROR("","FAILED TO SETUP TOF R");
            }
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
            //load_calibration(&_dev);
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

  stop_ranging(&_dev);
  close_dev(&_dev);
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
