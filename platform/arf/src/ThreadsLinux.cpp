#include "arf/Threads.h"

// Linux-specific
#ifdef __linux__
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>

namespace ARF
{
int get_num_processors()
{
    return get_nprocs_conf();
}

int set_thread_priority_max( pthread_t t )
{
    // Retrieve thread information
    int policy;
    struct sched_param param;
    int rv = pthread_getschedparam( t, &policy, &param);
    if (rv != 0) { return -1; }

    rlimit rlimits;
    getrlimit( RLIMIT_NICE, &rlimits );

    // param.sched_priority = sched_get_priority_max( policy );
    param.sched_priority = rlimits.rlim_max;
    rv = pthread_setschedparam( t, policy, &param );
    if( rv != 0 ) { return -1; }
    return 0;
}

}
#endif
