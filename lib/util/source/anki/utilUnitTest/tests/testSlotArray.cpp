/**
 * File: testSlotArray
 *
 * Author: baustin
 * Created: 4/10/15
 *
 * Description: Tests functionality of slot array container
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 * --gtest_filter=SlotArrayTest*
 **/

#include "util/helpers/includeGTest.h"
#include "util/container/slotArray.h"

using Anki::Util::SlotArray;

class TestObject
{
public:
  TestObject() : _t(0) { refCount++; }
  TestObject(int t) : _t(t) { refCount++; }
  ~TestObject() { refCount--; }

  TestObject(const TestObject& other) : _t(other._t) { refCount++; }
  TestObject(TestObject&& other) : _t(other._t) { refCount++; }

  int Get() const { return _t; }

  static int refCount;
private:
  int _t;
};

int TestObject::refCount = 0;

TEST(SlotArrayTest, Iterator)
{
  SlotArray<TestObject> container{4};
  TestObject t;
  container.emplace_first(1);
  container.copy_at(2, t);
  auto it = container.begin();
  EXPECT_EQ(container.get_index(*it), 0);
  EXPECT_EQ(it->Get(), 1);
  it++;
  EXPECT_EQ(container.get_index(*it), 2);
  EXPECT_EQ(it->Get(), 0);
  it = container.erase(it);
  EXPECT_EQ(it, container.end());
}

TEST(SlotArrayTest, MoveOperator)
{
  {
    auto f = []() -> SlotArray<TestObject> {
      SlotArray<TestObject> container{4};
      container.emplace_at(2);
      container.emplace_at(0, 1);
      return container;
    };

    SlotArray<TestObject> container{7};
    container.emplace_at(0);
    container.emplace_at(1);
    container.emplace_at(2);
    container.emplace_at(6);

    EXPECT_EQ(TestObject::refCount, 4);
    container = f();
    EXPECT_EQ(TestObject::refCount, 2);
  }
  EXPECT_EQ(TestObject::refCount, 0);
}

TEST(SlotArrayTest, CopyOperator)
{
  {
    SlotArray<TestObject> one{3};
    SlotArray<TestObject> two{4};

    one.emplace_first();
    one.emplace_first();
    two.emplace_first();
    two.emplace_first();
    two.emplace_first();

    EXPECT_EQ(TestObject::refCount, 5);
    one = two;
    EXPECT_EQ(TestObject::refCount, 6);

    SlotArray<TestObject> three{one};
    EXPECT_EQ(TestObject::refCount, 9);
  }
  EXPECT_EQ(TestObject::refCount, 0);
}

TEST(SlotArrayTest, CopyEmplace)
{
  {
    SlotArray<TestObject> container{3};
    TestObject* t0 = container.emplace_first(0);
    TestObject* t1 = container.emplace_first(1);
    TestObject* t2 = container.emplace_first(2);
    EXPECT_TRUE(t0 != nullptr && t0->Get() == 0);
    EXPECT_TRUE(t1 != nullptr && t1->Get() == 1);
    EXPECT_TRUE(t2 != nullptr && t2->Get() == 2);
    TestObject* t3 = container.emplace_first();
    EXPECT_EQ(t3, nullptr);
  }
  {
    SlotArray<TestObject> container{3};
    TestObject t{2};
    TestObject* t0 = container.copy_first(t);
    TestObject* t1 = container.copy_first(t);
    TestObject* t2 = container.copy_first(t);
    EXPECT_TRUE(t0 != nullptr && t0->Get() == 2);
    EXPECT_TRUE(t1 != nullptr && t1->Get() == 2);
    EXPECT_TRUE(t2 != nullptr && t2->Get() == 2);
    TestObject* t3 = container.copy_first(t);
    EXPECT_EQ(t3, nullptr);
  }
  EXPECT_EQ(TestObject::refCount, 0);
}
