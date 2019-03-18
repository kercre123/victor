#include <string.h>

#include "common.h"
#include "hardware.h"
#include "power.h"

#include "i2c.h"
#include "messages.h"
#include "lights.h"
#include "flash.h"
#include "comms.h"
#include "opto.h"

using namespace I2C;

// Failure codes
FailureCode Opto::failure = BOOT_FAIL_NONE;
static bool optoActive = false;

// Readback values
static uint16_t cliffSense[4];

#define TARGET(value) sizeof(value), (void*)&value
#define VALUE(value) 1, (void*)value

const I2C_Operation I2C_LOOP[] = {
  { I2C_REG_READ, 0, DROP_SENSOR_ADDRESS, PS_DATA_0, TARGET(cliffSense[3]) },
  { I2C_REG_READ, 1, DROP_SENSOR_ADDRESS, PS_DATA_0, TARGET(cliffSense[2]) },
  { I2C_REG_READ, 2, DROP_SENSOR_ADDRESS, PS_DATA_0, TARGET(cliffSense[1]) },
  { I2C_REG_READ, 3, DROP_SENSOR_ADDRESS, PS_DATA_0, TARGET(cliffSense[0]) },

  { I2C_DONE }
};

void Opto::tick(void) {
  I2C::execute(I2C_LOOP);
}

void Opto::transmit(BodyToHead *payload) {
  memcpy(payload->cliffSense, cliffSense, sizeof(cliffSense));
  payload->failureCode = failure;
  payload->flags =
    (optoActive ? RUNNING_FLAGS_SENSORS_VALID : 0);
}

void Opto::start(void) {
  failure = BOOT_FAIL_NONE;

  I2C::capture();

  // Turn on and configure the drop sensors
  for (int i = 0; i < 4; i++) {
    writeReg(i, DROP_SENSOR_ADDRESS, MAIN_CTRL, 0x01);
    writeReg(i, DROP_SENSOR_ADDRESS, PS_LED, 6 | (5 << 4));
    writeReg(i, DROP_SENSOR_ADDRESS, PS_PULSES, 8);
    writeReg(i, DROP_SENSOR_ADDRESS, PS_MEAS_RATE, 3 | (3 << 3) | 0x40);
    writeReg(i, DROP_SENSOR_ADDRESS, PS_CAN_0, 0);
    writeReg(i, DROP_SENSOR_ADDRESS, PS_CAN_1, 0);
  }

  // Return the i2c bus to the main execution loop
  I2C::release();

  optoActive = true;
}

void Opto::stop(void) {
  optoActive = false;

  // I2C bus is no longer valid (block sensors)
  I2C::capture();
  
  failure = BOOT_FAIL_NONE;

  // Turn on and configure the drop sensors
  for (int i = 0; i < 4; i++) {
    writeReg(i, DROP_SENSOR_ADDRESS, MAIN_CTRL, 0x00);
  }
}

bool Opto::sensorsValid() {
  return optoActive;
}

