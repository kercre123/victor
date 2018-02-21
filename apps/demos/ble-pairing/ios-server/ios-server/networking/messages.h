//
//  messages.h
//  ios-server
//
//  Created by Paul Aluri on 2/13/18.
//  Copyright © 2018 Paul Aluri. All rights reserved.
//

#ifndef messages_hpp
#define messages_hpp

#include <stdlib.h>

namespace Anki {
namespace Switchboard {
  class MessageProtocol {
  public:
    bool ReceiveRawBuffer(uint8_t* buffer, size_t size, uint8_t* bufferOut, size_t* sizeOut);
    void SendRawBuffer(uint8_t* buffer, size_t size);
  };
} // Switchboard
} // Anki

#endif /* messages_hpp */
