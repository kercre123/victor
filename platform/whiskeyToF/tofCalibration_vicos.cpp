
#include "whiskeyToF/tofCalibration_vicos.h"
#include "whiskeyToF/tofUserspace_vicos.h"
#include "whiskeyToF/tofError_vicos.h"

#include "whiskeyToF/vicos/vl53l1/core/inc/vl53l1_api.h"

#include "util/logging/logging.h"

#include <sys/stat.h>
#include <errno.h>

namespace
{
  std::string _savePath = "";
}

// --------------------Set/Get Calibration Data--------------------

int get_zone_calibration_data(VL53L1_Dev_t* dev, VL53L1_ZoneCalibrationData_t& calib)
{
  return VL53L1_GetZoneCalibrationData(dev, &calib);
}

int set_calibration_data(VL53L1_Dev_t* dev, VL53L1_CalibrationData_t& calib)
{
  return VL53L1_SetCalibrationData(dev, &calib);
}

int set_zone_calibration_data(VL53L1_Dev_t* dev, VL53L1_ZoneCalibrationData_t& calib)
{
  return VL53L1_SetZoneCalibrationData(dev, &calib);
}

int perform_refspad_calibration(VL53L1_Dev_t* dev)
{
  return VL53L1_PerformRefSpadManagement(dev);  
}


// --------------------Save/Load Calibration Data--------------------
void set_calibration_save_path(const std::string& path)
{
  _savePath = path;
}

// Path is expected to end in "/"
int save_calibration_to_disk(void* calib,
                             ssize_t size,
                             const std::string& path,
                             const std::string& filename)
{
  PRINT_NAMED_ERROR("","SAVING %s%s", path.c_str(), filename.c_str());
  char buf[128] = {0};
  sprintf(buf, "%s%s", path.c_str(), filename.c_str());
  int rc = -1;
  FILE* f = fopen(buf, "w+");
  if(f != nullptr)
  {
    const size_t count = 1;
    rc = fwrite(calib, size, count, f);
    if(rc != count)
    {
      PRINT_NAMED_ERROR("","fwrite to %s failed with %d", buf, ferror(f));
      rc = -1;
    }
    else
    {
      rc = 0;
    }
    (void)fclose(f);
  }
  else
  {
    PRINT_NAMED_ERROR("","FAILED TO OPEN FILE %u", errno);
  }
  return rc;

}

int save_calibration_to_disk(VL53L1_CalibrationData_t& data,
                             const std::string& meta = "")
{
  const std::string name = "tof" + meta + ".bin";
  (void)save_calibration_to_disk(&data, sizeof(data), _savePath, name);
  return save_calibration_to_disk(&data, sizeof(data), "/factory/", name);
}

int save_calibration_to_disk(VL53L1_ZoneCalibrationData_t& data,
                             const std::string& meta = "")
{
  const std::string name = "tofZone" + meta + ".bin";
  (void)save_calibration_to_disk(&data, sizeof(data), _savePath, name);
  return save_calibration_to_disk(&data, sizeof(data), "/factory/", name);
}

int load_calibration_from_disk(void* calib,
                               ssize_t size,
                               const std::string& path)
{
  int rc = -1;
  FILE* f = fopen(path.c_str(), "r");
  if(f != nullptr)
  {
    const size_t count = 1;
    rc = fread(calib, size, count, f);
    if(rc != count)
    {
      PRINT_NAMED_ERROR("","fread failed");
      rc = -1;
    }
    else
    {
      rc = 0;
    }
    (void)fclose(f);
  }
  return rc;
}

