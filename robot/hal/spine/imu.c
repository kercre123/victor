#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "platform/platform.h"
#include "anki/cozmo/robot/spi_imu.h"


//#define CONSOLE_DEBUG_PRINTF
#ifdef CONSOLE_DEBUG_PRINTF
#define imu_debug(fmt, args...) printf(fmt, ##args)
#else
#define imu_debug(fmt, args...) (LOGD( fmt, ##args))
#endif

#define REALTIME_CONSOLE_OUTPUT 0

//-*****************************************

//-* BMI160 REGISTERS */
#define CMD (0x7e)
#define STEP_CONF_1 (0x7b)
#define STEP_CONF_0 (0x7a)
#define STEP_CNT_1 (0x79)
#define STEP_CNT_0 (0x78)
#define OFFSET_6 (0x77)
#define OFFSET_5 (0x76)
#define OFFSET_4 (0x75)
#define OFFSET_3 (0x74)
#define OFFSET_2 (0x73)
#define OFFSET_1 (0x72)
#define OFFSET_0 (0x71)
#define NV_CONF (0x70)
#define SELF_TEST (0x6d)
#define PMU_TRIGGER (0x6c)
#define IF_CONF (0x6b)
#define CONF (0x6a)
#define FOC_CONF (0x69)
#define INT_FLAT_1 (0x68)
#define INT_FLAT_0 (0x67)
#define INT_ORIENT_1 (0x66)
#define INT_ORIENT_0 (0x65)
#define INT_TAP_1 (0x64)
#define INT_TAP_0 (0x63)
#define INT_MOTION_3 (0x62)
#define INT_MOTION_2 (0x61)
#define INT_MOTION_1 (0x60)
#define INT_MOTION_0 (0x5f)
#define INT_LOWHIGH_4 (0x5e)
#define INT_LOWHIGH_3 (0x5d)
#define INT_LOWHIGH_2 (0x5c)
#define INT_LOWHIGH_1 (0x5b)
#define INT_LOWHIGH_0 (0x5a)
#define INT_DATA_1 (0x59)
#define INT_DATA_0 (0x58)
#define INT_MAP_2 (0x57)
#define INT_MAP_1 (0x56)
#define INT_MAP_0 (0x55)
#define INT_LATCH (0x54)
#define INT_OUT_CTRL (0x53)
#define INT_EN_2 (0x52)
#define INT_EN_1 (0x51)
#define INT_EN_0 (0x50)
#define MAG_IF_4 (0x4f)
#define MAG_IF_3 (0x4e)
#define MAG_IF_2 (0x4d)
#define MAG_IF_1 (0x4c)
#define MAG_IF_0 (0x4b)

#define FIFO_CONFIG_1 (0x47)
#define FIFO_CONFIG_0 (0x46)
#define FIFO_DOWNS (0x45)

#define MAG_CONF (0x44)
#define GYR_RANGE (0x43)
#define GYR_CONF (0x42)
#define ACC_RANGE (0x41)
#define ACC_CONF (0x40)

#define FIFO_DATA (0x24)
#define FIFO_LENGTH_1 (0x23)
#define FIFO_LENGTH_0 (0x22)
#define TEMPERATURE_1 (0x21)
#define TEMPERATURE_0 (0x20)
#define INT_STATUS_3 (0x1f)
#define INT_STATUS_2 (0x1e)
#define INT_STATUS_1 (0x1d)
#define INT_STATUS_0 (0x1c)
#define STATUS (0x1b)
#define SENSORTIME_2 (0x1a)
#define SENSORTIME_1 (0x19)
#define SENSORTIME_0 (0x18)
#define DATA_19 (0x17)
#define DATA_18 (0x16)
#define DATA_17 (0x15)
#define DATA_16 (0x14)
#define DATA_15 (0x13)
#define DATA_14 (0x12)
#define DATA_13 (0x11)
#define DATA_12 (0x10)
#define DATA_11 (0x0f)
#define DATA_10 (0x0e)
#define DATA_9 (0x0d)
#define DATA_8 (0x0c)
#define DATA_7 (0x0b)
#define DATA_6 (0x0a)
#define DATA_5 (0x09)
#define DATA_4 (0x08)
#define DATA_3 (0x07)
#define DATA_2 (0x06)
#define DATA_1 (0x05)
#define DATA_0 (0x04)
#define PMU_STATUS (0x03)
#define ERR_REG (0x02)
#define CHIP_ID (0x00)


#define TIME_BYTES 3          //SENSORTIME_0..SENSORTIME_2
#define TEMP_AND_LEN_BYTES 4  //TEMPERATURE_0..FIFO_LENGTH_1
#define DATA_BYTES 12         //DATA_0..DATA_11   

