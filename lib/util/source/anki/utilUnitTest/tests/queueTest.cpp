//  queueTest.cpp
//  DrivePlugin
//
//  Created by Bryan Austin on 11/11/14.
//  Copyright (c) 2014 Anki, Inc. All rights reserved.
//
// --gtest_filter=QueueTest*

#include "util/helpers/includeGTest.h"

#include "util/dispatchQueue/dispatchQueue.h"
#include "util/helpers/templateHelpers.h"
#include <chrono>
#include <iostream>
#include <thread>

using Anki::Util::Dispatch::Queue;

//
// VIC-492: Disabled because test does not pass reliably
//
TEST(QueueTest, DISABLED_ScheduledCallbacks)
{
  std::cout << "testing repeat callbacks in queues\n";
  Queue* queue = Anki::Util::Dispatch::Create();
  volatile int a = 0;
  Anki::Util::TaskHandle handle = Anki::Util::Dispatch::ScheduleCallback(queue, std::chrono::milliseconds(500), [&a] { a++; } );
  std::this_thread::sleep_for(std::chrono::milliseconds(250)); // total 250
  ASSERT_EQ(a, 0);
  std::this_thread::sleep_for(std::chrono::milliseconds(500)); // total 750
  ASSERT_EQ(a, 1);
  std::this_thread::sleep_for(std::chrono::milliseconds(500)); // total 1250
  ASSERT_EQ(a, 2);
  Anki::Util::Dispatch::Sync(queue, [&a] { a++; } );
  std::this_thread::sleep_for(std::chrono::milliseconds(500)); // total 1750
  Anki::Util::Dispatch::Stop(queue);
  Anki::Util::Dispatch::Release(queue);
  ASSERT_EQ(a, 4);
}

//
// VIC-492: Disabled because test does not pass reliably
//
TEST(QueueTest, DISABLED_RemoveCallback)
{
  std::cout << "test removing scheduled callbacks\n";
  Queue* queue = Anki::Util::Dispatch::Create();
  volatile int a = 0;
  Anki::Util::TaskHandle handle = Anki::Util::Dispatch::ScheduleCallback(queue, std::chrono::milliseconds(250), [&a] { a++; } );
  std::this_thread::sleep_for(std::chrono::milliseconds(375)); // total 375
  ASSERT_EQ(a, 1);
  (*handle).Invalidate();
  std::this_thread::sleep_for(std::chrono::milliseconds(250)); // total 625
  ASSERT_EQ(a, 1);
  Anki::Util::Dispatch::Stop(queue);
  Anki::Util::Dispatch::Release(queue);
  ASSERT_EQ(a, 1);
}

TEST(QueueTest, TaskHandleHeartbeat)
{
  std::cout << "test task handle outliving its parent queue\n";
  Queue* queue = Anki::Util::Dispatch::Create();
  Anki::Util::TaskHandle handle = Anki::Util::Dispatch::ScheduleCallback(queue, std::chrono::milliseconds(1000), [] {} );
  Anki::Util::Dispatch::Stop(queue);
  Anki::Util::Dispatch::Release(queue);
  (*handle).Invalidate();
}

//
// VIC-492: Disabled because test does not pass reliably
//
// test that invalidating a handle prevents it from executing again, even if it's already queued to run
// on the main queue (like, it's already been moved from the deferred queue to the main queue because
// its time to execute passed while a task was running that invalidates the handle)
TEST(QueueTest, DISABLED_ReverseTaskHeartbeat)
{
  class TestClass {
  public:
    TestClass(volatile int& counter) : _counter(counter), _pointer(new std::string("stuff")) {}
    ~TestClass() {
      Anki::Util::SafeDelete(_pointer);
      _handle->Invalidate();
    }

    void Update() { std::cout << "QueueTest.ReverseTaskHeartbeat: " << *_pointer; _counter++; }
    void SetTaskHandle(Anki::Util::TaskHandle handle) {
      _handle = std::move(handle);
    }

  private:
    volatile int& _counter;
    std::string* _pointer;
    Anki::Util::TaskHandle _handle;
  };

  volatile int counter = 0;
  TestClass* obj = new TestClass(counter);

  Queue* queue = Anki::Util::Dispatch::Create();
  obj->SetTaskHandle(Anki::Util::Dispatch::ScheduleCallback(queue, std::chrono::milliseconds(100), [obj] {
    obj->Update();
  }));

  std::this_thread::sleep_for(std::chrono::milliseconds(120));
  EXPECT_EQ(1, counter);

  // now add a task that will sleep for long enough that the above task will be added to the main queue
  Anki::Util::Dispatch::Sync(queue, [&obj] {
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    // now delete the object
    Anki::Util::SafeDelete(obj);
  });

  // sleep for long enough that if the repeated task is on the main queue, it will execute and crash
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // the task shouldn't have executed a second time
  EXPECT_EQ(1, counter);

  Anki::Util::Dispatch::Stop(queue);
  Anki::Util::Dispatch::Release(queue);
}

// make sure that handle creates and deletes queue automatically
TEST(QueueTest, TestQueueHandle)
{
  using namespace Anki::Util;
  volatile int a = 0;
  {
    Dispatch::QueueHandle queue{Dispatch::create_queue};
    Anki::Util::TaskHandle handle = Anki::Util::Dispatch::ScheduleCallback(
      queue.get(), std::chrono::milliseconds(500), [&a] { a++; }
    );
    std::this_thread::sleep_for(std::chrono::milliseconds(250)); // total 250
    ASSERT_EQ(a, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // total 750
    ASSERT_EQ(a, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // total 1250
    ASSERT_EQ(a, 2);
  }
  // queue should be destroyed, make sure a doesn't change anymore
  std::this_thread::sleep_for(std::chrono::milliseconds(1500)); // total 2750
  ASSERT_EQ(a, 2);
}
