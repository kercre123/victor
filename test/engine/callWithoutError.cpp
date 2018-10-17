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

#include "test/engine/callWithoutError.h"
#include "util/logging/logging.h"
#include <functional>

bool CallWithoutError( const std::function<void()>& func )
{
  // locks access to other threads and sets errG==false
  Anki::Util::sPushErrG( false );

  // prevent break on error in case an error is expected
  const bool prevBreak = Anki::Util::_errBreakOnError;
  Anki::Util::_errBreakOnError = false;
  
  func();
  
  const bool errGGotSet = Anki::Util::sGetErrG();
  Anki::Util::_errBreakOnError = prevBreak;
  
  // restores previous errG value and unlocks access by other threads
  Anki::Util::sPopErrG();
  
  return errGGotSet;
}


