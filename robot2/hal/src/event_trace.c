#include <stdint.h>
#include <stdio.h>

#include <stdbool.h>

#include "anki/cozmo/robot/event_trace.h"

#ifdef EVENT_LOGGING_ENABLED

extern uint64_t steady_clock_now(void);

void EventTrace(EventType type)
{
   EventLOG[EventCount].time = steady_clock_now();
   EventLOG[EventCount].type = type;
   EventCount = (EventCount+1)&(MAX_EVENTS-1);
}

static void DumpEvents()
{
   int i;
   FILE* fp = fopen("event.log","w+");
   if (fp) {
      for (i= EventCount; i< MAX_EVENTS;i++)
      {
         fprintf(fp, "%llu, %d\n", EventLOG[i].time, EventLOG[i].type);
      }
      for (i=0;i<EventCount;i++)
      {
         fprintf(fp, "%llu, %d\n", EventLOG[i].time, EventLOG[i].type);
      }
      fclose(fp);
   }
}
      
void EventTraceManage(void)
{
   static bool b = false;
   static uint64_t starttime = 0;
   uint64_t now = steady_clock_now(); 
   if (!starttime) { starttime = now; }
   if (!b && (now)>= starttime+(EVENT_LOG_DURATION_SEC*1000)) { b = true; DumpEvents(); }
}

#endif
