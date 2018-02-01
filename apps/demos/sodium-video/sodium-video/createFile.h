//
//  createFile.hpp
//  sodium-video
//
//  Created by Paul Aluri on 1/29/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef createFile_hpp
#define createFile_hpp

#include <stdlib.h>
#include <stdio.h>
#include "sodium.h"

class SodiumVideo {
public:
  SodiumVideo();
  
private:
  unsigned char _ServerRx[crypto_kx_SESSIONKEYBYTES];
  unsigned char _ServerTx[crypto_kx_SESSIONKEYBYTES];
  
  void GenerateKeys();
};

#endif /* createFile_hpp */
