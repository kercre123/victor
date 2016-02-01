#ifndef __SPINE_H
#define __SPINE_H

namespace Spine {
  void enqueue(SpineProtocol& msg);
  void dequeue(SpineProtocol& msg);
  void processMessage(SpineProtocol& msg);
}

#endif
