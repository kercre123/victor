#define EV_STANDALONE 1
#define EV_USE_MONOTONIC 1
#define EV_USE_SELECT 1
#define EV_USE_POLL 0
#define EV_USE_EPOLL 0
#define EV_MULTIPLICITY 1
#define EV_IDLE_ENABLE 1
#define EV_CHILD_ENABLE 0

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wcomment"
#pragma clang diagnostic ignored "-Wstrict-aliasing"
#pragma clang diagnostic ignored "-Wunused-value"
#pragma clang diagnostic ignored "-Wparentheses"
#pragma clang diagnostic ignored "-Wc++1z-compat"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wextern-initializer"
#pragma clang diagnostic ignored "-Wunused-function"

#include "libev/ev.c"

#pragma clang diagnostic pop