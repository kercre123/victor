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

#include <thread>

class Log {
  public:
  static void Write(const char* message) {
#ifdef debug_logging
    std::thread::id threadId = std::this_thread::get_id();
    
    printf("[thread %d]\t\t", threadId);
    printf("ankiswitchboardd: %s\n", message);
#endif
  }
};

#endif /* log_h */
