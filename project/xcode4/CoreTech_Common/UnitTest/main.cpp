//
//  main.cpp
//  UnitTest
//
//  Created by Kevin Yoon on 9/9/13.
//  Copyright (c) 2013 Anki, Inc. All rights reserved.
//

#include <iostream>
#include "gtest/gtest.h"


GTEST_API_ int main(int argc, char * argv[])
{
  char *path=NULL;
  size_t size = 0;
  path=getcwd(path,size);
  std::cout<<"hello world! I am " << argv[0] << "\nRunning in " << path << "\nLet the testing commence\n";
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
