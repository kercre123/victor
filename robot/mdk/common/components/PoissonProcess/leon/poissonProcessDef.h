///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @addtogroup poissonProcess 
/// @{
/// @brief     Poisson process simulator. (Definitions)
///

#ifndef POISSON_PROCESS_DEF_H
#define POISSON_PROCESS_DEF_H

#include "mv_types.h"

/// @brief A structure representing a poisson process
typedef struct {
    u32 averageMicroSecondsPerEvent; ///< Average time between events
    u64 seed; ///< @brief Seed for the pseudo-random number generator.
              ///<
              ///< use any value here, but if you have multiple processes in parallel
              ///< then make sure their initial seed values are different.
    void (*callback)(void *private); ///< Callback to call on each event.
    void *private; ///* This pointer will be passed to callback() when the event fires.
} tyPoissonProcess;

/// @}
#endif
