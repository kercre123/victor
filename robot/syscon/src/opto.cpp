#include <string.h>

#include "common.h"
#include "hardware.h"
#include "power.h"

#include "opto.h"
#include "messages.h"
#include "lights.h"
#include "flash.h"
#include "comms.h"

//#define DISABLE_TOF

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

struct I2C_BulkWrite {
  uint8_t reg;
  uint8_t data;
};

struct SequenceStepEnables
{
  bool tcc, msrc, dss, pre_range, final_range;
};

struct SequenceStepTimeouts
{
  uint16_t pre_range_vcsel_period_pclks, final_range_vcsel_period_pclks;

  uint16_t msrc_dss_tcc_mclks, pre_range_mclks, final_range_mclks;
  uint32_t msrc_dss_tcc_us,    pre_range_us,    final_range_us;
};

#define TARGET(value) sizeof(value), (void*)&value

static bool aborted_setup = false;
static BootFail failure;

// Readback values
static uint16_t cliffSense[4];
static uint8_t stop_variable;
static uint8_t tof_status;
static uint16_t tof_reading;
static uint16_t tof_signal_rate;  // fixed point 9.7
static uint16_t tof_ambient_rate; // fixed point 9.7
static uint16_t tof_spad_count;   // fixed point 8.8
static uint32_t measurement_timing_budget_us;

static const I2C_Operation* i2c_hold = (I2C_Operation*) 0xDEADFACE;
static const I2C_Operation i2c_loop[] = {
  { I2C_REG_READ, 0, DROP_SENSOR_ADDRESS, PS_DATA_0, TARGET(cliffSense[3]) },
  { I2C_REG_READ, 1, DROP_SENSOR_ADDRESS, PS_DATA_0, TARGET(cliffSense[2]) },
  { I2C_REG_READ, 2, DROP_SENSOR_ADDRESS, PS_DATA_0, TARGET(cliffSense[1]) },
  { I2C_REG_READ, 3, DROP_SENSOR_ADDRESS, PS_DATA_0, TARGET(cliffSense[0]) },
  #ifndef DISABLE_TOF
  { I2C_REG_READ, 0, TOF_SENSOR_ADDRESS, RESULT_RANGE_STATUS, TARGET(tof_status) },
  { I2C_REG_READ, 0, TOF_SENSOR_ADDRESS, RESULT_RANGE_STATUS + 10, TARGET(tof_reading) },
  { I2C_REG_READ, 0, TOF_SENSOR_ADDRESS, RESULT_RANGE_STATUS + 6, TARGET(tof_signal_rate) },
  { I2C_REG_READ, 0, TOF_SENSOR_ADDRESS, RESULT_RANGE_STATUS + 8, TARGET(tof_ambient_rate) },
  { I2C_REG_READ, 0, TOF_SENSOR_ADDRESS, RESULT_RANGE_STATUS + 2, TARGET(tof_spad_count) },
  #endif
  { I2C_DONE }
};

// Current I2C state
static const I2C_Operation* volatile i2c_op = i2c_hold;
static uint8_t* i2c_target;
static volatile uint8_t total_bytes;
static bool address_sent;
typedef void (*callback)(void);

static void setupChannel() {
  // Move our pins around
  if (i2c_op->channel & 1) {
    SDA1::mode(MODE_INPUT);
    SDA2::mode(MODE_ALTERNATE);
  } else {
    SDA2::mode(MODE_INPUT);
    SDA1::mode(MODE_ALTERNATE);
  }

  if (i2c_op->channel & 2) {
    SCL1::mode(MODE_INPUT);
    SCL2::mode(MODE_ALTERNATE);
  } else {
    SCL2::mode(MODE_INPUT);
    SCL1::mode(MODE_ALTERNATE);
  }

  i2c_target = (uint8_t*) i2c_op->data;
  address_sent = false;
  total_bytes = 0;  // used to see if we aborted early on sync functions
}

