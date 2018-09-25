/**
 * File: testPointerHandle
 *
 * Author: baustin
 * Created: 12/17/15
 *
 * Description: Tests handles to externally owned pointer allocations
 * Full documentation/explanation of code available at:
 *   https://github.com/anki/wip/blob/master/baustin/template-rd/blogs/2015-12-17-pointer-handle.md
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "util/helpers/includeGTest.h"
#include "util/helpers/pointerHandle.h"

namespace Anki {
namespace Util {

static int a = 0;

class Base {
public:

  virtual ~Base() = default;

  virtual void Func() {
    a++;
  }
};

class Derived : public Base {
public:
  virtual void Func() override {
    a--;
  }
};

class Unrelated {
public:
  virtual void Func() {}
};

using BaseHandle = ExternalPointerHandle<Base>;
using DerivedHandle = BaseHandle::SubHandle<Derived>;
using SuperDerivedHandle = ExternalPointerHandle<Derived>;
using UnrelatedHandle = BaseHandle::SubHandle<Unrelated>; // we should not be able to use this meaningfully

TEST(TestPointerHandle, TestPointerHandle)
{
  {
    DerivedHandle derived;
    BaseHandle base(derived); // can construct base from derived
    //DerivedHandle derived2(base); // not allowed to construct derived from base
    DerivedHandle derived3 = base.cast<Derived>(); // allowed with explicit cast

    // unfortunate that we can instantiate this - it's a tradeoff, we could forbid it in the definition
    // of ExternalPointerHandle, but then any headers that refer to Handles will need to know complete
    // type information of the contained types and include their headers
    UnrelatedHandle unrelated;
    // fortunately this next line won't compile, as you can't cast to Unrelated from Base
    //Unrelated* unrelatedPtr = unrelated.get();

    EXPECT_EQ(nullptr, derived.get());
    EXPECT_EQ(nullptr, base.get());
    EXPECT_FALSE(derived);
    EXPECT_FALSE(base);
    EXPECT_EQ(0, a);
  }
  {
    BaseHandle base;
    {
      BaseHandle::Owner owner(new Base);
      base = BaseHandle(owner);
      if (!base) {
        FAIL();
      }
      base->Func();
      EXPECT_EQ(1, a);
    }
    if (base) {
      FAIL();
    }

    DerivedHandle outerDerived;
    {
      BaseHandle::Owner owner(new Derived);
      base = BaseHandle(owner);
      if (!base) {
        FAIL();
      }
      base->Func();
      EXPECT_EQ(0, a);
      DerivedHandle derived = base.cast<Derived>();
      outerDerived = derived;
      if (!outerDerived || !derived) {
        FAIL();
      }
      (*derived).Func();
      EXPECT_EQ(-1, a);
    }
    if (base || outerDerived) {
      FAIL();
    }

    {
      DerivedHandle::Owner owner(new Derived);
      base = BaseHandle(owner);
      if (!base) {
        FAIL();
      }
      DerivedHandle derived = base.cast<Derived>();
      if (!derived) {
        FAIL();
      }
      base->Func();
      EXPECT_EQ(-2, a);
      derived->Func();
      EXPECT_EQ(-3, a);
    }
    EXPECT_EQ(nullptr, base.get());

    {
      a = 0;
      SuperDerivedHandle::Owner owner(new Derived);
      SuperDerivedHandle superDerived(owner);
      base = BaseHandle(owner);
      if (!base) {
        FAIL();
      }
      DerivedHandle derived = base.cast<Derived>();
      if (!derived || !superDerived) {
        FAIL();
      }
      base->Func();
      EXPECT_EQ(-1, a);
      derived->Func();
      EXPECT_EQ(-2, a);
      superDerived->Func();
      EXPECT_EQ(-3, a);
    }
    if (base) {
      FAIL();
    }
    EXPECT_EQ(nullptr, base.get());
  }
}

}
}
