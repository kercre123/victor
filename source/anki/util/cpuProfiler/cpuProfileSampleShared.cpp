/**
 * File: cpuProfileSampleShared
 *
 * Author: Mark Wesley
 * Created: 07/30/16
 *
 * Description: Data shared between all instances of a single profile (across calls and ticks)
 *              Currently just the name of the sample
 *              Can easily be extended to track stats like min/avg/max time, num calls (per-tick or ever)
 *              but as that can also be collected externally we skip it here for now to minimize skewing performance
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#include "util/cpuProfiler/cpuProfileSampleShared.h"


#if ANKI_CPU_PROFILER_ENABLED


namespace Anki {
namespace Util {
  
    
    
} // end namespace Util
} // end namespace Anki


#endif // ANKI_CPU_PROFILER_ENABLED