static void kickOff() {
  // We have not selected our save address
  I2C2->ICR = ~0; // Clear all flags

  switch (i2c_op->op) {
    case I2C_HOLD:
      i2c_op = i2c_hold;
      return ;
    case I2C_DONE:
      i2c_op = NULL;
      return ;
    case I2C_REG_READ:
      setupChannel();

      // Enable NBYTE interrupt (Read specific)
      I2C2->CR1 |= I2C_CR1_TCIE;
      I2C2->CR2   = (1 << 16)
                  | I2C_CR2_START
                  | i2c_op->slave
                  ;
      break ;
    case I2C_REG_WRITE_VALUE:
      i2c_target = (uint8_t*) &i2c_op->data;
    case I2C_REG_WRITE:
      setupChannel();

      I2C2->CR1 &= ~I2C_CR1_TCIE;
      I2C2->CR2   = ((i2c_op->length + 1) << 16)
                  | I2C_CR2_START
                  | I2C_CR2_AUTOEND
                  | i2c_op->slave
                  ;
      break ;
  }
}

extern "C" void I2C2_IRQHandler(void) {
  // Operation finished (possibly aborted early)
  if (I2C2->ISR & I2C_ISR_STOPF) {
    i2c_op++;
    kickOff();
    return ;
  }

  // Register selection complete (read only)
  if (I2C2->ISR & I2C_ISR_TC) {
    I2C2->CR2   = (i2c_op->length << 16)
                | I2C_CR2_START
                | I2C_CR2_RD_WRN
                | I2C_CR2_AUTOEND
                | i2c_op->slave
                ;
    return ;
  }

  // Read register not empty
  if (I2C2->ISR & I2C_ISR_RXNE) {
    *(i2c_target++) = I2C2->RXDR;
    total_bytes++;
  }

  // Transmission register empty
  else if (I2C2->ISR & I2C_ISR_TXE) {
    if (address_sent) {
      total_bytes++;
      I2C2->TXDR = *(i2c_target++);
    } else {
      address_sent = true;
      I2C2->TXDR = i2c_op->reg;
    }
  }
}

static bool multiOp(I2C_Op func, uint8_t channel, uint8_t slave, uint8_t reg, int size, void* data) {
  if (aborted_setup) {
    return true;
  }

  int max_retries = 15;
  do {
    // Welp, something went wrong, we should just give up
    if (max_retries-- == 0) {
      Comms::enqueue(PAYLOAD_BOOT_FAIL, &failure, sizeof(failure));
      aborted_setup = true;
      return true;
    }

    I2C_Operation opTable[] = {
      { func, channel, slave, reg, size, data },
      { I2C_HOLD }
    };
    i2c_op = opTable;
    kickOff();
    while (i2c_op != i2c_hold) __wfi();
  } while (total_bytes < size);

  return false;
}

static bool writeReg(uint8_t channel, uint8_t slave, uint8_t reg, uint8_t data) {
  return multiOp(I2C_REG_WRITE, channel, slave, reg, sizeof(data), &data);
}

static bool writeReg16(uint8_t channel, uint8_t slave, uint8_t reg, uint16_t data) {
  return multiOp(I2C_REG_WRITE, channel, slave, reg, sizeof(data), &data);
}

static int readReg(uint8_t channel, uint8_t slave, uint8_t reg) {
  uint8_t data;
  if (multiOp(I2C_REG_READ, channel, slave, reg, sizeof(data), &data)) return -1;
  return data;
}

static int readReg16(uint8_t channel, uint8_t slave, uint8_t reg) {
  uint16_t data;
  if (multiOp(I2C_REG_READ, channel, slave, reg, sizeof(data), &data)) return -1;
  return data;
}

#define decodeVcselPeriod(reg_val)      (((reg_val) + 1) << 1)
#define calcMacroPeriod(vcsel_period_pclks) ((((uint32_t)2304 * (vcsel_period_pclks) * 1655) + 500) / 1000)

static uint16_t encodeTimeout(uint16_t timeout_mclks)
{
  uint32_t ls_byte = 0;
  uint16_t ms_byte = 0;

  if (timeout_mclks > 0)
  {
    ls_byte = timeout_mclks - 1;

    while ((ls_byte & 0xFFFFFF00) > 0)
    {
      ls_byte >>= 1;
      ms_byte++;
    }

    return (ms_byte << 8) | (ls_byte & 0xFF);
  }
  else { return 0; }
}

static uint16_t decodeTimeout(uint16_t reg_val)
{
  return (uint16_t)((reg_val & 0x00FF) <<
         (uint16_t)((reg_val & 0xFF00) >> 8)) + 1;
}

static uint32_t timeoutMclksToMicroseconds(uint16_t timeout_period_mclks, uint8_t vcsel_period_pclks)
{
  uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);

  return ((timeout_period_mclks * macro_period_ns) + (macro_period_ns / 2)) / 1000;
}

