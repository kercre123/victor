//
//  EndianHelper.h
//  ios-server
//
//  Created by Paul Aluri on 1/26/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef EndianHelper_h
#define EndianHelper_h

#include <stdlib.h>

class EndianHelper {
public:
  static bool IsHostLittleEndian() {
    uint32_t value = 0x01;
    return *((char*)(&value)) == 0x01;
  }
  
  template <typename value>
  static value ConvertHostToNetworkOrder(value a) {
    return ConvertHostToBigEndian(a);
  }
  
  template <typename value>
  static value ConvertNetworkOrderToHost(value a) {
    return ConvertBigEndianToHost(a);
  }
  
  template <typename value>
  static value ConvertHostToLittleEndian(value a) {
    if(IsHostLittleEndian()) {
      return a;
    } else {
      return ReverseBytesCopy(a);
    }
  }
  
  template <typename value>
  static value ConvertHostToBigEndian(value a) {
    if(!IsHostLittleEndian()) {
      return a;
    } else {
      return ReverseBytesCopy(a);
    }
  }
  
  template <typename value>
  static value ConvertLittleEndianToHost(value a) {
    if(IsHostLittleEndian()) {
      return a;
    } else {
      return ReverseBytesCopy(a);
    }
  }
  
  template <typename value>
  static value ConvertBigEndianToHost(value a) {
    if(!IsHostLittleEndian()) {
      return a;
    } else {
      return ReverseBytesCopy(a);
    }
  }
  
private:
  template <typename T>
  static T ReverseBytesCopy(const T& value) {
    /*htonl(<#x#>)
    htons(x);
    htonll(x);
    ntohl(<#x#>);
    ntohs(<#x#>);
    ntohll(<#x#>);*/
    
    T output = value;
    
    std::reverse((char*)(&output), (char*)(&output) + sizeof(T));
    
    return output;
  }
};

#endif /* EndianHelper_h */
