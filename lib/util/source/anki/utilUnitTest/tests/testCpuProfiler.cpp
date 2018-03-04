/**
 * File: testCpuProfiler
 *
 * Author: Mark Wesley
 * Created: 06/23/16
 *
 * Description: Unit tests for CpuProfiler
 *
 * Copyright: Anki, Inc. 2016
 *
 * --gtest_filter=CpuProfiler*
 **/


#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/helpers/includeGTest.h"

#include <thread>

#if ANKI_PROFILING_ENABLED

namespace Anki {
namespace Util {
  CONSOLE_VAR_EXTERN(bool, kProfilerLogSlowTicks);
}
}


namespace
{


const bool kLogProfiles = false;
  
  
void SubFuncQuick()
{
  ANKI_CPU_PROFILE("SubFuncQuick");
}

  
void SubFuncSleepShort()
{
  ANKI_CPU_PROFILE("SubFuncSleep50");
  usleep(50);
}


void SubFuncSleepLong()
{
  ANKI_CPU_PROFILE("SubFuncSleep100");
  usleep(100);
}

  
void SubFuncB(int num)
{
  ANKI_CPU_PROFILE("SubFuncB");
  for (int i=0; i<num;++i)
  {
    if (((num+i) % 2) == 0)
    {
      SubFuncSleepShort();
    }
    else
    {
      SubFuncSleepLong();
    }
  }
}
  

void SubFuncA()
{
  ANKI_CPU_PROFILE("SubFuncA");
  for (int i=0; i<3;++i)
  {
    SubFuncB(i);
  }
}


void FakeMainConfig(double maxTickTime = FLT_MAX, uint32_t logFrequency = Anki::Util::CpuThreadProfiler::kLogFrequencyNever)
{
  ANKI_CPU_TICK("MainConfig", maxTickTime, logFrequency);
  ANKI_CPU_PROFILE("MainConfig");
  SubFuncA();
}

  
void FakeMainA()
{
  ANKI_CPU_TICK("MainThreadA", FLT_MAX, Anki::Util::CpuThreadProfiler::kLogFrequencyNever);
  ANKI_CPU_PROFILE("FakeMainA");
  SubFuncA();
}
  
  
void FakeMainB()
{
  ANKI_CPU_TICK("MainThreadB", FLT_MAX, Anki::Util::CpuThreadProfiler::kLogFrequencyNever);
  ANKI_CPU_PROFILE("FakeMainB");
  SubFuncA();
}


void FakeMainC()
{
  ANKI_CPU_TICK("MainThreadC", FLT_MAX, Anki::Util::CpuThreadProfiler::kLogFrequencyNever);
  ANKI_CPU_PROFILE("FakeMainC");
  SubFuncA();
  {
    ANKI_CPU_PROFILE("LotsOfQuick");
    for (int i=0; i <100; ++i)
    {
      SubFuncQuick();
    }
  }
}


void FakeMainD()
{
  ANKI_CPU_TICK("MainThreadD", FLT_MAX, Anki::Util::CpuThreadProfiler::kLogFrequencyNever);
  ANKI_CPU_PROFILE("FakeMainD");
  SubFuncA();
}

  
} // end anonymous namespace


const char* SafeGetSampleName(const Anki::Util::CpuThreadProfile& profile, size_t index)
{
  if (index < profile.GetSampleCount())
  {
    return profile.GetSample(index).GetName();
  }
  else
  {
    return "";
  }
}