int load_calibration(VL53L1_Dev_t* dev)
{
  PRINT_NAMED_ERROR("","Load calibration");

  VL53L1_CalibrationData_t calib;
  memset(&calib, 0, sizeof(calib));

  int rc = load_calibration_from_disk(&calib, sizeof(calib), "/factory/tof.bin");
  if(rc < 0)
  {
    PRINT_NAMED_ERROR("","Attempting to load old calib format");
    rc = load_calibration_from_disk(&calib, sizeof(calib), "/factory/tof_right.bin");
    if(rc < 0)
    {
      PRINT_NAMED_ERROR("","Failed to load tof calibration");
    }
  }

  //calib.gain_cal.histogram_ranging_gain_factor = (uint16_t)(0.87f * (float)(2 << 11));
  
  rc = set_calibration_data(dev, calib);
  if(rc < 0)
  {
    PRINT_NAMED_ERROR("","Failed to set tof calibration");
  }


  VL53L1_ZoneCalibrationData_t calibZone;
  memset(&calibZone, 0, sizeof(calibZone));
  
  struct stat st;
  rc = stat("/factory/tofZone_right.bin", &st);
  if(rc == 0)
  {
    // DVT1 zone calibration was saved as the "stmvl531_zone_calibration_data_t" structure
    // from the kernel driver. This structure contains a u32 id field before the VL53L1_ZoneCalibrationData_t.
    // So we need to recreate the layout of that struct in order to properly load the saved calibration data.
    struct {
      uint32_t garbage;
      VL53L1_ZoneCalibrationData_t data;
    } buf;

    PRINT_NAMED_ERROR("","Loading zone data as old format");
    rc = load_calibration_from_disk(&buf, sizeof(buf), "/factory/tofZone_right.bin");
    if(rc == 0)
    {
      calibZone = buf.data;
    }
  }
  else
  {
    PRINT_NAMED_ERROR("","Loading zone data");
    rc = load_calibration_from_disk(&calibZone, sizeof(calibZone), "/factory/tofZone.bin");
  }

  if(rc < 0)
  {
    PRINT_NAMED_ERROR("","Failed to load tof zone calibration");
  }
  else
  {
    rc = set_zone_calibration_data(dev, calibZone);
    return_if_error(rc, "Failed to set tof zone calibration");
  }

  return rc;
}


// --------------------Reference SPAD Calibration--------------------
int run_refspad_calibration(VL53L1_Dev_t* dev)
{
  VL53L1_CalibrationData_t calib;
  memset(&calib, 0, sizeof(calib));
  VL53L1_Error rc = VL53L1_GetCalibrationData(dev, &calib);
  return_if_error(rc, "Get calibration data failed");

  rc = perform_refspad_calibration(dev);
  return_if_error(rc, "RefSPAD calibration failed");

  memset(&calib, 0, sizeof(calib));
  rc = VL53L1_GetCalibrationData(dev, &calib);
  return_if_error(rc, "Get calibration data failed");

  rc = save_calibration_to_disk(calib);
  return_if_error(rc, "Save calibration to disk failed");

  rc = set_calibration_data(dev, calib);
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
  // Note: Was using NO_TARGET mode before so need to make sure we are in the correct preset mode before calling
  return VL53L1_PerformXTalkCalibration(dev, VL53L1_XTALKCALIBRATIONMODE_SINGLE_TARGET);
}

int run_xtalk_calibration(VL53L1_Dev_t* dev)
{
  VL53L1_CalibrationData_t calib;
  memset(&calib, 0, sizeof(calib));
  
  VL53L1_Error rc = VL53L1_GetCalibrationData(dev, &calib);
  return_if_error(rc, "1 Get calibration data failed");

  rc = perform_xtalk_calibration(dev);

  bool noXtalk = false;
  if(rc == VL53L1_ERROR_XTALK_EXTRACTION_NO_SAMPLE_FAIL)
  {
    PRINT_NAMED_ERROR("","NO CROSSTALK FOUND");
    noXtalk = true;
  }
  
  memset(&calib, 0, sizeof(calib));
  rc = VL53L1_GetCalibrationData(dev, &calib);
  return_if_error(rc, "2 Get calibration data failed");

  // If there was no crosstalk calibration needed then zero-out xtalk calibration data
  // before setting it
  if(noXtalk)
  {
    zero_xtalk_calibration(calib);
  }
    
  rc = save_calibration_to_disk(calib);
  return_if_error(rc, "Save calibration to disk failed");

  rc = set_calibration_data(dev, calib);
  return_if_error(rc, "Set calibration data failed");
  
  return rc;
}

// --------------------Offset Calibration--------------------
int perform_offset_calibration(VL53L1_Dev_t* dev, uint32_t dist_mm, float reflectance)
{
  VL53L1_Error rc = VL53L1_SetOffsetCalibrationMode(dev, VL53L1_OFFSETCALIBRATIONMODE_MULTI_ZONE);
  return_if_error(rc, "SetOffsetCalibrationMode failed");
  
  return VL53L1_PerformOffsetCalibration(dev, dist_mm, (FixPoint1616_t)(reflectance * (2 << 16)));
}