static uint32_t timeoutMicrosecondsToMclks(uint32_t timeout_period_us, uint8_t vcsel_period_pclks)
{
  uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);

  return (((timeout_period_us * 1000) + (macro_period_ns / 2)) / macro_period_ns);
}

static void getSequenceStepEnables(SequenceStepEnables * enables)
{
  uint8_t sequence_config = readReg(0, TOF_SENSOR_ADDRESS, SYSTEM_SEQUENCE_CONFIG);

  enables->tcc          = (sequence_config >> 4) & 0x1;
  enables->dss          = (sequence_config >> 3) & 0x1;
  enables->msrc         = (sequence_config >> 2) & 0x1;
  enables->pre_range    = (sequence_config >> 6) & 0x1;
  enables->final_range  = (sequence_config >> 7) & 0x1;
}

static void getSequenceStepTimeouts(SequenceStepEnables const * enables, SequenceStepTimeouts * timeouts)
{
  timeouts->pre_range_vcsel_period_pclks = decodeVcselPeriod(readReg(0, TOF_SENSOR_ADDRESS, PRE_RANGE_CONFIG_VCSEL_PERIOD));

  timeouts->msrc_dss_tcc_mclks = readReg(0, TOF_SENSOR_ADDRESS, MSRC_CONFIG_TIMEOUT_MACROP) + 1;
  timeouts->msrc_dss_tcc_us =
    timeoutMclksToMicroseconds(timeouts->msrc_dss_tcc_mclks,
                               timeouts->pre_range_vcsel_period_pclks);

  timeouts->pre_range_mclks =
    decodeTimeout(readReg16(0, TOF_SENSOR_ADDRESS, PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
  timeouts->pre_range_us =
    timeoutMclksToMicroseconds(timeouts->pre_range_mclks,
                               timeouts->pre_range_vcsel_period_pclks);

  timeouts->final_range_vcsel_period_pclks = decodeVcselPeriod(readReg(0, TOF_SENSOR_ADDRESS, FINAL_RANGE_CONFIG_VCSEL_PERIOD));

  timeouts->final_range_mclks =
    decodeTimeout(readReg16(0, TOF_SENSOR_ADDRESS, FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI));

  if (enables->pre_range)
  {
    timeouts->final_range_mclks -= timeouts->pre_range_mclks;
  }

  timeouts->final_range_us =
    timeoutMclksToMicroseconds(timeouts->final_range_mclks,
                               timeouts->final_range_vcsel_period_pclks);
}

static void performSingleRefCalibration(uint8_t vhv_init_byte)
{
  writeReg(0, TOF_SENSOR_ADDRESS, SYSRANGE_START, 0x01 | vhv_init_byte); // VL53L0X_REG_SYSRANGE_MODE_START_STOP

  while ((readReg(0, TOF_SENSOR_ADDRESS, RESULT_INTERRUPT_STATUS) & 0x07) == 0) ;

  writeReg(0, TOF_SENSOR_ADDRESS, SYSTEM_INTERRUPT_CLEAR, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, SYSRANGE_START, 0x00);
}

static bool setMeasurementTimingBudget(uint32_t budget_us)
{
  SequenceStepEnables enables;
  SequenceStepTimeouts timeouts;

  uint16_t const StartOverhead      = 1320; // note that this is different than the value in get_
  uint16_t const EndOverhead        = 960;
  uint16_t const MsrcOverhead       = 660;
  uint16_t const TccOverhead        = 590;
  uint16_t const DssOverhead        = 690;
  uint16_t const PreRangeOverhead   = 660;
  uint16_t const FinalRangeOverhead = 550;

  uint32_t const MinTimingBudget = 20000;

  if (budget_us < MinTimingBudget) { return false; }

  uint32_t used_budget_us = StartOverhead + EndOverhead;

  getSequenceStepEnables(&enables);
  getSequenceStepTimeouts(&enables, &timeouts);

  if (enables.tcc)
  {
    used_budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
  }

  if (enables.dss)
  {
    used_budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
  }
  else if (enables.msrc)
  {
    used_budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
  }

  if (enables.pre_range)
  {
    used_budget_us += (timeouts.pre_range_us + PreRangeOverhead);
  }

  if (enables.final_range)
  {
    used_budget_us += FinalRangeOverhead;

    // "Note that the final range timeout is determined by the timing
    // budget and the sum of all other timeouts within the sequence.
    // If there is no room for the final range timeout, then an error
    // will be set. Otherwise the remaining time will be applied to
    // the final range."

    if (used_budget_us > budget_us)
    {
      // "Requested timeout too big."
      return false;
    }

    uint32_t final_range_timeout_us = budget_us - used_budget_us;

    // set_sequence_step_timeout() begin
    // (SequenceStepId == VL53L0X_SEQUENCESTEP_FINAL_RANGE)

    // "For the final range timeout, the pre-range timeout
    //  must be added. To do this both final and pre-range
    //  timeouts must be expressed in macro periods MClks
    //  because they have different vcsel periods."

    uint16_t final_range_timeout_mclks =
      timeoutMicrosecondsToMclks(final_range_timeout_us,
                                 timeouts.final_range_vcsel_period_pclks);

    if (enables.pre_range)
    {
      final_range_timeout_mclks += timeouts.pre_range_mclks;
    }

    writeReg16(0, TOF_SENSOR_ADDRESS, FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI,
      encodeTimeout(final_range_timeout_mclks));

    // set_sequence_step_timeout() end

    measurement_timing_budget_us = budget_us; // store for internal reuse
  }
  return true;
}

// Get the measurement timing budget in microseconds
// based on VL53L0X_get_measurement_timing_budget_micro_seconds()
// in us
static uint32_t getMeasurementTimingBudget(void)
{
  SequenceStepEnables enables;
  SequenceStepTimeouts timeouts;

  uint16_t const StartOverhead     = 1910; // note that this is different than the value in set_
  uint16_t const EndOverhead        = 960;
  uint16_t const MsrcOverhead       = 660;
  uint16_t const TccOverhead        = 590;
  uint16_t const DssOverhead        = 690;
  uint16_t const PreRangeOverhead   = 660;
  uint16_t const FinalRangeOverhead = 550;

  // "Start and end overhead times always present"
  uint32_t budget_us = StartOverhead + EndOverhead;

  getSequenceStepEnables(&enables);
  getSequenceStepTimeouts(&enables, &timeouts);

  if (enables.tcc)
  {
    budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
  }

  if (enables.dss)
  {
    budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
  }
  else if (enables.msrc)
  {
    budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
  }

  if (enables.pre_range)
  {
    budget_us += (timeouts.pre_range_us + PreRangeOverhead);
  }

  if (enables.final_range)
  {
    budget_us += (timeouts.final_range_us + FinalRangeOverhead);
  }

  measurement_timing_budget_us = budget_us; // store for internal reuse
  return budget_us;
}

static void initHardware() {
  aborted_setup = false;

  // Turn on and configure the drop sensors
  for (int i = 0; i < 4; i++) {
    failure.code = BOOT_FAIL_CLIFF1 + i;
    writeReg(i, DROP_SENSOR_ADDRESS, MAIN_CTRL, 0x01);
    writeReg(i, DROP_SENSOR_ADDRESS, PS_LED, 6 | (5 << 4));
    writeReg(i, DROP_SENSOR_ADDRESS, PS_PULSES, 8);
    writeReg(i, DROP_SENSOR_ADDRESS, PS_MEAS_RATE, 4 | (3 << 3) | 0x40);
    writeReg(i, DROP_SENSOR_ADDRESS, PS_CAN_0, 0);
    writeReg(i, DROP_SENSOR_ADDRESS, PS_CAN_1, 0);
  }

  #ifndef DISABLE_TOF
  failure.code = BOOT_FAIL_TOF;
  // Turn on TOF sensor
  // "Set I2C standard mode"
  writeReg(0, TOF_SENSOR_ADDRESS, 0x88, 0x00);

  writeReg(0, TOF_SENSOR_ADDRESS, 0x80, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x00, 0x00);
  stop_variable = readReg(0, TOF_SENSOR_ADDRESS, 0x91);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x00, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x80, 0x00);

  // disable SIGNAL_RATE_MSRC (bit 1) and SIGNAL_RATE_PRE_RANGE (bit 4) limit checks
  writeReg(0, TOF_SENSOR_ADDRESS, MSRC_CONFIG_CONTROL, readReg(0, TOF_SENSOR_ADDRESS, MSRC_CONFIG_CONTROL) | 0x12);

  // set final range signal rate limit to 0.25 MCPS (million counts per second)
  writeReg16(0, TOF_SENSOR_ADDRESS, FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, 32);

  writeReg(0, TOF_SENSOR_ADDRESS, SYSTEM_SEQUENCE_CONFIG, 0xFF);

  // Get spad info
  uint8_t spad_count;
  bool spad_type_is_aperture;
  {
    writeReg(0, TOF_SENSOR_ADDRESS, 0x80, 0x01);
    writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x01);
    writeReg(0, TOF_SENSOR_ADDRESS, 0x00, 0x00);

    writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x06);
    writeReg(0, TOF_SENSOR_ADDRESS, 0x83, readReg(0, TOF_SENSOR_ADDRESS, 0x83) | 0x04);
    writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x07);
    writeReg(0, TOF_SENSOR_ADDRESS, 0x81, 0x01);

    writeReg(0, TOF_SENSOR_ADDRESS, 0x80, 0x01);

    writeReg(0, TOF_SENSOR_ADDRESS, 0x94, 0x6b);
    writeReg(0, TOF_SENSOR_ADDRESS, 0x83, 0x00);
    while (readReg(0, TOF_SENSOR_ADDRESS, 0x83) == 0x00) ;
    writeReg(0, TOF_SENSOR_ADDRESS, 0x83, 0x01);
    uint8_t tmp = readReg(0, TOF_SENSOR_ADDRESS, 0x92);

    spad_count = tmp & 0x7f;
    spad_type_is_aperture = (tmp >> 7) & 0x01;

    writeReg(0, TOF_SENSOR_ADDRESS, 0x81, 0x00);
    writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x06);
    writeReg(0, TOF_SENSOR_ADDRESS, 0x83, readReg(0, TOF_SENSOR_ADDRESS, 0x83  & ~0x04));
    writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x01);
    writeReg(0, TOF_SENSOR_ADDRESS, 0x00, 0x01);

    writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x00);
    writeReg(0, TOF_SENSOR_ADDRESS, 0x80, 0x00);
  }

  // The SPAD map (RefGoodSpadMap) is read by VL53L0X_get_info_from_device() in
  // the API, but the same data seems to be more easily readable from
  // GLOBAL_CONFIG_SPAD_ENABLES_REF_0 through _6, so read it from there
  uint8_t ref_spad_map[6];
  multiOp(I2C_REG_READ, 0, TOF_SENSOR_ADDRESS, GLOBAL_CONFIG_SPAD_ENABLES_REF_0, sizeof(ref_spad_map), ref_spad_map);

  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C);
  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4);

  uint8_t first_spad_to_enable = spad_type_is_aperture ? 12 : 0; // 12 is the first aperture spad
  uint8_t spads_enabled = 0;

  for (uint8_t i = 0; i < 48; i++)
  {
    if (i < first_spad_to_enable || spads_enabled == spad_count)
    {
      // This bit is lower than the first one that should be enabled, or
      // (reference_spad_count) bits have already been enabled, so zero this bit
      ref_spad_map[i / 8] &= ~(1 << (i % 8));
    }
    else if ((ref_spad_map[i / 8] >> (i % 8)) & 0x1)
    {
      spads_enabled++;
    }
  }

  multiOp(I2C_REG_WRITE, 0, TOF_SENSOR_ADDRESS, GLOBAL_CONFIG_SPAD_ENABLES_REF_0, sizeof(ref_spad_map), ref_spad_map);

  // DefaultTuningSettings from vl53l0x_tuning.h
  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x00, 0x00);

  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x09, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x10, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x11, 0x00);

  writeReg(0, TOF_SENSOR_ADDRESS, 0x24, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x25, 0xFF);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x75, 0x00);

  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x4E, 0x2C);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x48, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x30, 0x20);

  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x30, 0x09);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x54, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x31, 0x04);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x32, 0x03);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x40, 0x83);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x46, 0x25);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x60, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x27, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x50, 0x06);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x51, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x52, 0x96);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x56, 0x08);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x57, 0x30);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x61, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x62, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x64, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x65, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x66, 0xA0);

  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x22, 0x32);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x47, 0x14);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x49, 0xFF);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x4A, 0x00);

  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x7A, 0x0A);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x7B, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x78, 0x21);

  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x23, 0x34);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x42, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x44, 0xFF);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x45, 0x26);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x46, 0x05);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x40, 0x40);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x0E, 0x06);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x20, 0x1A);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x43, 0x40);

  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x34, 0x03);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x35, 0x44);

  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x31, 0x04);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x4B, 0x09);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x4C, 0x05);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x4D, 0x04);

  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x44, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x45, 0x20);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x47, 0x08);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x48, 0x28);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x67, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x70, 0x04);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x71, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x72, 0xFE);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x76, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x77, 0x00);

  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x0D, 0x01);

  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x80, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x01, 0xF8);

  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x8E, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x00, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x80, 0x00);

  // "Set interrupt config to new sample ready"
  writeReg(0, TOF_SENSOR_ADDRESS, SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
  writeReg(0, TOF_SENSOR_ADDRESS, GPIO_HV_MUX_ACTIVE_HIGH, readReg(0, TOF_SENSOR_ADDRESS, GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10); // active low
  writeReg(0, TOF_SENSOR_ADDRESS, SYSTEM_INTERRUPT_CLEAR, 0x01);

  measurement_timing_budget_us = getMeasurementTimingBudget();

  // "Disable MSRC and TCC by default"
  // MSRC = Minimum Signal Rate Check
  // TCC = Target CentreCheck

  writeReg(0, TOF_SENSOR_ADDRESS, SYSTEM_SEQUENCE_CONFIG, 0xE8);

  // "Recalculate timing budget"
  setMeasurementTimingBudget(measurement_timing_budget_us);

  writeReg(0, TOF_SENSOR_ADDRESS, SYSTEM_SEQUENCE_CONFIG, 0x01);
  performSingleRefCalibration(0x40);

  // -- VL53L0X_perform_phase_calibration()
  writeReg(0, TOF_SENSOR_ADDRESS, SYSTEM_SEQUENCE_CONFIG, 0x02);
  performSingleRefCalibration(0x00);

  // "restore the previous Sequence Config"
  writeReg(0, TOF_SENSOR_ADDRESS, SYSTEM_SEQUENCE_CONFIG, 0xE8);

  // Start back to back mode
  writeReg(0, TOF_SENSOR_ADDRESS, 0x80, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x00, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x91, stop_variable);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x00, 0x01);
  writeReg(0, TOF_SENSOR_ADDRESS, 0xFF, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, 0x80, 0x00);
  writeReg(0, TOF_SENSOR_ADDRESS, SYSRANGE_START, 0x02); // VL53L0X_REG_SYSRANGE_MODE_BACKTOBACK
  #endif

  // Return the i2c bus to the main execution loop
  i2c_op = NULL;
}

