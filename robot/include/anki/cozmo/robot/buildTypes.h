/** @file buildTypes.h
 * @brief Definitions of build types and feature flags for Cozmo firmware
 * @author Daniel Casner <daniel@anki.com>
 */


#ifndef _BUILD_TYPES_H_
#define _BUILD_TYPES_H_

#if defined(NDEBUG)
#define ANKI_DEBUG_LEVEL  ANKI_DEBUG_ERRORS_AND_WARNS
#define ANKI_DEBUG_INFO   0
#elif defined(SIMULATOR)
#define ANKI_DEBUG_LEVEL  ANKI_DEBUG_ALL
#define ANKI_DEBUG_INFO   1
#else // Default is development build
#define ANKI_DEBUG_LEVEL  ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS
#define ANKI_DEBUG_INFO   1
#endif

// Which errors will be checked and reported?
#define ANKI_DEBUG_MINIMAL 0 // Only check and output issues with explicit unit tests
#define ANKI_DEBUG_ERRORS 10 // Check and output AnkiErrors and explicit unit tests
#define ANKI_DEBUG_ERRORS_AND_WARNS 20 // Check and output AnkiErrors, AnkiWarns, and explicit unit tests
#define ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS 30 // Check and output AnkiErrors, AnkiWarns, AnkiAsserts, and explicit unit tests
#define ANKI_DEBUG_ALL 40 // Check and output AnkiErrors, AnkiWarns, and explicit unit tests, plus run any additional extensive tests

#endif
