#ifndef TRACE_H
# define TRACE_H

# include "hook.h"
# include "macros.h"
# include "profile.h"

/** Core tracing implementation.  Implements utilities needed
    to implement actual trace modules. */
class HIDDEN IgTrace
{
public:
  static int                  panic(const char *file, int line,
				     const char *func, const char *expr);
  static void                 debug(const char *format, ...);
  static const char *         options(void);
  static const char *         program(void);
  static bool                 filter(const char *info, void *stack[], int depth);

  static bool                 initialize(void);
  static bool                 enabled(void);
  static bool                 enable(void);
  static bool                 disable(void);
};

#endif // TRACE_H
