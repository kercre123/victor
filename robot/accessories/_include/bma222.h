// I2C addresses
#define I2C_ADDR      0x18 // 7-bit slave address
#define I2C_ADDR_ALT  0x19 // 7-bit slave address

// IMU Chip IDs
#define CHIPID          0xF8

// Accelerometer Register Map
#define BGW_CHIPID      0x00
#define ACCD_X_LSB      0x02
#define ACCD_X_MSB      0x03
#define ACCD_Y_LSB      0x04
#define ACCD_Y_MSB      0x05
#define ACCD_Z_LSB      0x06
#define ACCD_Z_MSB      0x07
#define ACCD_TEMP       0x08
#define INT_STATUS_0    0x09
#define INT_STATUS_1    0x0A
#define INT_STATUS_2    0x0B
#define INT_STATUS_3    0x0C
#define ACC_FIFO_STATUS     0x0E
#define PMU_RANGE       0x0F
#define PMU_BW          0x10
#define PMU_LPW         0x11
#define PMU_LOW_POWER   0x12
#define ACCD_HBW        0x13
#define BGW_SOFTRESET   0x14
#define INT_EN_0        0x16
#define INT_EN_1        0x17
#define INT_EN_2        0x18
#define INT_MAP_0       0x19
#define INT_MAP_1       0x1A
#define INT_MAP_2       0x1B
#define INT_SRC         0x1E
#define INT_OUT_CTRL    0x20
#define INT_RST_LATCH   0x21
#define INT_0           0x22
#define INT_1           0x23
#define INT_2           0x24
#define INT_3           0x25
#define INT_4           0x26
#define INT_5           0x27
#define INT_6           0x28
#define INT_7           0x29
#define INT_8           0x2A
#define INT_9           0x2B
#define INT_A           0x2C
#define INT_B           0x2D
#define INT_C           0x2E
#define INT_D           0x2F
#define FIFO_CONFIG_0   0x30
#define PMU_SELF_TEST   0x32
#define TRIM_NVM_CTRL   0x33
#define BGW_SPI3_WDT    0x34
#define OFC_CTRL        0x36
#define OFC_SETTING     0x37
#define OFC_OFFSET_X    0x38
#define OFC_OFFSET_Y    0x39
#define OFC_OFFSET_Z    0x3A
#define TRIM_GP0        0x3B
#define TRIM_GP1        0x3C
#define FIFO_CONFIG_1   0x3E
#define FIFO_DATA       0x3F

#define BGW_SOFTRESET_MAGIC   0xB6

// Accelerometer Register values  // XXX check these values
#define RANGE_2G            0x03
#define RANGE_4G            0x05
#define RANGE_8G            0x08
#define RANGE_16G           0x0B

#define BW_7_81             0x08
#define BW_15_63            0x09
#define BW_31_25            0x0A
#define BW_62_5             0x0B
#define BW_125              0x0C
#define BW_250              0x0D
#define BW_500              0x0E
#define BW_1000             0x0F

#define PMU_LOWPOWER_MODE   (1 << 6)   
#define PMU_SUSPEND         (4 << 5)
#define PMU_NORMAL          (0 << 5)

#define FIFO_BYPASS         (0 << 6)
#define FIFO_FIFO           (1 << 6)
#define FIFO_STREAM         (2 << 6)
#define FIFO_WORKAROUND     (3 << 2)
#define FIFO_XYZ            0
#define FIFO_X              1
#define FIFO_Y              2
#define FIFO_Z              3

#define ACC_INT_OPEN_DRAIN  0x0F  // Active high, open drain

#define I2C_ACK  0
#define I2C_NACK 1
#define I2C_READ_BIT  1    
#define I2C_WRITE_BIT 0

#define LPU_DEEP_SUSPEND 32

#define SPI_READ 128
