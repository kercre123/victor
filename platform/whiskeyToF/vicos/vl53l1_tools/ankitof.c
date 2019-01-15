/**
 *@file  ankitof.c Reference implementation of VL53L1 sensor client for 16 zone imaging.
 *@author Daniel Casner <daniel@anki.com>
 *
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <linux/input.h>

/* local bare driver top api file */
#include "vl53l1_def.h"
/* our driver kernel interface */
#include "stmvl53l1_if.h"
#include "stmvl53l1_internal_if.h"

#define SPAD_COLS (16)  ///< What's in the sensor
#define SPAD_ROWS (16)  ///< What's in the sensor
#define SPAD_MIN_ROI (4) ///< Minimum ROI size in spads
#define MAX_ROWS (SPAD_ROWS / SPAD_MIN_ROI)
#define MAX_COLS (SPAD_COLS / SPAD_MIN_ROI)

#define error(fmt, ...)  fprintf(stderr, "[E] " fmt"\n", ##__VA_ARGS__)
#define warn(fmt, ...)  fprintf(stderr, "[W] " fmt"\n", ##__VA_ARGS__)
#define return_if_not(cond, ret, fmt, ...) do { if (!(cond)) { \
  fprintf(stderr, "[E] " fmt "\n", ##__VA_ARGS__); \
  return ret; \
}} while(0)
#if 0
#define print_and_pause(fmt, ...) do { fprintf(stderr, "[P] " fmt "\n", ##__VA_ARGS__); getc(stdin); } while(0)
#else
#define print_and_pause(fmt, ...) (void)0
#endif

/// Open VL53L1 device from /dev by device number
int open_dev(const int dev_no) {
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


/// Setup 4x4 multi-zone imaging
int setup(void) {
  int fd = -1;
  int rc = 0;

  print_and_pause("open dev");
  fd = open_dev(0);
  return_if_not(fd >= 0, -1, "Could not open VL53L1 device");

  // Stop all ranging so we can change settings
  print_and_pause("stop ranging");
  rc = stop_ranging(fd);
  return_if_not(rc == 0, -1, "ioctl error stopping ranging: %d", errno);

  // Switch to multi-zone scanning mode
  print_and_pause("Switch to multi-zone scanning");
  rc = preset_mode_set(fd, VL53L1_PRESETMODE_MULTIZONES_SCANNING);
  return_if_not(rc == 0, -1, "ioctl error setting preset_mode: %d", errno);

  // Setup ROIs
  print_and_pause("Setup ROI grid");
  rc = setup_roi_grid(fd, 4, 4);
  return_if_not(rc == 0, -1, "ioctl error setting up preset grid: %d", errno);

  // Setup timing budget
  print_and_pause("set timing budget");
  rc = timing_budget_set(fd, 16*1000);
  return_if_not(rc == 0, -1, "ioctl error setting timing budged: %d", errno);

  // Set distance mode
  print_and_pause("set distance mode");
  rc = distance_mode_set(fd, VL53L1_DISTANCEMODE_SHORT);
  return_if_not(rc == 0, -1, "ioctl error setting distance mode: %d", errno);

  // Start the sensor
  print_and_pause("start ranging");
  rc = start_ranging(fd);
  return_if_not(rc == 0, -1, "ioctl error starting ranging: %d", errno);

  return fd;
}


/// Get multi-zone ranging data measurements.
int get_mz_data(const int dev, const int blocking, VL53L1_MultiRangingData_t *data) {
  return ioctl(dev, blocking ? VL53L1_IOCTL_MZ_DATA_BLOCKING : VL53L1_IOCTL_MZ_DATA, data);
}


int main(int argc, char *argv[]) {
  VL53L1_MultiRangingData_t mz_data;
  int dev, rc, i, j;

  dev = setup();

  if (dev < 0) return -1;

  print_and_pause("get_mz_data loop");
  for (i=0; i<10; ++i) {
    rc = get_mz_data(dev, 1, &mz_data);
    if (rc == 0) {
      printf("sc = %02d\troi = %02d\tnof = %02d\tst = %02X\n", mz_data.StreamCount,
                                                       mz_data.RoiNumber,
                                                       mz_data.NumberOfObjectsFound,
                                                       mz_data.RoiStatus);
      if (mz_data.NumberOfObjectsFound > 0) {
        printf("\tObjects: ")
        for (j=0; j<mz_data.NumberOfObjectsFound; ++i) {
          printf("%dmm\t", mz_data.RangeData[j].RangeMilliMeter);
        }
        printf("\n");
      }
    }
    else {
      error("ioctl error getting mz data: %d\t%d", rc, errno);
    }
  }

  stop_ranging(dev);
  close(dev);
  return 0;
}
