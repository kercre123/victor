//
//  log.h
//  ios-server
//
//  Created by Paul Aluri on 1/31/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

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
