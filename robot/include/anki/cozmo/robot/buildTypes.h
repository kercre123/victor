/** @file buildTypes.h
 * @brief Definitions of build types and feature flags for Cozmo firmware
 * @author Daniel Casner <daniel@anki.com>
 */


#ifndef _BUILD_TYPES_H_
#define _BUILD_TYPES_H_

#include "build_type.h"

// This is a superset of factory
#if defined(FCC_TEST)
#define FACTORY           1
#endif

#if defined(RELEASE)
#define ANKI_DEBUG_LEVEL  ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS
#define ANKI_DEBUG_INFO   0
#define ANKI_DEBUG_EVENTS 1
#elif defined(FACTORY)
#define ANKI_DEBUG_LEVEL  ANKI_DEBUG_ALL
#define ANKI_DEBUG_INFO   0
#define ANKI_DEBUG_EVENTS 1
#elif defined(SIMULATOR)
#define ANKI_DEBUG_LEVEL  ANKI_DEBUG_ALL
#define ANKI_DEBUG_INFO   1
#define ANKI_DEBUG_EVENTS 1
#elif defined(DEVELOPMENT) // Default is development build
#define ANKI_DEBUG_LEVEL  ANKI_DEBUG_ALL
#define ANKI_DEBUG_INFO   1
#define ANKI_DEBUG_EVENTS 1
#else
#error "Unsupported build type"
#endif

#if defined(FACTORY)
#define ALLOW_FACTORY_NV_WRITE 1
#define FACTORY_FACE_MENUS 1
#define FACTORY_BIRTH_CERTIFICATE_CHECK_ENABLED 1
#define FACTORY_UPGRADE_CONTROLLER 1
#define FACTORY_BOOT_SEQUENCE 1
#define FACTORY_USE_STATIC_VERSION_DATA 1
#else
#define ALLOW_FACTORY_NV_WRITE 0
#define FACTORY_FACE_MENUS 0
#define FACTORY_BIRTH_CERTIFICATE_CHECK_ENABLED 0
#define FACTORY_UPGRADE_CONTROLLER 0
#define FACTORY_BOOT_SEQUENCE 0
#define FACTORY_USE_STATIC_VERSION_DATA 0
#endif

// Which errors will be checked and reported?
#define ANKI_DEBUG_MINIMAL 0 // Only check and output issues with explicit unit tests
#define ANKI_DEBUG_ERRORS 10 // Check and output AnkiErrors and explicit unit tests
#define ANKI_DEBUG_ERRORS_AND_WARNS 20 // Check and output AnkiErrors, AnkiWarns, and explicit unit tests
#define ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS 30 // Check and output AnkiErrors, AnkiWarns, AnkiAsserts, and explicit unit tests
#define ANKI_DEBUG_ALL 40 // Check and output AnkiErrors, AnkiWarns, and explicit unit tests, plus run any additional extensive tests

#endif