// IMU CHIP Configuration
#define ACC_RANGE_2G 0x03          // Full Scale is 2.0G
#define ACC_RATE_200HZ 0x19        // 200Hz
#define GYRO_RANGE_500DPS 0x02
#define GYRO_RATE_200HZ_OVS_4X 0x09    // 200Hz update,  4x oversample
#define INT_DISABLED_OPEN_DRAIN 0x44

#define SET_PMU_MODE 0x10
#define ACC_PMU 0x00
#define GYR_PMU 0x04

#define FIFO_FLUSH (0xB0)

#define DATA_INVALID (0x80)

#define FIFO_FILTER_NO_DOWNSAMPLE 0x88
#define FIFO_WATERMARK_200_ENTRIES 200
#define FIFO_DATA_ACC_GYR         0xC2  // c2 for timestamp?

// SPI DRIVER CONFIGURATION  
static  uint32_t k_spi_mode = 0;
static  uint32_t k_spi_bits = 8;
static  uint32_t k_spi_speed = 15000000;

static int gSPI_fd;

static int16_t m_latest_temperature = 0;

static int spi_setup(int fd, uint32_t key, uint32_t* val)
{
  return (ioctl(fd, key, val) != -1); //true on good.
}

static int spi_transfer(int bytes, const void* data, void* response)
{

  struct spi_ioc_transfer tr = {
    .tx_buf = (unsigned long)data,
    .rx_buf = (unsigned long)response,
    .len = bytes,
    .delay_usecs = 0,
    .speed_hz = k_spi_speed,
    .bits_per_word = k_spi_bits,
  };
  int result = ioctl(gSPI_fd, SPI_IOC_MESSAGE(1), &tr);
  if (result < 1) {
    int errnum  = errno;
    imu_debug("can't send message: ioctl err %d [%s]\n", errnum, strerror(errnum));
    return -1;
  }
  return 0;
}



static int spi_write(int reg, int value)
{
  uint8_t sequence[2] = {reg, value};
  return spi_transfer(2, sequence, NULL);
}

static uint8_t spi_read(int reg)
{
  uint8_t sequence[2] = {0x80 | reg, 0};
  uint8_t response[2] = {0};
  spi_transfer(2, sequence, response);
  return response[1];
}

#define READ_BLOCK_SZ (256) // Arbitrary, how much stack do you want to take up?
//Performs BURST READ: we issue a 'read' command followed by `n` zeros.
// response[1..n] contain the valid data.  response[0] is junk.
static uint8_t spi_read_n(int reg, uint8_t response[], int n_values)
{
  assert(n_values < READ_BLOCK_SZ);
  uint8_t sequence[READ_BLOCK_SZ + 1] = {0x80 | reg, 0};

  return spi_transfer(1 + n_values, sequence, response);
}


static void MicroWait(long microsec)
{
  uint64_t start = steady_clock_now();
  while ((steady_clock_now() - start) < (microsec * 1000)) {
    ;
  }
}


void imu_init()
{
  imu_debug("Initializing IMU");

  //Chip starts in I2C mode.  A rising edge after power-up switches the interface switches to SPI
  //Recommended to do a SPI single -read- access to the ADDRESS 0x7F before the actual communication.
  spi_read(0x7F); 

  spi_write(CMD, SET_PMU_MODE | ACC_PMU | 1); // Power up accelerometer (normal mode)
  MicroWait(4400);   // Datasheet says wait 4ms

  spi_write(CMD, SET_PMU_MODE | GYR_PMU | 1); // Power up gyroscope (normal mode)
  MicroWait(81000);   // Datasheet says wait 80ms

#ifdef IMU_DEBUG
  imu_debug("IMU status after power up: %02x %02x %02x\n", spi_read(CHIP_ID), spi_read(ERR_REG), spi_read(PMU_STATUS));
  //TODO: verify error register?
#endif

  //Configure our sensors
  spi_write(ACC_RANGE, ACC_RANGE_2G);
  spi_write(ACC_CONF, ACC_RATE_200HZ);

  spi_write(GYR_RANGE, GYRO_RANGE_500DPS);
  spi_write(GYR_CONF, GYRO_RATE_200HZ_OVS_4X);

  spi_write(INT_OUT_CTRL, INT_DISABLED_OPEN_DRAIN);


  spi_write(FIFO_DOWNS, FIFO_FILTER_NO_DOWNSAMPLE);  //filtered data w/ no downsampling
  spi_write(FIFO_CONFIG_0, FIFO_WATERMARK_200_ENTRIES); //fifo watermmark level
  spi_write(FIFO_CONFIG_1, FIFO_DATA_ACC_GYR);

  uint8_t id = spi_read(CHIP_ID);
  imu_debug("IMU ID = %d\n", id);

}

void imu_purge(void)
{
  spi_write(CMD, FIFO_FLUSH);  //FIFO FLUSH
}

