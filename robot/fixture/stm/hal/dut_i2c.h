#ifndef __DUT_I2C_H
#define __DUT_I2C_H

#include <stdint.h>

//-----------------------------------------------------------
//        API
//-----------------------------------------------------------

#ifdef __cplusplus

namespace DUT_I2C
{
  void init();
  void deinit();
  
  //return non-0 on error
  int transfer(uint8_t address, int wlen, uint8_t *wdat, int rlen, uint8_t *out_rdat);
}

//from syscon's i2c.h:
enum I2C_Op {
  I2C_DONE,
  I2C_HOLD,
  I2C_REG_READ,
  I2C_REG_WRITE,
  I2C_REG_WRITE_VALUE
};
/*struct I2C_Operation {
  I2C_Op op;
  uint8_t channel;
  uint8_t slave;
  uint8_t reg;
  int length;
  void* data;
};*/

//wrappers for syscon i2c driver interface
namespace DUT_I2C
{
  //void init(void);
  //void execute(const I2C_Operation* cmd);
  //void capture();
  //void release();

  bool multiOp(I2C_Op func, uint8_t channel, uint8_t slave, uint8_t reg, int size, void* data);
  bool writeReg(uint8_t channel, uint8_t slave, uint8_t reg, uint8_t data);
  bool writeReg16(uint8_t channel, uint8_t slave, uint8_t reg, uint16_t data);
  bool writeReg32(uint8_t channel, uint8_t slave, uint8_t reg, uint32_t data);
  int readReg(uint8_t channel, uint8_t slave, uint8_t reg);
  int readReg16(uint8_t channel, uint8_t slave, uint8_t reg);
}

#endif /* __cplusplus */

#endif //__DUT_I2C_H

