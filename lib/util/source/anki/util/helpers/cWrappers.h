//
//  cWrappers.h
//  BaseStation
//
//  Created by Mark Pauley on 10/9/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#ifndef UTIL_CWRAPPERS_H_
#define UTIL_CWRAPPERS_H_

#define DEFINE_SIMPLE_CONVERSIONS(ty, ref_ty)               \
  inline ty* unwrap(ref_ty ref) {                    \
    return reinterpret_cast<ty*>(ref);                      \
  }                                                         \
\
  inline ref_ty wrap (const ty* ptr) {               \
    return reinterpret_cast<ref_ty>(const_cast<ty*>(ptr));  \
  }

#endif

