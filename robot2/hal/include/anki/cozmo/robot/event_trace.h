#ifndef EVENTTRACE_H
#define EVENTTRACE_H

#ifdef __cplusplus
extern "C" {
#endif

/***********
 Log a fixed number of precision event timestamps
************/

#define EVENT_LOG_DURATION_SEC 10   //Dump log after this many seconds
#define MAX_EVENTS 0x1000  //Record this many recent events (must be power of 2)


typedef  enum {
   event_READSTART,
   event_READEND,
  
   event_SYNC,
  
   event_FRAMESTART,
   event_FRAMEEND,
  
   event_STEPSTART,
   event_STEPEND,
  
   event_IMUSTART,
   event_IMUEND,
  
   event_SENDSTART,
   event_SENDEND,
   /**^^^ Insert New Events Above Here ^^^**/
   EVENT_TYPE_COUNT
} EventType;

static struct SpineEvent_t{
   uint64_t time;
   EventType type;
} EventLOG[MAX_EVENTS];
static uint32_t EventCount=0;

//#define EVENT_LOGGING_ENABLED
#ifdef EVENT_LOGGING_ENABLED

void EventTrace(EventType type);
void EventTraceManage(void);

#else
#define EventTrace(t) 
#define EventTraceManage()

#endif

#ifdef __cplusplus
}
#endif

#endif//EVENTTRACE_H

