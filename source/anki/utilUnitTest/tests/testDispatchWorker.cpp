/**
 * File: testDispatchWorker
 *
 * Author: Lee Crippen
 * Created: 8/31/16
 *
 * Description: Unit tests for DispatchWorker
 *
 * Copyright: Anki, Inc. 2016
 *
 * --gtest_filter=DispatchWorker*
 **/


#include "util/helpers/includeGTest.h"
#include "util/dispatchWorker/dispatchWorker.h"

#include <atomic>
#include <memory>
#include <string>

TEST(DispatchWorker, ProcessNoArgs)
{
  using namespace Anki::Util;
  
  // Zero extra worker threads, no jobs
  {
    std::atomic_int myInt(0);
    using MyDispatchWorker = DispatchWorker<0>;
    MyDispatchWorker::FunctionType workerFunction = [&myInt] () { std::atomic_fetch_add(&myInt, 1); };
    MyDispatchWorker myWorker(workerFunction);
    myWorker.Process();
    EXPECT_EQ(myInt.load(), 0);
  }
  
  // Zero extra worker threads, only one job
  {
    std::atomic_int myInt(0);
    using MyDispatchWorker = DispatchWorker<0>;
    MyDispatchWorker::FunctionType workerFunction = [&myInt] () { std::atomic_fetch_add(&myInt, 1); };
    MyDispatchWorker myWorker(workerFunction);
    myWorker.PushJob();
    myWorker.Process();
    EXPECT_EQ(myInt.load(), 1);
  }
  
  // Zero extra worker threads, many jobs
  {
    std::atomic_int myInt(0);
    using MyDispatchWorker = DispatchWorker<0>;
    MyDispatchWorker::FunctionType workerFunction = [&myInt] () { std::atomic_fetch_add(&myInt, 1); };
    MyDispatchWorker myWorker(workerFunction);
    for (int i=0; i<100; ++i) { myWorker.PushJob(); }
    myWorker.Process();
    EXPECT_EQ(myInt.load(), 100);
  }
  
  // Many extra worker threads, no jobs
  {
    std::atomic_int myInt(0);
    using MyDispatchWorker = DispatchWorker<100>;
    MyDispatchWorker::FunctionType workerFunction = [&myInt] () { std::atomic_fetch_add(&myInt, 1); };
    MyDispatchWorker myWorker(workerFunction);
    myWorker.Process();
    EXPECT_EQ(myInt.load(), 0);
  }
  
  // Many extra worker threads, not enough jobs
  {
    std::atomic_int myInt(0);
    using MyDispatchWorker = DispatchWorker<100>;
    MyDispatchWorker::FunctionType workerFunction = [&myInt] () { std::atomic_fetch_add(&myInt, 1); };
    MyDispatchWorker myWorker(workerFunction);
    myWorker.PushJob();
    myWorker.Process();
    EXPECT_EQ(myInt.load(), 1);
  }
  
  // Many extra worker threads, many jobs
  {
    std::atomic_int myInt(0);
    using MyDispatchWorker = DispatchWorker<100>;
    MyDispatchWorker::FunctionType workerFunction = [&myInt] () { std::atomic_fetch_add(&myInt, 1); };
    MyDispatchWorker myWorker(workerFunction);
    for (int i=0; i<1000; ++i) { myWorker.PushJob(); }
    myWorker.Process();
    EXPECT_EQ(myInt.load(), 1000);
    
    // Do it again!
    for (int i=0; i<1000; ++i) { myWorker.PushJob(); }
    myWorker.Process();
    EXPECT_EQ(myInt.load(), 2000);
  }
}


