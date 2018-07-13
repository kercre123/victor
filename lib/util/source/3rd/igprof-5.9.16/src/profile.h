#ifndef PROFILE_H
# define PROFILE_H

# include "macros.h"
# include "atomic.h"
# include "hook.h"
# include <pthread.h>

class IgProfTrace;
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wattributes"
#endif
typedef void IgProfAbortFunc (void) __attribute__((noreturn));
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
# pragma GCC diagnostic pop
#endif

extern bool             s_igprof_activated;
extern IgProfAtomic     s_igprof_enabled;
extern pthread_key_t    s_igprof_bufkey;
extern pthread_key_t    s_igprof_flagkey;
extern int              s_igprof_stderrOpen;
extern void             (*igprof_abort) (void) __attribute__((noreturn));
extern char *           (*igprof_getenv) (const char *);
extern int              (*igprof_unsetenv) (const char *);

HIDDEN const char *igprof_options(void);
HIDDEN void igprof_reset_profiles(void);
HIDDEN void igprof_debug(const char *format, ...);
HIDDEN int igprof_panic(const char *file, int line, const char *func, const char *expr);
HIDDEN bool igprof_init(const char *id, void (*threadinit)(void),
	                bool perthread, double clockres = 0.);

/** Return a profile buffer for a profiler in the current thread.  It
    is safe to call this function from any thread and in asynchronous
    signal handlers at any time.  Returns the buffer to use or a null
    to indicate no data should be gathered in the calling context, for
    example if the profile core itself has already been destroyed.  */
HIDDEN inline IgProfTrace *
igprof_buffer(void)
{
  return LIKELY(s_igprof_activated)
    ? (IgProfTrace *) pthread_getspecific(s_igprof_bufkey) : 0;
}

/** Enable the profiling system globally. Safe to call from anywhere. */
inline void
igprof_enable_globally(void)
{
  IgProfAtomicInc(&s_igprof_enabled);
}

/** Disable the profiling system globally. Safe to call from anywhere. */
inline void
igprof_disable_globally(void)
{
  IgProfAtomicDec(&s_igprof_enabled);
}

/** Enable the profiler in this thread. Safe to call from anywhere.
    Returns @c true if the profiler is enabled after the call. */
HIDDEN inline bool
igprof_enable(void)
{
  IgProfAtomic *flag = (IgProfAtomic *) pthread_getspecific(s_igprof_flagkey);
  return LIKELY(flag) && IgProfAtomicInc(flag) > 0 && s_igprof_enabled > 0;
}

/** Disable the profiler in this thread. Safe to call from anywhere.
    Returns @c true if the profiler was enabled before the call.  */
HIDDEN inline bool
igprof_disable(void)
{
  IgProfAtomic *flag = (IgProfAtomic *) pthread_getspecific(s_igprof_flagkey);
  return LIKELY(flag) && IgProfAtomicDec(flag) >= 0 && s_igprof_enabled > 0;
}

#endif // PROFILE_H
