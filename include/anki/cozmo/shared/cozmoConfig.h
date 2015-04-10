#ifndef COZMO_CONFIG_H
#define COZMO_CONFIG_H


#ifdef SIMULATOR
    // Simulated robot
    #include "anki/cozmo/shared/cozmoConfig_sim.h"
#elif defined(COZMO_ROBOT_V31)
    // Physical robot (first robot with treads)
    #include "anki/cozmo/shared/cozmoConfig_v3.1.h"
#elif defined(COZMO_ROBOT_V32)
    // Physical robot (larger robot with treads)
    #include "anki/cozmo/shared/cozmoConfig_v3.2.h"
#else // COZMO_BASESTATION?
    // Engine / App
    // Include the file for whichever robot you want to be controlling with the app
    #include "anki/cozmo/shared/cozmoConfig_v3.1.h"
#endif
    
    #include "anki/cozmo/shared/cozmoConfig_common.h"


#endif // COZMO_CONFIG_H