TEST(DispatchWorker, ProcessWithArgs)
{
  using namespace Anki::Util;
  
  // Many extra worker threads, many jobs with args
  {
    std::atomic_int myInt(0);
    using MyDispatchWorker = DispatchWorker<100, int, int, int>;
    MyDispatchWorker::FunctionType workerFunction = [&myInt] (int int1, int int2, int int3)
    {
      std::atomic_fetch_add(&myInt, int1);
      std::atomic_fetch_add(&myInt, int2);
      std::atomic_fetch_add(&myInt, int3);
    };
    
    MyDispatchWorker myWorker(workerFunction);
    for (int i=0; i<1000; ++i) { myWorker.PushJob(1, 2, 3); }
    myWorker.Process();
    EXPECT_EQ(myInt.load(), 6000);
  }
  
  // Some extra worker threads, longer jobs with args
  {
    std::atomic_int myInt(0);
    using MyDispatchWorker = DispatchWorker<10, int, int, int>;
    MyDispatchWorker::FunctionType workerFunction = [&myInt] (int int1, int int2, int int3)
    {
      int total = 0;
      for (int i = int1; i < int2; ++i)
      {
        total += int3;
      }
      std::atomic_fetch_add(&myInt, total);
    };
    
    MyDispatchWorker myWorker(workerFunction);
    for (int i=0; i<1000; ++i) { myWorker.PushJob(0, 10000, 2); }
    myWorker.Process();
    EXPECT_EQ(myInt.load(), 20000000);
  }
  
  // Some extra worker threads, string rvalue arg
  {
    #ifndef LINUX // linux appears to hate tuples containing rvalue members
    std::atomic_int myInt(0);
    using MyDispatchWorker = DispatchWorker<10, std::string&&>;
    MyDispatchWorker::FunctionType workerFunction = [&myInt] (std::string&& myString)
    {
      int total = static_cast<int>(myString.length());
      std::atomic_fetch_add(&myInt, total);
    };
    
    MyDispatchWorker myWorker(workerFunction);
    for (int i=0; i<100; ++i) { myWorker.PushJob(std::string("a")); }
    myWorker.Process();
    EXPECT_EQ(myInt.load(), 100);
    #endif
  }
  
  // Some extra worker threads, const vector reference argument
  {
    std::atomic_int myInt(0);
    using MyDispatchWorker = DispatchWorker<10, std::vector<int>&>;
    MyDispatchWorker::FunctionType workerFunction = [&myInt] (const std::vector<int>& myVector)
    {
      std::atomic_fetch_add(&myInt, myVector[0]);
    };
    
    MyDispatchWorker myWorker(workerFunction);
    std::vector<int> myVector = { 8675309 };
    for (int i=0; i<100; ++i)
    {
      myWorker.PushJob(myVector);
    }
    myWorker.Process();
    EXPECT_EQ(myInt.load(), 867530900);
  }
  
  // Some extra worker threads, normal lvalue vector argument
  {
    std::atomic_int myInt(0);
    using MyDispatchWorker = DispatchWorker<10, std::vector<int>>;
    MyDispatchWorker::FunctionType workerFunction = [&myInt] (std::vector<int> myVector)
    {
      std::atomic_fetch_add(&myInt, myVector[0]);
    };
    
    MyDispatchWorker myWorker(workerFunction);
    std::vector<int> myVector = { 8675309 };
    for (int i=0; i<100; ++i)
    {
      myWorker.PushJob(myVector);
    }
    myWorker.Process();
    EXPECT_EQ(myInt.load(), 867530900);
  }
  
  // Some extra worker threads, unique_ptr argument
  {
    #ifndef LINUX // std::is_copy_constructible<std::tuple<std::unique_ptr<...>>> is a build failure
    std::atomic_int myInt(0);
    using MyDispatchWorker = DispatchWorker<10, std::unique_ptr<std::vector<int>>>;
    MyDispatchWorker::FunctionType workerFunction = [&myInt] (std::unique_ptr<std::vector<int>> myUnique)
    {
      std::atomic_fetch_add(&myInt, (*myUnique)[0]);
    };
    
    MyDispatchWorker myWorker(workerFunction);
    std::unique_ptr<std::vector<int>> myUnique;
    myUnique.reset(new std::vector<int>{8675309});
    myWorker.PushJob(std::move(myUnique));
    myWorker.Process();
    EXPECT_EQ(myInt.load(), 8675309);
    #endif
  }
}
