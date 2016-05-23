//
//  export.h
//  BaseStation
//
//  Created by Mark Pauley on 10/9/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#ifndef Util_Export_h_
#define Util_Export_h_

#define ANKI_VISIBLE __attribute__((visibility("default")))

#define ANKI_EXPORT	ANKI_VISIBLE extern
#define ANKI_INTERN __attribute__((visibility("hidden")))  extern

#define ANKI_TRUE 1
#define ANKI_FALSE 0
#define PRINT_EXTERNAL_CALLS 0

#ifdef __cplusplus
#define ANKI_C_BEGIN extern "C" {
#define ANKI_C_END }
#define ANKI_C_EXTERN extern "C"
#else
#define ANKI_C_BEGIN // C
#define ANKI_C_END   // C
#define ANKI_C_EXTERN extern
#endif


#endif