/* Datasheet notes:
   FIFO  Frame rate is maximum of sensors enabled for fifo in reg x46,x47
   Register 0x45 for downsampilng

   Data retrieved by burst read on 0x24
   Headerless mode: frames are only databytes.  all bytes sensordata.  Must be same ODR (output data rate)

   -- selected by disabling fifo_header in Register (0x46-0x47) FIFO_CONFIG.
   In case of overreading the FIFO, non-valid frames always contain the fixed expression (magic number) 0x80 in the data frame.


   - Overflow loses oldest data.
   - Underflow returns 0x80

   Writing CMD  = 0xB0  => FIFO FLUSH

   0x1B status holds "dataready" flags. Probably not worth checking unless we are running abnormally fast.

   0x22-23 report 12 bit fifo level.

   0x24 triggers burst read.  read out as many bytes as needed.
*/
int imu_manage(struct IMURawData* data)
{
  assert(data != NULL);

  //buffers must be 1 byte longer than actual response, which will start at index 1
  uint8_t rawdata[DATA_BYTES*IMU_MAX_SAMPLES_PER_READ+1];
  memset(rawdata, DATA_INVALID, sizeof(rawdata)); // Prefill frame with invalid so we don't have to branch on read success

  int rc = spi_read_n(FIFO_DATA, (uint8_t*)rawdata, DATA_BYTES*IMU_MAX_SAMPLES_PER_READ);
  if(rc < 0)
  {
    return rc;
  }
  
  int i;
  for (i=0; i < IMU_MAX_SAMPLES_PER_READ; i++) {
    uint8_t* sample_data = rawdata + 1 + (DATA_BYTES*i);
    if (sample_data[0] == 0 && sample_data[1] == DATA_INVALID) {
      // Fifo overread
      return i;
    }
    else {
      data[i].gyro[0] =  ((sample_data[ 5] << 8) | sample_data[ 4]);
      data[i].gyro[1] =  ((sample_data[ 3] << 8) | sample_data[ 2]);
      data[i].gyro[2] = -((sample_data[ 1] << 8) | sample_data[ 0]);
      data[i].acc[0]  =  ((sample_data[11] << 8) | sample_data[10]);
      data[i].acc[1]  =  ((sample_data[ 9] << 8) | sample_data[ 8]);
      data[i].acc[2]  = -((sample_data[ 7] << 8) | sample_data[ 6]);
      data[i].temperature = m_latest_temperature;
      
#if REALTIME_CONSOLE_OUTPUT
      {
         static int rate_limiter=0;
         if (++rate_limiter == 10) {
         rate_limiter = 0;

         printf("%d %f %f %f %f %f %f %f\r",
                data[i].timestamp,
                data[i].gyro[0]*IMU_GYRO_SCALE_DPS,
                data[i].gyro[1]*IMU_GYRO_SCALE_DPS,
                data[i].gyro[2]*IMU_GYRO_SCALE_DPS,
                data[i].acc[0]*IMU_ACCEL_SCALE_G,
                data[i].acc[1]*IMU_ACCEL_SCALE_G,
                data[i].acc[2]*IMU_ACCEL_SCALE_G,
                IMU_TEMP_RAW_TO_C(data[i].temperature)
            );
         }
      }
#endif 
    }
  }
  
  // If we got here then we had more data than we should so flush the IMU FIFO
  imu_purge();
  return IMU_MAX_SAMPLES_PER_READ;
}

void imu_update_temperature(void)
{
  uint8_t temp_buf[3];
  if (spi_read_n(TEMPERATURE_0, temp_buf, 2) < 0) {
    printf("IMU: Failed to read temp\n");
    return; // TODO Handle error
  }
  m_latest_temperature = temp_buf[1] | (temp_buf[2] << 8);
}


const char* imu_open(void)
{
  static uint32_t mode = 0;
  static uint32_t bits = 8;
  static uint32_t speed = 0;

  /* Open */
  gSPI_fd = open(DEFAULT_IMU_SPI_DEVICE, O_RDWR);
  if (gSPI_fd < 0) {
    return "IMU: Can't open SPI DEVICE "DEFAULT_IMU_SPI_DEVICE;
  }

  /* Setup */
  /*
   * spi mode
   */
  if (!(spi_setup(gSPI_fd, SPI_IOC_WR_MODE, &k_spi_mode) &&
        spi_setup(gSPI_fd, SPI_IOC_RD_MODE, &mode) &&
        spi_setup(gSPI_fd, SPI_IOC_WR_BITS_PER_WORD, &k_spi_bits) &&
        spi_setup(gSPI_fd, SPI_IOC_RD_BITS_PER_WORD, &bits) &&
        spi_setup(gSPI_fd, SPI_IOC_WR_MAX_SPEED_HZ, &k_spi_speed) &&
        spi_setup(gSPI_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed))) {
    return "IMU: Can't configure SPI device";
  }
  if (k_spi_mode != mode || k_spi_bits != bits || k_spi_speed != speed) {
    return "IMU: configuration not successful";
  }
  return NULL;
}

void imu_close(void)
{
  close(gSPI_fd);
}
