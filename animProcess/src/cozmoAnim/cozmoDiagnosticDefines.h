/*
 * File:          cozmoDiagnosticDefines.h
 * Date:          9/25/2018
 * Author:        Stuart Eichert
 *
 * Description:   Decide if we are going to track these diagnostics
 *
 */

#ifndef ANKI_COZMO_DIAGNOSTIC_DEFINES_H
#define ANKI_COZMO_DIAGNOSTIC_DEFINES_H


#include "util/global/globalDefinitions.h"
#if ANKI_PROFILING_ENABLED && !defined(SIMULATOR)
  #define ENABLE_CE_SLEEP_TIME_DIAGNOSTICS 0
  #define ENABLE_CE_RUN_TIME_DIAGNOSTICS 1
#else
  #define ENABLE_CE_SLEEP_TIME_DIAGNOSTICS 0
  #define ENABLE_CE_RUN_TIME_DIAGNOSTICS 0
#endif

#endif // ANKI_COZMO_DIAGNOSTIC_DEFINES_H
