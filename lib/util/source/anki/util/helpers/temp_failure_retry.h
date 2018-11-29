/*
 * File: temp_failure_retry.h
 *
 * Author: Stuart Eichert <seichert@anki.com>
 * Created: 11/26/2018
 *
 * Copyright: Anki, Inc. 2018
 *
 */

#ifndef UTIL_HELPERS_TEMP_FAILURE_RETRY_H
#define UTIL_HELPERS_TEMP_FAILURE_RETRY_H

#include <errno.h>

// Mac OS doesn't have TEMP_FAILURE_RETRY
// https://android.googlesource.com/platform/system/core/+/master/base/include/android-base/macros.h
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(exp)           \
  ({                                      \
    decltype (exp) _rc;                   \
    do {                                  \
      _rc = (exp);                        \
    } while(_rc == -1 && errno == EINTR); \
    _rc;                                  \
  })
#endif


#endif /* UTIL_HELPERS_TEMP_FAILURE_RETRY_H */
