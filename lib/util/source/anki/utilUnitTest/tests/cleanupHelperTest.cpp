//  cleanupHelperTest.cpp
//  Anki-Util
//
//  Created by Andrew Stein on 4/11/16.
//  Copyright (c) 2016 Anki, Inc. All rights reserved.
//
// --gtest_filter=CleanupHelperTest*

#include "util/helpers/includeGTest.h"

#include "util/helpers/cleanupHelper.h"

TEST(CleanupHelperTest, ScopedCleanup)
{
  std::cout << "Testing CleanupHelper\n";
  
  bool cleanupDone = false;
  
  {
    // Construct cleanup object
    Anki::Util::CleanupHelper cleanupTester([&cleanupDone]() {
      cleanupDone = true;
    });
    
    // When it goes out of scope here and destructs, cleanupDone should get set
    // to true by the lambda passed into the CleanupHelper's constructor
  }
  
  ASSERT_TRUE(cleanupDone);
}
