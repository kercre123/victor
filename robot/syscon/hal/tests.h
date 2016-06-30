#ifndef TESTS_H
#define TESTS_H

//#define DO_ENCODER_TESTING
//#define DO_MOTOR_TESTING
//#define DO_LIGHTS_TESTING

enum {
  TEST_POWERON       = 0x80,
  TEST_RADIOTX       = 0x81,
  TEST_KILLHEAD      = 0x82,
  TEST_GETVER        = 0x83,
  TEST_GETMOTOR      = 0x85,
  TEST_DROP          = 0x86,
  TEST_RUNMOTOR      = 0x87,
};

#if defined(DO_ENCODER_TESTING) || defined(DO_MOTOR_TESTING) || defined(DO_LIGHTS_TESTING)
  #define RUN_TESTS
#endif

namespace TestFixtures {
  void run();
  void dispatch(uint8_t test, uint8_t param);
}

#endif

