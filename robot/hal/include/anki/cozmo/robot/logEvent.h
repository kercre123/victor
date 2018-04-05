
#ifndef ANKI_COZMO_ROBOT_LOGEVENT
#define ANKI_COZMO_ROBOT_LOGEVENT

#define ENABLE_EVENT_LOGGING 0

#if ENABLE_EVENT_LOGGING

  #include <stdio.h>

  #ifdef __cplusplus
  extern "C" {
  #endif

  // Enum of all event types
  enum EventType {
    HAL_STEP,
    MAIN_STEP,
    IMU,
    WRITE_SPINE,
    READ_SPINE,
    ROBOT_IO,
    PARSE_FRAME,

    ROBOT_IO_SLEEP,
    ROBOT_IO_READ,
    ROBOT_IO_RECEIVE,

    MAIN_CYCLE_TOO_LONG,
    MAIN_CYCLE_TOO_LATE,

    COUNT
  };

  // Called once at the beginning of the tick
  void EventStep();

  // Call when to start timing the event
  // Supports events that happen multiple times a tick
  void EventStart(enum EventType event);

  // Call when to end timing the event
  // Supports events that happen multiple times a tick
  void EventStop(enum EventType event);

  // Call to set misc data about the event
  void EventSetMisc(enum EventType event, uint32_t misc);

  // Call to add to the misc data about the event
  void EventAddToMisc(enum EventType event, uint32_t misc);

  #ifdef __cplusplus
  } //extern "C"
  #endif

#else

  #define EventStep()
  #define EventStart(event)
  #define EventStop(event)
  #define EventSetMisc(event, misc)
  #define EventAddToMisc(event, misc)

#endif

#endif