/**
 * File: utilUnitTestShared
 *
 * Author: Mark Wesley
 * Created: 12/16/15
 *
 * Description: Shared settings etc. for all Util Unit tests
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __UtilUnitTest_Shared_UtilUnitTestShared_H__
#define __UtilUnitTest_Shared_UtilUnitTestShared_H__



class UtilUnitTestShared
{
public:
  
  enum class TimingMode
  {
    Default,
    Valgrind
  };
  
  static void SetTimingMode(TimingMode mode) { sTimingMode = mode; }
  static TimingMode GetTimingMode() { return sTimingMode; }
  
private:
  
  static TimingMode sTimingMode;
};


#endif // __UtilUnitTest_Shared_UtilUnitTestShared_H__

