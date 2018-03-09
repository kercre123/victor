/**
 * File: log.h
 *
 * Author: paluri
 * Created: 1/31/2018
 *
 * Description: Abstracted log for ankiswitchboardd
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef log_h
#define log_h

#define debug_logging

#include "anki-ble/log.h"
#include <cstdio>
#include <thread>
#include <stdlib.h>

class Log {
  public:
  template<typename... Args>
  static void Write(Args&&... args) {
#ifdef debug_logging
    //std::thread::id threadId = std::this_thread::get_id();
    //printf("[thread %d]\t\t", threadId);
    char buffer [200];
    snprintf(buffer, 200, "%s", std::forward<Args>(args)...);
    printf("ankiswitchboardd: %s\n", buffer);
#endif
  }
};

#endif /* log_h */