TEST(CpuProfiler, OneTickProfile)
{
  Anki::Util::CpuProfiler& cpuProfiler = Anki::Util::CpuProfiler::GetInstance();
  cpuProfiler.Reset();
  // Don't throw any samples out based on time (otherwise results depend on machine speed)
  Anki::Util::CpuThreadProfiler::SetMinSampleDuration_ms(-0.01);
  
  FakeMainA();
  
  Anki::Util::CpuThreadProfiler* threadProfMain = cpuProfiler.GetThreadProfiler( Anki::Util::GetCurrentThreadId() );
  
  ASSERT_NE(threadProfMain, nullptr);
  
  EXPECT_EQ( cpuProfiler.GetThreadProfilerByName("maiNtHReadA"), threadProfMain ); // Case insensitive lookup
  
  const Anki::Util::CpuThreadProfile& profile = threadProfMain->GetCurrentProfile();
  
  EXPECT_EQ(profile.GetTickNum(), 1);
  EXPECT_EQ(profile.GetSampleCount(), 8);
  
  threadProfMain->SortProfile();
  EXPECT_EQ(profile.GetSampleCount(), 8);
  // samples are sorted by start time, earliest first
  EXPECT_EQ( strcmp(SafeGetSampleName(profile, 0), "FakeMainA"), 0 );
  EXPECT_EQ( strcmp(SafeGetSampleName(profile, 1), "SubFuncA"), 0 );
  EXPECT_EQ( strcmp(SafeGetSampleName(profile, 2), "SubFuncB"), 0 );
  EXPECT_EQ( strcmp(SafeGetSampleName(profile, 3), "SubFuncB"), 0 );
  EXPECT_EQ( strcmp(SafeGetSampleName(profile, 4), "SubFuncSleep100"), 0 );
  EXPECT_EQ( strcmp(SafeGetSampleName(profile, 5), "SubFuncB"), 0 );
  EXPECT_EQ( strcmp(SafeGetSampleName(profile, 6), "SubFuncSleep50"), 0 );
  EXPECT_EQ( strcmp(SafeGetSampleName(profile, 7), "SubFuncSleep100"), 0 );
}


TEST(CpuProfiler, OneSlowTickProfile)
{
  Anki::Util::kProfilerLogSlowTicks = true;
  Anki::Util::CpuProfiler& cpuProfiler = Anki::Util::CpuProfiler::GetInstance();
  cpuProfiler.Reset();
  
  FakeMainConfig(0.0f); // It's guarenteed to take longer than 0ms, so this should trigger a slow log
  
  Anki::Util::CpuThreadProfiler* threadProfMain = cpuProfiler.GetThreadProfiler( Anki::Util::GetCurrentThreadId() );
  
  ASSERT_NE(threadProfMain, nullptr);
}


TEST(CpuProfiler, MultiTicksFreqProfile)
{
  Anki::Util::CpuProfiler& cpuProfiler = Anki::Util::CpuProfiler::GetInstance();
  cpuProfiler.Reset();
  // Don't throw any samples out based on time (otherwise results depend on machine speed)
  Anki::Util::CpuThreadProfiler::SetMinSampleDuration_ms(-0.01);
  
  for (int i=0; i < 4; ++i)
  {
    FakeMainConfig(FLT_MAX, 3);
    
    Anki::Util::CpuThreadProfiler* threadProfMain = cpuProfiler.GetThreadProfiler( Anki::Util::GetCurrentThreadId() );
    ASSERT_NE(threadProfMain, nullptr);
    const Anki::Util::CpuThreadProfile& profile = threadProfMain->GetCurrentProfile();
    EXPECT_EQ(profile.GetTickNum(), i+1);
    EXPECT_EQ(profile.GetSampleCount(), 8);
  }
}


