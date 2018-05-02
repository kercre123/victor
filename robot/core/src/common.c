#include <stdlib.h>
#include <stdarg.h>

#include "core/common.h"

void error_exit(CoreAppErrorCode code, const char* msg, ...)
{
  va_list args;

  printf("ERROR %d: ", code);
  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);
  printf("\n\n");
  core_common_on_exit();
  exit(code);
}
