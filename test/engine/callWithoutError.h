/**
 * File: callWithoutError.cpp
 *
 * Author: ross
 * Created: 2018
 *
 * Description: runs a lambda without allowing errG to break on error, and returns whether errG was set
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#ifndef __Test_Engine_CallWithoutError_H__
#define __Test_Engine_CallWithoutError_H__
#pragma once

#include <functional>

bool  __attribute__((warn_unused_result))
  CallWithoutError( const std::function<void()>& func );

#endif