TEST(CpuProfiler, MultiThreaded)
{
  Anki::Util::CpuProfiler& cpuProfiler = Anki::Util::CpuProfiler::GetInstance();
  cpuProfiler.Reset();
  // Don't throw any samples out based on time (otherwise results depend on machine speed)
  Anki::Util::CpuThreadProfiler::SetMinSampleDuration_ms(-0.01);
  
  std::thread threadA = std::thread(FakeMainA);
  std::thread threadB = std::thread(FakeMainB);
  std::thread threadC = std::thread(FakeMainC);
  std::thread threadD = std::thread(FakeMainD);
  
  // Have to ask for thread ids immediately (before the thread ends and clears them)
  std::thread::id threadA_id = threadA.get_id();
  std::thread::id threadB_id = threadB.get_id();
  std::thread::id threadC_id = threadC.get_id();
  std::thread::id threadD_id = threadD.get_id();
  
  // Wait for threads to complete
  threadA.join();
  threadB.join();
  threadC.join();
  threadD.join();
  
  Anki::Util::CpuThreadProfiler* threadProfA = cpuProfiler.GetThreadProfiler(threadA_id);
  Anki::Util::CpuThreadProfiler* threadProfB = cpuProfiler.GetThreadProfiler(threadB_id);
  Anki::Util::CpuThreadProfiler* threadProfC = cpuProfiler.GetThreadProfiler(threadC_id);
  Anki::Util::CpuThreadProfiler* threadProfD = cpuProfiler.GetThreadProfiler(threadD_id);
  
  ASSERT_NE(threadProfA, nullptr);
  ASSERT_NE(threadProfB, nullptr);
  ASSERT_NE(threadProfC, nullptr);
  ASSERT_NE(threadProfD, nullptr);
  
  {
    threadProfA->SortProfile();
    const Anki::Util::CpuThreadProfile& profile = threadProfA->GetCurrentProfile();
    
    EXPECT_EQ(profile.GetSampleCount(), 8);
    // samples are sorted by start time, earliest first
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 0), "FakeMainA"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 1), "SubFuncA"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 2), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 3), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 4), "SubFuncSleep100"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 5), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 6), "SubFuncSleep50"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 7), "SubFuncSleep100"), 0 );
  }
  
  {
    threadProfB->SortProfile();
    const Anki::Util::CpuThreadProfile& profile = threadProfB->GetCurrentProfile();
    
    EXPECT_EQ(profile.GetSampleCount(), 8);
    // samples are sorted by start time, earliest first
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 0), "FakeMainB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 1), "SubFuncA"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 2), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 3), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 4), "SubFuncSleep100"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 5), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 6), "SubFuncSleep50"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 7), "SubFuncSleep100"), 0 );
  }
  
  {
    threadProfC->SortProfile();
    const Anki::Util::CpuThreadProfile& profile = threadProfC->GetCurrentProfile();
    
    EXPECT_EQ(profile.GetSampleCount(), 109);
    // samples are sorted by start time, earliest first
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 0), "FakeMainC"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 1), "SubFuncA"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 2), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 3), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 4), "SubFuncSleep100"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 5), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 6), "SubFuncSleep50"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 7), "SubFuncSleep100"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 8), "LotsOfQuick"), 0 );
    for (int i=0; i < 100; ++i)
    {
      EXPECT_EQ( strcmp(SafeGetSampleName(profile, 9+i), "SubFuncQuick"), 0 );
    }
  }
  
  {
    threadProfD->SortProfile();
    const Anki::Util::CpuThreadProfile& profile = threadProfD->GetCurrentProfile();
    
    EXPECT_EQ(profile.GetSampleCount(), 8);
    // samples are sorted by start time, earliest first
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 0), "FakeMainD"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 1), "SubFuncA"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 2), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 3), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 4), "SubFuncSleep100"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 5), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 6), "SubFuncSleep50"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 7), "SubFuncSleep100"), 0 );
  }
  
  if (kLogProfiles)
  {
    threadProfA->SortAndLogProfile();
    threadProfB->SortAndLogProfile();
    threadProfC->SortAndLogProfile();
    threadProfD->SortAndLogProfile();
  }
}


