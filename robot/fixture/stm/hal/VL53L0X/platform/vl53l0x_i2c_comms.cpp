#include <string.h>
#include "vl53l0x_i2c_platform.h"
#include "vl53l0x_def.h"
#include "common.h"
#include "console.h"
#include "dut_i2c.h"
#include "timer.h"

namespace I2C = DUT_I2C;
using namespace I2C;

int VL53L0X_i2c_init(void) {
  I2C::init();
  return VL53L0X_ERROR_NONE;
}

int VL53L0X_write_multi(uint8_t deviceAddress, uint8_t index, uint8_t *pdata, uint32_t count)
{
  if( !I2C::multiOp(I2C_REG_WRITE, 0, deviceAddress, index, count, pdata) )
    return VL53L0X_ERROR_NONE;
  /*
  const int max_write_size = 40;
  uint8_t buf[1+max_write_size];
  if( count<1 || !pdata ) return VL53L0X_ERROR_INVALID_PARAMS;
  if( count>max_write_size ) return VL53L0X_ERROR_NOT_SUPPORTED;
  
  buf[0] = index; //register offset
  memcpy( buf+1, pdata, count );
  
  if( transfer(deviceAddress, 1+count,buf, 0,NULL) == 0 )
    return VL53L0X_ERROR_NONE;
  */
  return VL53L0X_ERROR_CONTROL_INTERFACE;
}

int VL53L0X_read_multi(uint8_t deviceAddress, uint8_t index, uint8_t *pdata, uint32_t count) {
  if( !I2C::multiOp(I2C_REG_READ, 0, deviceAddress, index, count, pdata) )
    return VL53L0X_ERROR_NONE;
  /*
  if( count<1 || !pdata )
    return VL53L0X_ERROR_INVALID_PARAMS;
  
  if( transfer(deviceAddress, 1,&index, count, (uint8_t*)pdata) == 0 )
    return VL53L0X_ERROR_NONE;
  */
  return VL53L0X_ERROR_CONTROL_INTERFACE;
}

int VL53L0X_write_byte(uint8_t deviceAddress, uint8_t index, uint8_t data) {
  return VL53L0X_write_multi(deviceAddress, index, &data, 1);
}

int VL53L0X_write_word(uint8_t deviceAddress, uint8_t index, uint16_t data) {
  uint8_t buff[2];
  buff[1] = data & 0xFF;
  buff[0] = data >> 8;
  return VL53L0X_write_multi(deviceAddress, index, buff, 2);
}

int VL53L0X_write_dword(uint8_t deviceAddress, uint8_t index, uint32_t data) {
  uint8_t buff[4];

  buff[3] = data & 0xFF;
  buff[2] = data >> 8;
  buff[1] = data >> 16;
  buff[0] = data >> 24;

  return VL53L0X_write_multi(deviceAddress, index, buff, 4);
}

int VL53L0X_read_byte(uint8_t deviceAddress, uint8_t index, uint8_t *data) {
  return VL53L0X_read_multi(deviceAddress, index, data, 1);
}

int VL53L0X_read_word(uint8_t deviceAddress, uint8_t index, uint16_t *data) {
  uint8_t buff[2];
  int r = VL53L0X_read_multi(deviceAddress, index, buff, 2);

  uint16_t tmp;
  tmp = buff[0];
  tmp <<= 8;
  tmp |= buff[1];
  *data = tmp;

  return r;
}

int VL53L0X_read_dword(uint8_t deviceAddress, uint8_t index, uint32_t *data) {
  uint8_t buff[4];
  int r = VL53L0X_read_multi(deviceAddress, index, buff, 4);

  uint32_t tmp;
  tmp = buff[0];
  tmp <<= 8;
  tmp |= buff[1];
  tmp <<= 8;
  tmp |= buff[2];
  tmp <<= 8;
  tmp |= buff[3];

  *data = tmp;

  return r;
}
