#if defined(USE_ANKITRACE)
#include <lttng/tracelog.h>
#include "platform/anki-trace/anki_ust.h"
#else
#define tracepoint(p,f,c)
#define tracelog(e,m,...)
#endif