TEST(CpuProfiler, MultiThreadedWithMain)
{
  Anki::Util::CpuProfiler& cpuProfiler = Anki::Util::CpuProfiler::GetInstance();
  cpuProfiler.Reset();
  // Don't throw any samples out based on time (otherwise results depend on machine speed)
  Anki::Util::CpuThreadProfiler::SetMinSampleDuration_ms(-0.01);
  
  std::thread threadB = std::thread(FakeMainB);
  std::thread threadC = std::thread(FakeMainC);
  std::thread threadD = std::thread(FakeMainD);
  
  // Have to ask for thread ids immediately (before the thread ends and clears them)
  std::thread::id threadB_id = threadB.get_id();
  std::thread::id threadC_id = threadC.get_id();
  std::thread::id threadD_id = threadD.get_id();
  
  // Wait for threads to complete
  FakeMainA();
  threadB.join();
  threadC.join();
  threadD.join();
  
  Anki::Util::CpuThreadProfiler* threadProfMain = cpuProfiler.GetThreadProfiler(Anki::Util::GetCurrentThreadId());
  Anki::Util::CpuThreadProfiler* threadProfB = cpuProfiler.GetThreadProfiler(threadB_id);
  Anki::Util::CpuThreadProfiler* threadProfC = cpuProfiler.GetThreadProfiler(threadC_id);
  Anki::Util::CpuThreadProfiler* threadProfD = cpuProfiler.GetThreadProfiler(threadD_id);
  
  ASSERT_NE(threadProfMain, nullptr);
  ASSERT_NE(threadProfB, nullptr);
  ASSERT_NE(threadProfC, nullptr);
  ASSERT_NE(threadProfD, nullptr);
  
  EXPECT_EQ( strcmp(threadProfMain->GetThreadName(), "MainThreadA"), 0 );
  EXPECT_EQ( strcmp(threadProfB->GetThreadName(), "MainThreadB"), 0 );
  EXPECT_EQ( strcmp(threadProfC->GetThreadName(), "MainThreadC"), 0 );
  EXPECT_EQ( strcmp(threadProfD->GetThreadName(), "MainThreadD"), 0 );
  
  {
    threadProfMain->SortProfile();
    const Anki::Util::CpuThreadProfile& profile = threadProfMain->GetCurrentProfile();
    
    EXPECT_EQ(profile.GetSampleCount(), 8);
    // samples are sorted by start time, earliest first
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 0), "FakeMainA"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 1), "SubFuncA"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 2), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 3), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 4), "SubFuncSleep100"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 5), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 6), "SubFuncSleep50"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 7), "SubFuncSleep100"), 0 );
  }
  
  {
    threadProfB->SortProfile();
    const Anki::Util::CpuThreadProfile& profile = threadProfB->GetCurrentProfile();
    
    EXPECT_EQ(profile.GetSampleCount(), 8);
    // samples are sorted by start time, earliest first
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 0), "FakeMainB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 1), "SubFuncA"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 2), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 3), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 4), "SubFuncSleep100"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 5), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 6), "SubFuncSleep50"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 7), "SubFuncSleep100"), 0 );
  }
  
  {
    threadProfC->SortProfile();
    const Anki::Util::CpuThreadProfile& profile = threadProfC->GetCurrentProfile();
    
    EXPECT_EQ(profile.GetSampleCount(), 109);
    // samples are sorted by start time, earliest first
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 0), "FakeMainC"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 1), "SubFuncA"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 2), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 3), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 4), "SubFuncSleep100"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 5), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 6), "SubFuncSleep50"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 7), "SubFuncSleep100"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 8), "LotsOfQuick"), 0 );
    for (int i=0; i < 100; ++i)
    {
      EXPECT_EQ( strcmp(SafeGetSampleName(profile, 9+i), "SubFuncQuick"), 0 );
    }
  }
  
  {
    threadProfD->SortProfile();
    const Anki::Util::CpuThreadProfile& profile = threadProfD->GetCurrentProfile();
    
    EXPECT_EQ(profile.GetSampleCount(), 8);
    // samples are sorted by start time, earliest first
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 0), "FakeMainD"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 1), "SubFuncA"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 2), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 3), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 4), "SubFuncSleep100"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 5), "SubFuncB"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 6), "SubFuncSleep50"), 0 );
    EXPECT_EQ( strcmp(SafeGetSampleName(profile, 7), "SubFuncSleep100"), 0 );
  }
  
  if (kLogProfiles)
  {
    threadProfMain->SortAndLogProfile();
    threadProfB->SortAndLogProfile();
    threadProfC->SortAndLogProfile();
    threadProfD->SortAndLogProfile();
  }
}

#endif // ANKI_PROFILING_ENABLED
