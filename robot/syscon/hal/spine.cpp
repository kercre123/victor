#include <stdint.h>
#include <string.h>

#include "anki/cozmo/robot/spineData.h"

#include "radio.h"
#include "spine.h"

static const int FIFO_LIMIT = 4;
static SpineProtocol queue[FIFO_LIMIT];

static int fifoStart = 0;
static int fifoEnd = 0;
static int fifoCount = 0;

namespace Spine {
  void enqueue(SpineProtocol& msg) {
    if (fifoCount == FIFO_LIMIT) {
      return ;
    }

    fifoCount++;
    memcpy(&queue[fifoEnd], &msg, sizeof(SpineProtocol));
      
    if (++fifoEnd > FIFO_LIMIT) fifoEnd = 0;
  }
  
  void dequeue(SpineProtocol& msg) {
    if (fifoCount == 0) {
      return ;
    }
    
    fifoCount--;
    memcpy(&msg, &queue[fifoStart], sizeof(SpineProtocol));
    
    if (++fifoStart > FIFO_LIMIT) fifoStart = 0;
  }

  void processMessage(SpineProtocol& msg) {
    switch (msg.opcode) {
    case ASSIGN_PROP:
      Radio::assignProp(msg.AssignProp.slot, msg.AssignProp.prop_id);
      break ;
    case SET_PROP_STATE:
      uint16_t colors[4];
      memcpy(&colors, (void*)&msg.SetPropState.colors, sizeof(colors));
      Radio::setPropState(msg.SetPropState.slot, colors);
      break ;
    }
  }
}