void Opto::init(void) {
  // Disable the perf at boot
  I2C2->CR1 = 0;
  I2C2->TIMINGR = 0
                | (5 << 20)   // Data setup time
                | (15 << 16)  // Data hold time
                | (30 << 8)   // SCL high
                | (63)        // SCL low
                ;

  I2C2->CR1 = 0
            | I2C_CR1_ANFOFF
            | I2C_CR1_RXIE
            | I2C_CR1_STOPIE
            | I2C_CR1_TXIE
            ;

  // Configure I/O
  SCL1::alternate(1);
  SCL1::speed(SPEED_HIGH);
  SCL1::pull(PULL_NONE);

  SDA1::alternate(1);
  SDA1::speed(SPEED_HIGH);
  SDA1::pull(PULL_NONE);

  SCL2::alternate(1);
  SCL2::speed(SPEED_HIGH);
  SCL2::pull(PULL_NONE);

  SDA2::alternate(1);
  SDA2::speed(SPEED_HIGH);
  SDA2::pull(PULL_NONE);

  // Enable the I2C perfs
  I2C2->CR1 |= I2C_CR1_PE;

  NVIC_SetPriority(I2C2_IRQn, PRIORITY_I2C_TRANSMIT);
  NVIC_EnableIRQ(I2C2_IRQn);

  initHardware();
}

void Opto::stop(void) {
  // TODO
}

void Opto::tick(void) {
  // The I2C perf is busy
  if (i2c_op != NULL) {
    return ;
  }

  i2c_op = i2c_loop;
  kickOff();
}

void Opto::transmit(BodyToHead *payload) {
  memcpy(payload->cliffSense, cliffSense, sizeof(cliffSense));
  payload->proximity.rangeMM = tof_reading;
  payload->proximity.rangeStatus = tof_status;
  payload->proximity.signalRate = tof_signal_rate;
  payload->proximity.ambientRate = tof_ambient_rate;
  payload->proximity.spadCount = tof_spad_count;
}

