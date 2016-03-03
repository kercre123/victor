#ifndef TESTS_H
#define TESTS_H

//#define DO_ENCODER_TESTING
//#define DO_MOTOR_TESTING
//#define DO_LIGHTS_TESTING

#if defined(DO_ENCODER_TESTING) || defined(DO_MOTOR_TESTING) || defined(DO_LIGHTS_TESTING)
  #define RUN_TESTS
#endif

namespace TestFixtures {
  void run();
}

#endif

