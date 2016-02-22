#ifndef __SPINE_H
#define __SPINE_H

namespace Spine {
  /// Enqueue CLAD data to send to body.
  // @warning Only call from main executation thread
  bool enqueue(const u8* data, const u8 length);
  /// Dequeu data for body.
  // @warning Only call from hal exec thread
  int  dequeue(u8* dest, const u8 maxLength);
  void processMessage(SpineProtocol& msg);
}

#endif
