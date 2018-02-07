//
//  AnkiSwitchboard.hpp
//  ios-server
//
//  Created by Paul Aluri on 2/5/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef AnkiSwitchboard_hpp
#define AnkiSwitchboard_hpp

#include <stdio.h>
#include <memory>
#include "libev.h"

#define SB_WIFI_PORT 3291

namespace Anki {
  class Switchboard {
  public:
    static void Start();
    
  private:
    static struct ev_loop* sLoop;
    
    static void HandleStartPairing();
    
    static void StartBleComms();
    static void StartWifiComms();
  };
}

#endif /* AnkiSwitchboard_h */
