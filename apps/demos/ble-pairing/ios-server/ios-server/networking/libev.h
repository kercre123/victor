//
//  libev.h
//  ios-server
//
//  Created by Paul Aluri on 1/31/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef libev_h
#define libev_h

#define EV_STANDALONE 1
#define EV_USE_MONOTONIC 1
#define EV_USE_SELECT 1
#define EV_USE_POLL 0
#define EV_USE_EPOLL 0
#define EV_MULTIPLICITY 1
#define EV_IDLE_ENABLE 1

#include "../libev/ev++.h"

#endif /* libev_h */
