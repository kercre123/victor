///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup poissonProcess Poisson process simulator
/// @{
/// @brief     Poisson process simulator.
///
/// This component can be used to simulate poisson processes.
/// for details see:
/// http://en.wikipedia.org/wiki/Poisson_process
///
/// A poisson process is a good model for radioactive decay,
/// arrival of packets in a network, some types of random errors.
///
/// This component uses drvTimer, and swcRandom.

#ifndef POISSON_PROCESS_H
#define POISSON_PROCESS_H

#include "poissonProcessDef.h"

/// @brief Initialize the PoissonProcess component.
///
/// Call it once at the start of the application.
void poissonInit();

/// @brief Start simulating a poisson process
///
/// @param  process  pointer to a tyPoissonProcess structure
void poissonProcess(tyPoissonProcess *process);
///@}
#endif
