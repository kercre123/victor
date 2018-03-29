#ifndef SPI_IMU_H
#define SPI_IMU_H

#include <stdint.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_IMU_SPI_DEVICE "/dev/spidev0.0"

#define IMU_ACC_RANGE 2.0    // g       [2.11.12: acc_range 2 => +- 2g ]
#define IMU_GYRO_RANGE 500.0 // deg/sec [2.11.14: gyr_range 2 => +- 500s]
#define IMU_TEMP_RANGE 64.0 // degC     [2.11.8: 1/pow(2,9) K/LSB.  0x8000/(1<<9)=64]
#define IMU_TEMP_OFFSET 23.0 //degC     [2.11.8: 0x0000 => 23 C]

#define IMU_TICKS_PER_CYCLE (1<<7) //see table 11 of BMI160 datasheet
#define IMU_CYCLES_PER_SEC  (200)
#define NS_PER_IMU_TICK (1.0e9/(IMU_TICKS_PER_CYCLE*IMU_CYCLES_PER_SEC))

#define MAX_16BIT_POSITIVE 0x7FFF

#define RADIANS_PER_DEGREE ((M_PI*2.0)/360.0)
#define MMPS2_PER_GEE    (9810/1.0)

#define IMU_ACCEL_SCALE_G ((double)(IMU_ACC_RANGE)/MAX_16BIT_POSITIVE)
#define IMU_GYRO_SCALE_DPS ((double)(IMU_GYRO_RANGE)/MAX_16BIT_POSITIVE)

#define IMU_TEMP_SCALE_C ((double)(IMU_TEMP_RANGE)/(MAX_16BIT_POSITIVE+1))

#define IMU_TEMP_RAW_TO_C(raw)  ((raw)*(IMU_TEMP_SCALE_C)+IMU_TEMP_OFFSET)

#define IMU_MAX_SAMPLES_PER_READ 3

struct __attribute__((packed)) IMURawData {
   int16_t gyro[3];
   int16_t acc[3];
   uint32_t timestamp;
   int16_t temperature;
};

/** Read up to IMU_MAX_SAMPLES_PER_READ into array of IMURawData structs
 * Returns the number of valid samples read.
 */
int imu_manage(struct IMURawData* raw);

/** Opens the IMU device file handle
 * returns NULL or error message.
 */
const char* imu_open(void);
void imu_close(void);

/** Initalizes IMU device
 */
void imu_init(void);

/** Requests temperature update from IMU
 * Data will be populated into following IMURawData structs on calls to imu_manage
 */
void imu_update_temperature(void);

#define STEADY_CLOCK_TICKS_PER_SECOND 1000000000 //nanosecs
#define TICKS_5MS (STEADY_CLOCK_TICKS_PER_SECOND/200)

uint64_t steady_clock_now(void);
   
#ifdef __cplusplus
} //extern "C"
#endif
   
#endif//SPI_IMU_H
