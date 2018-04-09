#include "anki/cozmo/robot/logEvent.h"

#if ENABLE_EVENT_LOGGING

#include <time.h>
#include <errno.h>
#include <string.h>

typedef struct {
  struct timespec start[COUNT];
  uint64_t total[COUNT];
  uint8_t count[COUNT];
  uint32_t misc[COUNT];
} EventTimes;

// How many ticks to track events for before writing to file
const uint32_t kNumEvents = 3000;
static EventTimes events[kNumEvents];
static uint32_t index = 0;

void EventStep()
{
  index++;

  if(index >= kNumEvents)
  {
    index = 0;

    printf("WRITING EVENTS\n");

    FILE* file = NULL;
    file = fopen("/data/misc/output.csv", "w");

    // csv header
    fprintf(file, 
            "HAL_STEP,, MAIN_STEP,, IMU,, WRITE_SPINE,, "
            "READ_SPINE,, ROBOT_IO,, PARSE_FRAME,, ROBOT_IO_SLEEP,, "
            "ROBOT_IO_READ,,ROBOT_IO_READ_AMOUNT, "
            "ROBOT_IO_RECIEVE,, MAIN_CYCLE_TOO_LONG,, MAIN_CYCLE_TOO_LATE,\n");
    for(uint32_t i = 0; i < kNumEvents; i++)
    {
     fprintf(file, 
             "%llu, %u, %llu, %u, %llu, %u, %llu, %u, "
             "%llu, %u, %llu, %u, %llu, %u, %llu, %u, "
             "%llu, %u, %u, "
             "%llu, %u, %llu, %u, %llu, %u\n",
             events[i].total[HAL_STEP],
             events[i].count[HAL_STEP],
             events[i].total[MAIN_STEP],
             events[i].count[MAIN_STEP],
             events[i].total[IMU],
             events[i].count[IMU],
             events[i].total[WRITE_SPINE],
             events[i].count[WRITE_SPINE],
             events[i].total[READ_SPINE],
             events[i].count[READ_SPINE],
             events[i].total[ROBOT_IO],
             events[i].count[ROBOT_IO],
             events[i].total[PARSE_FRAME],
             events[i].count[PARSE_FRAME],
             events[i].total[ROBOT_IO_SLEEP],
             events[i].count[ROBOT_IO_SLEEP],
             events[i].total[ROBOT_IO_READ],
             events[i].count[ROBOT_IO_READ],
             events[i].misc[ROBOT_IO_READ],
             events[i].total[ROBOT_IO_RECEIVE],
             events[i].count[ROBOT_IO_RECEIVE],
             events[i].total[MAIN_CYCLE_TOO_LONG],
             events[i].count[MAIN_CYCLE_TOO_LONG],
             events[i].total[MAIN_CYCLE_TOO_LATE],
             events[i].count[MAIN_CYCLE_TOO_LATE]);
    }
    fclose(file);
    file = NULL;
    memset(events, 0, sizeof(events));
  }
}

void EventStart(enum EventType event)
{
  // Track event start time
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  events[index].start[event] = t;
}

void EventStop(enum EventType event)
{
  // Get event end time
  struct timespec t; 
  clock_gettime(CLOCK_MONOTONIC, &t);

  // Subtract end minus start time and deal with negative nanoseconds
  int64_t nano = (t.tv_nsec - events[index].start[event].tv_nsec);
  int64_t sec = (t.tv_sec - events[index].start[event].tv_sec);
  if(nano < 0)
  {
    sec -= 1;
    nano += 1000000000L;
  }

  // Add duration of event to total and increment count
  events[index].total[event] += (sec + nano);
  events[index].count[event]++;
}

void EventSetMisc(enum EventType event, uint32_t misc)
{
  events[index].misc[event] = misc;
}

void EventAddToMisc(enum EventType event, uint32_t misc)
{
  events[index].misc[event] += misc;
}

#endif //#if ENABLE_EVENT_LOGGING