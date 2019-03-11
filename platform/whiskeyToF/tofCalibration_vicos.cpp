/**
 * File: tofCalibration_vicos.cpp
 *
 * Author: Al Chaussee
 * Created: 1/24/2019
 *
 * Description: ToF calibration related functions
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#include "whiskeyToF/tofCalibration_vicos.h"
#include "whiskeyToF/tofUserspace_vicos.h"
#include "whiskeyToF/tofError_vicos.h"

#include "whiskeyToF/vicos/vl53l1/core/inc/vl53l1_api.h"

#include "util/logging/logging.h"

#include "anki/cozmo/shared/factory/emr.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

namespace
{
  const char* const kEMRPath = "/dev/block/bootdevice/by-name/emr";
  const uint32_t kCalibMagic1 = 0x746F6673;
  const uint32_t kCalibMagic2 = 0x77736b79;
  const uint32_t kZoneCalibOffset = sizeof(kCalibMagic1) + sizeof(VL53L1_CalibrationData_t);
}

// --------------------Save/Load Calibration Data--------------------

int open_calib_file(int openFlags, off_t offset)
{
  int fd = open(kEMRPath, openFlags);
  if(fd == -1)
  {
    PRINT_NAMED_ERROR("ToFCalibration.open_calib_faile.FailedToOpenBlockDevice",
                      "%d", errno);
    return -1;
  }

  // ToF calibration is offset by 8mb in the block device
  printf("writing start %ld\n", Anki::Cozmo::Factory::kToFCalibOffset + offset);
  const off_t seek = lseek(fd, Anki::Cozmo::Factory::kToFCalibOffset + offset, SEEK_SET);
  if(seek == -1)
  {
    printf("seek fail\n");
    PRINT_NAMED_ERROR("NVStorageComponent.OpenCameraCalibFile.FailedToSeek",
                      "%d", errno);
    return -1;
  }

  return fd;
}

int save_calibration_to_disk(void* calib,
                             ssize_t size,
                             off_t offset,
                             const uint32_t magic)
{
  int rc = 0;
  int fd = open_calib_file(O_WRONLY, offset);
  if(fd < 0)
  {
    printf("fail to open %d\n", errno);
    PRINT_NAMED_ERROR("save_calibration_to_disk", "Failed to open emr %d %d", fd, errno);
    return -1;
  }

  const size_t kNumBytesForCalib = sizeof(magic) + size;
  uint8_t buf[kNumBytesForCalib];
  *((uint32_t*)buf) = magic;
  memcpy(buf+sizeof(magic), calib, size);

  ssize_t numBytesWritten = write(fd, buf, kNumBytesForCalib);
  if(numBytesWritten == -1)
  {
    printf("write faill\n");
    PRINT_NAMED_ERROR("ToFCalibration.save_calibration_to_disk.FailedToWriteBlockDevice",
                      "%d", errno);

    rc = -1;
  }
  else if(numBytesWritten != kNumBytesForCalib)
  {
    printf("not enough write\n");
    PRINT_NAMED_ERROR("ToFCalibration.save_calibration_to_disk.FailedToWriteBlockDevice",
                      "Only wrote %zd bytes instead of %d",
                      numBytesWritten,
                      kNumBytesForCalib);
    rc = -1;
  }

  int res = close(fd);
  if(res == -1)
  {
    printf("seek fail\n");
    PRINT_NAMED_ERROR("ToFCalibration.save_calibration_to_disk.FailedToCloseBlockDevice",
                      "%d", errno);
    rc = -1;
  }

  sync();

  return rc;
}

int save_calibration_to_disk(VL53L1_CalibrationData_t& data)
{
  return save_calibration_to_disk(&data, sizeof(data), 0, kCalibMagic1);
}

int save_calibration_to_disk(VL53L1_ZoneCalibrationData_t& data)
{
  // Zone calibration comes after regular calibration data so it must be offset
  return save_calibration_to_disk(&data,
                                  sizeof(data),
                                  kZoneCalibOffset,
                                  kCalibMagic2);
}

int load_calibration_from_disk(VL53L1_CalibrationData_t& calibData,
                               VL53L1_ZoneCalibrationData_t& zoneCalibData)
{
  int fd = open_calib_file(O_RDONLY, 0);
  if(fd < 0)
  {
    PRINT_NAMED_ERROR("ToFCalibration.load_calibration_from_disk.CalibOpenFail",
                      "%d",
                      errno);
    return -1;
  }

  static_assert(sizeof(kCalibMagic1) == sizeof(kCalibMagic2),
                "Magic1 and Magic2 have different sizes");
  const uint32_t kSizeOfMagic = sizeof(kCalibMagic1);
  const uint32_t kNumBytesForCalib = kSizeOfMagic + sizeof(calibData) + kSizeOfMagic + sizeof(zoneCalibData);
  const uint32_t kNumBytesToRead = 4096; // Read a whole page due to this being a block device
  static_assert(kNumBytesToRead >= kNumBytesForCalib, "Bytes to read smaller than size of camera calibration");

  uint8_t buf[kNumBytesToRead];
  uint8_t* bufPtr = (uint8_t*)(&buf);
  ssize_t numBytesRead = 0;
  do
  {
    numBytesRead += read(fd, buf + numBytesRead, kNumBytesToRead - numBytesRead);
    if(numBytesRead == -1)
    {
      printf("read\n");
      PRINT_NAMED_ERROR("ToFCalibration.load_calibration_from_disk.FailedToReadCalibration",
                      "%d", errno);
      (void)close(fd);
      return -1;
    }
  }
  while(numBytesRead < kNumBytesToRead);

  (void)close(fd);

  static_assert(std::is_same<decltype(kCalibMagic1), const uint32_t>::value,
                "CalibMagic1 must be const uint32_t");
  static_assert(std::is_same<decltype(kCalibMagic2), const uint32_t>::value,
                "CalibMagic2 must be const uint32_t");
  const uint32_t dataMagic = (uint32_t)(*(uint32_t*)buf);
  if(dataMagic != kCalibMagic1)
  {
    printf("magic 1 0x%x\n", dataMagic);
    PRINT_NAMED_ERROR("ToFCalibration.load_calibration_from_disk.CalibMagicMismatch",
                      "Read magic 0x%x and expected magic 0x%x do not match",
                      dataMagic,
                      kCalibMagic1);
    return -1;
  }

  memcpy(&calibData, bufPtr + sizeof(dataMagic), sizeof(calibData));
  bufPtr += (sizeof(dataMagic) + sizeof(calibData));

  const uint32_t zoneMagic = (uint32_t)(*(uint32_t*)bufPtr);
  if(zoneMagic != kCalibMagic2)
  {
    printf("magic 2 0%x\n", zoneMagic);
    PRINT_NAMED_ERROR("ToFCalibration.load_calibration_from_disk.CalibZoneMagicMismatch",
                      "Read magic 0x%x and expected magic 0x%x do not match",
                      zoneMagic,
                      kCalibMagic2);
    return -1;
  }

  memcpy(&zoneCalibData, bufPtr + sizeof(zoneMagic), sizeof(zoneCalibData));

  return 0;
}

int load_calibration(VL53L1_Dev_t* dev)
{
  PRINT_NAMED_INFO("load_calibration", "Loading calibration");

  VL53L1_CalibrationData_t calib;
  memset(&calib, 0, sizeof(calib));

  VL53L1_ZoneCalibrationData_t calibZone;
  memset(&calibZone, 0, sizeof(calibZone));

  int rc = load_calibration_from_disk(calib, calibZone);
  if(rc < 0)
  {
    PRINT_NAMED_ERROR("ToFCalibration.load_calibration.Fail",
                      "Failed to load calibration from disk");
    return rc;
  }

  rc = VL53L1_SetCalibrationData(dev, &calib);
  return_if_error(rc, "Failed to set tof calibration");

  rc = VL53L1_SetZoneCalibrationData(dev, &calibZone);
  return_if_error(rc, "Failed to set tof zone calibration");

  return rc;
}


// --------------------Reference SPAD Calibration--------------------
int run_refspad_calibration(VL53L1_Dev_t* dev)
{
  VL53L1_CalibrationData_t calib;
  memset(&calib, 0, sizeof(calib));

  VL53L1_Error rc = VL53L1_GetCalibrationData(dev, &calib);
  return_if_error(rc, "Get calibration data failed");

  rc = VL53L1_PerformRefSpadManagement(dev);
  return_if_error(rc, "RefSPAD calibration failed");

  memset(&calib, 0, sizeof(calib));
  rc = VL53L1_GetCalibrationData(dev, &calib);
  return_if_error(rc, "Get calibration data failed");

  rc = save_calibration_to_disk(calib);
  return_if_error(rc, "Save calibration to disk failed");

  rc = VL53L1_SetCalibrationData(dev, &calib);
  return_if_error(rc, "Set calibration data failed");

  return rc;
}


// --------------------Crosstalk Calibration--------------------
void zero_xtalk_calibration(VL53L1_CalibrationData_t& calib)
{
  calib.customer.algo__crosstalk_compensation_plane_offset_kcps = 0;
  calib.customer.algo__crosstalk_compensation_x_plane_gradient_kcps = 0;
  calib.customer.algo__crosstalk_compensation_y_plane_gradient_kcps = 0;
  memset(&calib.xtalkhisto, 0, sizeof(calib.xtalkhisto));
}

int perform_xtalk_calibration(VL53L1_Dev_t* dev)
{
  // Make sure we are in the correct preset mode
  (void)VL53L1_SetPresetMode(dev, VL53L1_PRESETMODE_MULTIZONES_SCANNING);
  return VL53L1_PerformXTalkCalibration(dev, VL53L1_XTALKCALIBRATIONMODE_SINGLE_TARGET);
}

int run_xtalk_calibration(VL53L1_Dev_t* dev)
{
  VL53L1_CalibrationData_t calib;
  memset(&calib, 0, sizeof(calib));

  VL53L1_Error rc = VL53L1_GetCalibrationData(dev, &calib);
  return_if_error(rc, "run_xtalk_calibration: get calibration data failed");

  rc = perform_xtalk_calibration(dev);

  bool noXtalk = false;
  if(rc == VL53L1_ERROR_XTALK_EXTRACTION_NO_SAMPLE_FAIL)
  {
    PRINT_NAMED_INFO("run_xtalk_calibration","No crosstalk found");
    noXtalk = true;
  }

  memset(&calib, 0, sizeof(calib));
  rc = VL53L1_GetCalibrationData(dev, &calib);
  return_if_error(rc, "run_xtalk_calibration: get calibration data failed");

  // If there was no crosstalk calibration needed then zero-out xtalk calibration data
  // before setting it
  if(noXtalk)
  {
    zero_xtalk_calibration(calib);
  }

  rc = save_calibration_to_disk(calib);
  return_if_error(rc, "run_xtalk_calibration: Save calibration to disk failed");

  rc = VL53L1_SetCalibrationData(dev, &calib);
  return_if_error(rc, "run_xtalk_calibration: Set calibration data failed");

  return rc;
}

// --------------------Offset Calibration--------------------
int perform_offset_calibration(VL53L1_Dev_t* dev, uint32_t dist_mm, float reflectance)
{
  VL53L1_Error rc = VL53L1_SetOffsetCalibrationMode(dev, VL53L1_OFFSETCALIBRATIONMODE_MULTI_ZONE);
  return_if_error(rc, "perform_offset_calibration: SetOffsetCalibrationMode failed");

  return VL53L1_PerformOffsetCalibration(dev, dist_mm, (FixPoint1616_t)(reflectance * (2 << 16)));
}

int run_offset_calibration(VL53L1_Dev_t* dev, uint32_t distanceToTarget_mm, float targetReflectance)
{
  VL53L1_CalibrationData_t calib;
  memset(&calib, 0, sizeof(calib));

  int rc = VL53L1_GetCalibrationData(dev, &calib);
  return_if_error(rc, "run_offset_calibration: Get calibration data failed");

  rc = setup_roi_grid(dev, 4, 4);
  return_if_error(rc, "run_offset_calibration: error setting up roi grid");

  rc = perform_offset_calibration(dev, distanceToTarget_mm, targetReflectance);
  return_if_error(rc, "run_offset_calibration: offset calibration failed");

  memset(&calib, 0, sizeof(calib));
  rc = VL53L1_GetCalibrationData(dev, &calib);
  return_if_error(rc, "run_offset_calibration: Get calibration data failed");

  rc = save_calibration_to_disk(calib);
  return_if_error(rc, "run_offset_calibration: Save calibration to disk failed");

  rc = VL53L1_SetCalibrationData(dev, &calib);
  return_if_error(rc, "run_offset_calibration: Set calibration data failed");

  // Offset calibration populates the ZoneCalibrationData struct when configured for multizones
  // so we also need to save that calibration data
  VL53L1_ZoneCalibrationData_t calibZone;
  memset(&calibZone, 0, sizeof(calibZone));

  rc = VL53L1_GetZoneCalibrationData(dev, &calibZone);
  return_if_error(rc, "run_offset_calibration: Get zone calib failed");

  rc = save_calibration_to_disk(calibZone);
  return_if_error(rc, "run_offset_calibration: Save zone calibration to disk failed");

  rc = VL53L1_SetZoneCalibrationData(dev, &calibZone);
  return_if_error(rc, "run_offset_calibration: Set zone calibration data failed");

  return rc;
}

int perform_calibration(VL53L1_Dev_t* dev, uint32_t dist_mm, float reflectance)
{
  // Stop all ranging so we can change settings
  VL53L1_Error rc = VL53L1_StopMeasurement(dev);
  return_if_error(rc, "perform_calibration: error stopping ranging");

  // Switch to multi-zone scanning mode
  rc = VL53L1_SetPresetMode(dev, VL53L1_PRESETMODE_MULTIZONES_SCANNING);
  return_if_error(rc, "perform_calibration: error setting preset_mode");

  // Setup ROIs
  rc = setup_roi_grid(dev, 4, 4);
  return_if_error(rc, "perform_calibration: error setting up roi grid");

  // Setup timing budget
  rc = VL53L1_SetMeasurementTimingBudgetMicroSeconds(dev, 8*2000);
  return_if_error(rc, "perform_calibration: error setting timing budged");

  // Set distance mode
  rc = VL53L1_SetDistanceMode(dev, VL53L1_DISTANCEMODE_SHORT);
  return_if_error(rc, "perform_calibration: error setting distance mode");

  // Set output mode
  rc = VL53L1_SetOutputMode(dev, VL53L1_OUTPUTMODE_STRONGEST);
  return_if_error(rc, "perform_calibration: error setting distance mode");

  // Disable xtalk compensation
  rc = VL53L1_SetXTalkCompensationEnable(dev, 0);
  return_if_error(rc, "perform_calibration: error setting live xtalk");

  VL53L1_CalibrationData_t calib;
  memset(&calib, 0, sizeof(calib));
  VL53L1_SetCalibrationData(dev, &calib);

  rc = VL53L1_GetCalibrationData(dev, &calib);
  return_if_error(rc, "perform_calibration: Get calibration data failed");

  // rc = save_calibration_to_disk(calib);
  // return_if_error(rc, "perform_calibration: Save calibration to disk failed");

  rc = run_refspad_calibration(dev);
  return_if_error(rc, "perform_calibration: run_refspad_calibration");

  rc = run_xtalk_calibration(dev);
  return_if_error(rc, "perform_calibration: run_xtalk_calibration");

  rc = run_offset_calibration(dev, dist_mm, reflectance);
  return_if_error(rc, "perform_calibration: run_offset_calibration");

  return rc;
}
