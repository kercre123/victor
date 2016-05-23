
//  signalTest.cpp
//  DrivePlugin
//
//  Created by Bryan Austin on 11/6/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//
// --gtest_filter=SignalTest*

#include "util/helpers/includeGTest.h"

#include "util/signals/simpleSignal.hpp"
#include <iostream>

TEST(SignalTest, ScopedHandles)
{
  // smart handles going out of scope should remove them
  {
    std::cout << "test handles going out of scope\n";
    int a = 0;
    using TestSignal = Signal::Signal<void(void)>;

    TestSignal signal;
    Signal::SmartHandle persistentHandle = signal.ScopedSubscribe( [&a] { ++a; } );
    {
      Signal::SmartHandle tempHandle = signal.ScopedSubscribe( [&a] { a += 5; } );
      signal.emit();
      ASSERT_EQ(a, 6);
    }
    signal.emit();
    ASSERT_EQ(a, 7);
  }
}

TEST(SignalTest, SubjectLifetime)
{
  // smart handles should not go bonkers if their subject is deleted before they are
  {
    std::cout << "test signals being destroyed before observers\n";
    using TestSignal = Signal::Signal<int(int&, int)>;
    Signal::SmartHandle* persistentHandle = new Signal::SmartHandle();
    ASSERT_EQ((*persistentHandle) == nullptr, true);
    {
      int a = 0;
      TestSignal signal;
      *persistentHandle = signal.ScopedSubscribe( [] (int& a, int b) { a += b; return a; } );
      signal.emit(a, 2);
      signal.emit(a, 3);
      ASSERT_EQ(a, 5);
    }
    delete persistentHandle;
  }
}

TEST(SignalTest, HandleAssignment)
{
  // assigning away handles should unregister them
  {
    std::cout << "test assigning away handles properly unregistering\n";
    using TestSignal = Signal::Signal<void(void)>;
    TestSignal signal;

    int a = 0;
    Signal::SmartHandle handle = signal.ScopedSubscribe( [&a] { ++a; } );
    signal.emit();
    ASSERT_EQ(a, 1);
    handle = nullptr;
    ASSERT_EQ(handle == nullptr, true);
    signal.emit();
    ASSERT_EQ(a, 1);
    handle = signal.ScopedSubscribe( [&a] { a += 5; } );
    ASSERT_EQ(handle == nullptr, false);
    signal.emit();
    ASSERT_EQ(a, 6);
  }
}

TEST(SignalTest, ClassUsage)
{
  // test typical class usage
  {
    std::cout << "test class usage\n";
    using TestSignal = Signal::Signal<void(void)>;

    class TestObserver {
    public:
      TestObserver() {}
      ~TestObserver() {}
      void ListenToSignal( TestSignal& signal, int& a ) { _handle = signal.ScopedSubscribe( [&a] { ++a; } ); }
    private:
      Signal::SmartHandle _handle;
    };

    TestSignal signal;
    int a = 0;
    TestObserver* observer = new TestObserver();
    observer->ListenToSignal(signal, a);
    signal.emit();
    ASSERT_EQ(a, 1);
    delete observer;
    signal.emit();
    ASSERT_EQ(a, 1);
    observer = new TestObserver();
    signal.emit();
    ASSERT_EQ(a, 1);
    observer->ListenToSignal(signal, a);
    signal.emit();
    ASSERT_EQ(a, 2);
    delete observer;
  }
}
