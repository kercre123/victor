#ifndef __I2C_H
#define __I2C_H

#include "messages.h"

enum I2C_Op {
  I2C_DONE,
  I2C_HOLD,
  I2C_REG_READ,
  I2C_REG_WRITE,
  I2C_REG_WRITE_VALUE
};

struct I2C_Operation {
  I2C_Op op;
  uint8_t channel;
  uint8_t slave;
  uint8_t reg;
  int length;
  void* data;
};

namespace I2C {
	void init(void);
  void execute(const I2C_Operation* cmd);
  void capture();
  void release();

	bool multiOp(I2C_Op func, uint8_t channel, uint8_t slave, uint8_t reg, int size, void* data);
	bool writeReg(uint8_t channel, uint8_t slave, uint8_t reg, uint8_t data);
	bool writeReg16(uint8_t channel, uint8_t slave, uint8_t reg, uint16_t data);
  bool writeReg32(uint8_t channel, uint8_t slave, uint8_t reg, uint32_t data);
	int readReg(uint8_t channel, uint8_t slave, uint8_t reg);
	int readReg16(uint8_t channel, uint8_t slave, uint8_t reg);
}


#endif
