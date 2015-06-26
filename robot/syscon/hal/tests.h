#ifndef TESTS_H
#define TESTS_H

//#define DO_MOTOR_TESTING
//#define DO_GEAR_RATIO_TESTING
//#define DO_FIXED_DISTANCE_TESTING
//#define DO_LIGHTS_TESTING

#if defined(DO_MOTOR_TESTING) || \
    defined(DO_GEAR_RATIO_TESTING) || \
    defined(DO_FIXED_DISTANCE_TESTING) || \
    defined(DO_LIGHTS_TESTING)
#define TESTS_ENABLED

namespace TestFixtures {
  void run();

  void motors();
  void lights();
}
#endif

#endif