int run_offset_calibration(VL53L1_Dev_t* dev, uint32_t distanceToTarget_mm, float targetReflectance)
{
  VL53L1_CalibrationData_t calib;
  memset(&calib, 0, sizeof(calib));

  int rc = VL53L1_GetCalibrationData(dev, &calib);
  return_if_error(rc, "1 Get calibration data failed");

  rc = setup_roi_grid(dev, 4, 4);
  return_if_error(rc, "ioctl error setting up preset grid");

  rc = perform_offset_calibration(dev, distanceToTarget_mm, targetReflectance);
  return_if_error(rc, "offset calibration failed");

  memset(&calib, 0, sizeof(calib));
  rc = VL53L1_GetCalibrationData(dev, &calib);
  return_if_error(rc, "2 Get calibration data failed");

  rc = save_calibration_to_disk(calib);
  return_if_error(rc, "Save calibration to disk failed");

  rc = set_calibration_data(dev, calib);
  return_if_error(rc, "Set calibration data failed");

  VL53L1_ZoneCalibrationData_t calibZone;
  memset(&calibZone, 0, sizeof(calibZone));
  rc = get_zone_calibration_data(dev, calibZone);
  return_if_error(rc, "Get zone calib failed");
  
  rc = save_calibration_to_disk(calibZone);
  return_if_error(rc, "Save zone calibration to disk failed");

  rc = set_zone_calibration_data(dev, calibZone);
  return_if_error(rc, "Set zone calibration data failed");

  return rc;
}

int perform_calibration(VL53L1_Dev_t* dev, uint32_t dist_mm, float reflectance)
{
  // Stop all ranging so we can change settings
  PRINT_NAMED_ERROR("","stop ranging\n");
  VL53L1_Error rc = VL53L1_StopMeasurement(dev);
  return_if_error(rc, "ioctl error stopping ranging");

  // Have the device reset when ranging is stopped
  // PRINT_NAMED_ERROR("","reset on stop\n");
  // rc = reset_on_stop_set(dev, 1);
  // return_if_not(rc == 0, -1, "ioctl error reset on stop: %d", errno);

  // Switch to multi-zone scanning mode
  PRINT_NAMED_ERROR("","Switch to multi-zone scanning\n");
  rc = VL53L1_SetPresetMode(dev, VL53L1_PRESETMODE_MULTIZONES_SCANNING);
  return_if_error(rc, "ioctl error setting preset_mode");

  // Setup ROIs
  PRINT_NAMED_ERROR("","Setup ROI grid\n");
  rc = setup_roi_grid(dev, 4, 4);
  return_if_error(rc, "ioctl error setting up preset grid");

  // Setup timing budget
  PRINT_NAMED_ERROR("","set timing budget\n");
  rc = VL53L1_SetMeasurementTimingBudgetMicroSeconds(dev, 8*2000);
  return_if_error(rc, "ioctl error setting timing budged");

  // Set distance mode
  PRINT_NAMED_ERROR("","set distance mode\n");
  rc = VL53L1_SetDistanceMode(dev, VL53L1_DISTANCEMODE_SHORT);
  return_if_error(rc, "ioctl error setting distance mode");

  // Set output mode
  PRINT_NAMED_ERROR("","set output mode\n");
  rc = VL53L1_SetOutputMode(dev, VL53L1_OUTPUTMODE_STRONGEST);
  return_if_error(rc, "ioctl error setting distance mode");

  PRINT_NAMED_ERROR("","Enable live xtalk\n");
  rc = VL53L1_SetXTalkCompensationEnable(dev, 0);
  return_if_error(rc, "ioctl error setting live xtalk");

  VL53L1_CalibrationData_t calib;
  memset(&calib, 0, sizeof(calib));
  set_calibration_data(dev, calib);
  
  rc = VL53L1_GetCalibrationData(dev, &calib);
  return_if_error(rc, "1 Get calibration data failed");

  rc = save_calibration_to_disk(calib, "Orig");
  return_if_error(rc, "Save calibration to disk failed %d", errno);

  rc = run_refspad_calibration(dev);
  return_if_error(rc, "perform_calibration: run_refspad_calibration");
  
  rc = run_xtalk_calibration(dev);
  return_if_error(rc, "perform_calibration: run_xtalk_calibration");

  rc = run_offset_calibration(dev, dist_mm, reflectance);
  return_if_error(rc, "perform_calibration: run_offset_calibration");

  return rc;
}





