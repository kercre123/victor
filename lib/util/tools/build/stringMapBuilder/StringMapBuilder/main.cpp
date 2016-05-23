//
//  main.cpp
//  StringMapBuilder
//
//  Created by damjan stulic on 10/26/13.
//  Copyright (c) 2013 damjan stulic. All rights reserved.
//

#include <iostream>
#include <string>
#include "StringMapBuilder.h"

int main(int argc, const char * argv[])
{

  if (argc != 4)
  {
    std::cout << "StringMapBuilder <source_path> <work_path> <config.json>\n";
    std::cout << "or run stringMapBuilder.sh\n";
    return -1;
  }

  std::string sourcePath(argv[1]);
  std::string workPath(argv[2]);
  std::string configJson(argv[3]);
  if (StringMapBuilder::Build(sourcePath, workPath, configJson, false)) {
    std::cout << "StringMapBuilder finished successfully\n";
    return 0;
  } else {
    std::cout << "StringMapBuilder finished with errors. String map.json should not be trusted!\n";
    return 1;
  }
}
