#ifndef SYSCON_TESTS_H
#define SYSCON_TESTS_H

// Used when sending messages up to head
#include "../../generated/clad/robot/clad/types/fwTestMessages.h"

enum {
  TEST_POWERON       = 0x80,
  TEST_RADIOTX       = 0x81,
  TEST_KILLHEAD      = 0x82,
  TEST_GETVER        = 0x83,
  TEST_PLAYTONE      = 0x84,
  TEST_GETMOTOR      = 0x85,
  TEST_DROP          = 0x86,
  TEST_RUNMOTOR      = 0x87,
  TEST_LIGHT         = 0x88,
  TEST_MOTORSLAM     = 0x89,
  TEST_ADC           = 0x8A,
  TEST_BACKBUTTON    = 0x8B,
  TEST_BACKPULLUP    = 0x8C,
  TEST_ENCODERS      = 0x8D,
};

namespace TestFixtures {
  void dispatch(uint8_t test, uint8_t param);
  void manage();
}

#endif

