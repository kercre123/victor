//
//  assertHelpers.h
//  BaseStation
//
//  Created by Bryan Austin on 2/12/15.
//  Copyright (c) 2015 Anki. All rights reserved.
//

#ifndef __Util_Helpers_AssertHelpers_H__
#define __Util_Helpers_AssertHelpers_H__

#include <assert.h>

#define ASSERT_AND_RETURN_IF_FAIL(cond) \
  assert(cond); \
  if (!(cond)) return;

#define ASSERT_AND_RETURN_VALUE_IF_FAIL(cond, val) \
  assert(cond); \
  if (!(cond)) return (val);

#endif // __Util_Helpers_AssertHelpers_H__
