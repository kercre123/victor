//
//  compareFcns.h
//
//  Created by Kevin Yoon on 7/24/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#ifndef CORETECH_ENGINE_UTIL_COMPAREFCNS
#define CORETECH_ENGINE_UTIL_COMPAREFCNS

namespace Anki {
  
  // Compare functions for pairs based on first or second element only
  template < class X , class Y >
  struct CompareFirst
  {
    bool operator() (const std::pair<X,Y>& a, const std::pair<X,Y>& b) {
      return a.first < b.first;
    }
  };
  
  template < class X , class Y >
  struct CompareSecond
  {
    bool operator() (const std::pair<X,Y>& a, const std::pair<X,Y>& b) {
      return a.second < b.second;
    }
  };

}

#endif // CORETECH_ENGINE_UTIL_PRINT_COMPAREFCNS
