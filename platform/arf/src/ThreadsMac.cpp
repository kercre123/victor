#include "arf/Threads.h"

// Mac-specific implementations for Threads

// Mac-specific
// Borrowed from https://yyshen.github.io/2015/01/18/binding_threads_to_cores_osx.html
#ifdef __APPLE__

#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/thread_policy.h>
#include <mach/thread_act.h>
#define SYSCTL_CORE_COUNT   "machdep.cpu.core_count"

namespace ARF
{
int get_num_processors()
{
    int32_t core_count = 0;
    size_t  len = sizeof(core_count);
    int ret = sysctlbyname(SYSCTL_CORE_COUNT, &core_count, &len, 0, 0);
    if (ret) {
        printf("error while get core count %d\n", ret);
        return -1;
    }
    return core_count;
}

int set_thread_priority_max( pthread_t /*t*/ )
{
    // Does nothing for now
    return 0;
}

}

void CPU_ZERO(cpu_set_t *cs) { cs->count = 0; }

void CPU_SET(int num, cpu_set_t *cs) { cs->count |= (1 << num); }

int CPU_ISSET(int num, cpu_set_t *cs) { return (cs->count & (1 << num)); }

int pthread_setaffinity_np(pthread_t thread, size_t cpu_size,
                           cpu_set_t *cpu_set)
{
  thread_port_t mach_thread;
  int core = 0;

  for (core = 0; core < static_cast<int>(8 * cpu_size); core++) {
    if (CPU_ISSET(core, cpu_set)) break;
  }
  thread_affinity_policy_data_t policy = { core };
  mach_thread = pthread_mach_thread_np(thread);
  thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY,
                    (thread_policy_t)&policy, 1);
  return 0;
}

#endif
