//
//  main.cpp
//  dasTest
//
//  Created by damjan stulic on 1/23/15.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#include <iostream>
#pragma GCC diagnostic push

#if __has_warning("-Wundef")
#pragma GCC diagnostic ignored "-Wundef"
#endif

#include "gtest/gtest.h"

#pragma GCC diagnostic pop

int main(int argc, char * argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
