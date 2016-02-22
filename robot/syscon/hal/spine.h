#ifndef __SPINE_H
#define __SPINE_H

namespace Spine {
  bool enqueue(const u8* data, const u8 length);
  int  dequeue(u8* dest, const u8 maxLength);
  void processMessage(SpineProtocol& msg);
}

#endif
