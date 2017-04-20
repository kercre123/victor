/*
 * File:          activeBlock.cpp
 * Date:
 * Description:   Main controller for simulated blocks and chargers
 * Author:        
 * Modifications: 
 */

#include "activeBlock.h"
#include "BlockMessages.h"
#include "clad/types/activeObjectConstants.h"
#include "clad/types/ledTypes.h"
#include "clad/robotInterface/lightCubeMessage.h"
#include "anki/cozmo/robot/ledController.h"
#include "util/logging/logging.h"
#include "util/math/math.h"

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>

#include <webots/Supervisor.hpp>
#include <webots/Receiver.hpp>
#include <webots/Emitter.hpp>
#include <webots/Accelerometer.hpp>
#include <webots/LED.hpp>

webots::Supervisor active_object_controller;


namespace Anki {
namespace Cozmo {
namespace ActiveBlock {

namespace {

  const s32 TIMESTEP = 10; // ms

  // Number of cycles (of length TIMESTEP) in between transmission of ObjectAvailable messages
  const u32 OBJECT_AVAILABLE_MESSAGE_PERIOD = 100;
  
  // Number of cycles (of length TIMESTEP) in between transmission of motion update messages
  // (This is just to throttle movement messages a bit)
  const s32 MOVEMENT_UPDATE_PERIOD = 10;
  
  // Number of cycles (of length TIMESTEP) for which UpAxis must be stable before sending
  //  out an "UpAxisChanged" message. (On actual cube firmware, this is about 525 ms. See
  //  'robot/syscon/cubes/cubes.cpp' for details)
  const s32 UP_AXIS_STABLE_PERIOD = 52;
  
  // once we start moving, we have to stop for this long to send StoppedMoving message
  const double STOPPED_MOVING_TIME_SEC = 0.5f;
  
  webots::Receiver* receiver_;
  webots::Emitter*  emitter_;
  webots::Emitter*  discoveryEmitter_;
  
  webots::Accelerometer* accel_;
  
  // Accelerometer filter window
  const u32 MAX_ACCEL_BUFFER_SIZE = 30;
  f32 accelBuffer_[3][MAX_ACCEL_BUFFER_SIZE];
  u32 accelBufferStartIdx_ = 0;
  u32 accelBufferSize_ = 0;
  
  // High-pass filter params for tap detection
  const f32 TAP_DETECT_THRESH = 9;
  const u32 TAP_DETECT_WINDOW_MS = 100;
  const f32 CUTOFF_FREQ = 50;
  const f32 RC = 1.0f/(CUTOFF_FREQ*2*3.14f);
  const f32 dt = 0.001f*TIMESTEP;
  const f32 alpha = RC/(RC + dt);
  
  
  typedef enum {
    NORMAL = 0,
    FLASHING_ID,
    BEING_CARRIED
  } BlockState;
  
  s32 blockID_ = -1;
  
  BlockState state_ = NORMAL;
  
  webots::LED* led_[NUM_CUBE_LEDS];
  webots::Field* ledColorField_ = nullptr;

  LightState ledParams_[NUM_CUBE_LEDS];
  TimeStamp_t ledPhases_[NUM_CUBE_LEDS];
  
  // To keep track of previous (and next) UpAxis
  //  sent to the robot (via UpAxisChanged msg)
  UpAxis prevUpAxis_ = UnknownAxis;
  UpAxis nextUpAxis_ = UnknownAxis;
  
  bool wasMoving_;
  double wasLastMovingTime_sec_;
  
  bool streamAccel_ = false;
  
  // Lookup table for which four LEDs are in the back, left, front, and right positions,
  // given the current up axis.
  // When the top marker is facing up, these positions are with respect to the top marker.
  // When the top marker is facing horizontally, these positions are with respect to the marker
  // as if it were oriented front-side up.
  // When the top marker is facing down, these positions are with respect to the marker
  // as if you were looking down on it (through the top of the block).
  const u8 ledIndexLUT[NumAxes][NUM_CUBE_LEDS] =
  {
    // Xneg (Front Face on top)
    {0, 1, 2, 3},
    
    // Xpos (Back Face on top)
    {2, 3, 0, 1},
    
    // Yneg (Right on top)
    {1, 2, 3, 0},
    
    // Ypos (Left Face on top)
    {3, 0, 1, 2},
    
    // Zneg (Bottom Face on top) -- NOTE: Flipped 180deg around X axis!!
    {0, 3, 2, 1},
    
    // Zpos (Top Face on top)
    {0, 1, 2, 3}
  };
  
  // Mapping from block ID to ObjectType
  const ObjectType blockIDToObjectType_[MAX_NUM_CUBES] = { Block_LIGHTCUBE1, Block_LIGHTCUBE2, Block_LIGHTCUBE3, Charger_Basic };
  u32 factoryID_ = 0;
  ObjectType objectType_ = UnknownObject;
  
  // Flash ID params
  double flashIDStartTime_ = 0;
  
  // For now, flashing ID means turning the LED off for a bit, then red for a bit,
  // then off for a bit, then resuming the regular pattern
  const double flashID_t1_off = 0.025;
  const double flashID_t2_on = 0.1;
  const double flashID_t3_off = 0.025;
  
  
} // private namespace

// Handle the shift by 8 bits to remove alpha channel from 32bit RGBA pixel
// to make it suitable for webots LED 24bit RGB color
inline void SetLED_helper(u32 index, u32 rgbaColor) {
  led_[index]->set(rgbaColor);

  double red = (rgbaColor & 0x00ff0000) >> 16;  // get first byte
  double green = (rgbaColor & 0x0000ff00) >> 8;  // second byte
  double blue = (rgbaColor & 0x000000ff);  // third byte
  double betterColor[3] = {red, green, blue};
  if (ledColorField_) {
    ledColorField_->setMFVec3f(index, betterColor);
    //const double *p = ledColorField_->getMFVec3f(index);
    //PRINT_NAMED_WARNING("SetLED", "Cube %d, LED %d, Color %f %f %f (%0llx)", blockID_, index, p[0], p[1], p[2], (u64)p);
  }
}

// ========== Callbacks for messages from robot =========
void Process_flashID(const FlashObjectIDs& msg)
{
  state_ = FLASHING_ID;
  flashIDStartTime_ = active_object_controller.getTime();
  //printf("Starting ID flash\n");
}

void Process_setCubeID(const CubeID& msg)
{
  
}

void Process_setCubeLights(const CubeLights& msg)
{
  // See if the message is actually changing anything about the block's current
  // state. If not, don't update anything.
  if(memcmp(ledParams_, msg.lights, sizeof(ledParams_))) {
    memcpy(ledParams_, msg.lights, sizeof(ledParams_));
    for (int i=0; i<NUM_CUBE_LEDS; ++i) ledPhases_[i] = 0;
  } else {
    //printf("Ignoring SetBlockLights message with parameters identical to current state.\n");
  }
  
  // Set lights immediately
  for (u32 i=0; i<NUM_CUBE_LEDS; ++i) {
    // The color in the CLAD structure LightState is 16 bits structed as the following in binary:
    // irrrrrgggggbbbbb, that's 1 bit for infrared, 5 bits for red, green, blue, respectively.
    const u32 newColor = ((msg.lights[i].onColor & LED_ENC_RED) << (16 - 10 + 3)) |
                         ((msg.lights[i].onColor & LED_ENC_GRN) << ( 8 -  5 + 3)) |
                         ((msg.lights[i].onColor & LED_ENC_BLU) << ( 0 -  0 + 3));
    SetLED_helper(i, newColor);
  }
  
}

void Process_setObjectBeingCarried(const ObjectBeingCarried& msg)
{
  const bool isBeingCarried = static_cast<bool>(msg.isBeingCarried);
  state_ = (isBeingCarried ? BEING_CARRIED : NORMAL);
}

void Process_streamObjectAccel(const StreamObjectAccel& msg)
{
  streamAccel_ = msg.enable;
}
  
// Stubs to make linking work until we clean up how simulated cubes work.
void Process_moved(const ObjectMoved& msg) {}
void Process_stopped(const ObjectStoppedMoving& msg) {}
void Process_tapped(const ObjectTapped& msg) {}
void Process_upAxisChanged(const ObjectUpAxisChanged& msg) {}
void Process_accel(const ObjectAccel& msg) {}
  
void ProcessBadTag_LightCubeMessage(BlockMessages::LightCubeMessage::Tag badTag)
{
  printf("Ignoring LightCubeMessage with invalid tag: %d\n", badTag);
}
// ================== End of callbacks ==================

// Returns the ID of the block which is used as the
// receiver channel for comms with the robot.
// This is not meant to reflect how actual comms will work.
s32 GetBlockID() {
  webots::Node* selfNode = active_object_controller.getSelf();
  
  webots::Field* activeIdField = selfNode->getField("ID");
  if(activeIdField) {
    return activeIdField->getSFInt32();
  } else {
    return -1;
  }
}

void Process_available(const ObjectAvailable& msg)
{
  //stub
}

Result Init()
{
  state_ = NORMAL;

  active_object_controller.step(TIMESTEP);
  
  
  webots::Node* selfNode = active_object_controller.getSelf();
  
  // Get this block's ID
  webots::Field* activeIdField = selfNode->getField("ID");
  if(activeIdField) {
    blockID_ = activeIdField->getSFInt32();
  } else {
    printf("Failed to find active object ID\n");
    return RESULT_FAIL;
  }
  
  
  // Get active object type
  webots::Field* nodeName = selfNode->getField("name");
  if (nodeName) {
    std::string name = nodeName->getSFString();
    if (name.compare("LightCube") == 0) {
      objectType_ = blockIDToObjectType_[blockID_];
      if (blockID_ > 2) {
        printf("Expecting charger to have ID < 3\n");
        return RESULT_FAIL;
      }
    } else if (name.compare("CozmoCharger") == 0) {
      objectType_ = Charger_Basic;
      if (blockID_ != 3) {
        printf("Expecting charger to have ID 3\n");
        return RESULT_FAIL;
      }
    }
  } else {
    printf("Active object has no name\n");
    return RESULT_FAIL;
  }
  
  // Generate a factory ID
  // If PROTO factoryID is > 0, use that.
  // Otherwise use blockID as factoryID.
  webots::Field* factoryIdField = selfNode->getField("factoryID");
  if (factoryIdField) {
    s32 fID = factoryIdField->getSFInt32();
    if (fID > 0) {
      factoryID_ = fID;
    } else {
      factoryID_ = blockID_ + 1;
    }
  }
  factoryID_ &= 0x7FFFFFFF; // Make sure it doesn't get mistaken for a charger
  PRINT_NAMED_INFO("ActiveBlock", "Starting active object %d (factoryID %d)", blockID_, factoryID_);
  
  
  // Get all LED handles
  for (int i=0; i<NUM_CUBE_LEDS; ++i) {
    char led_name[32];
    sprintf(led_name, "led%d", i);
    led_[i] = active_object_controller.getLED(led_name);
    assert(led_[i] != nullptr);
  }
  
  // Field for monitoring color from webots tests
  ledColorField_ = selfNode->getField("ledColors");
  
  // Get radio receiver
  receiver_ = active_object_controller.getReceiver("receiver");
  assert(receiver_ != nullptr);
  receiver_->enable(TIMESTEP);
  receiver_->setChannel(factoryID_ + 1);
  
  // Get radio emitter
  emitter_ = active_object_controller.getEmitter("emitter");
  assert(emitter_ != nullptr);
  emitter_->setChannel(factoryID_);
  
  // Get radio emitter for discovery
  discoveryEmitter_ = active_object_controller.getEmitter("discoveryEmitter");
  assert(discoveryEmitter_ != nullptr);
  discoveryEmitter_->setChannel(OBJECT_DISCOVERY_CHANNEL);
  
  // Get accelerometer
  accel_ = active_object_controller.getAccelerometer("accel");
  assert(accel_ != nullptr);
  accel_->enable(TIMESTEP);
  
  wasMoving_ = false;
  wasLastMovingTime_sec_ = 0.0;
  
  // Register callbacks
  // I don't think these are relivant anymore. ~Daniel
  //RegisterCallbackForMessageFlashID(ProcessFlashIDMessage);
  //RegisterCallbackForMessageSetBlockLights(ProcessSetBlockLightsMessage);
  
  return RESULT_OK;
}

void DeInit() {
  if (receiver_) {
    receiver_->disable();
  }
  if (accel_) {
    accel_->disable();
  }
}


UpAxis GetCurrentUpAxis()
{
#       define X 0
#       define Y 1
#       define Z 2
  const double* accelVals = accel_->getValues();

  // Determine the axis with the largest absolute acceleration
  UpAxis retVal = UnknownAxis;
  double maxAbsAccel = -1;
  double absAccel[3];

  s32 whichAxis = -1;
  for(s32 i=0; i<3; ++i) {
    absAccel[i] = fabs(accelVals[i]);
    if(absAccel[i] > maxAbsAccel) {
      whichAxis = i;
      maxAbsAccel = absAccel[i];
    }
  }

//        printf("Block %d acceleration: (%.3f, %.3f, %.3f)\n", blockID_,
//               accelVals[X], accelVals[Y], accelVals[Z]);
  
  // Return the corresponding signed axis
  switch(whichAxis)
  {
    case X:
      retVal = (accelVals[X] < 0 ? XNegative : XPositive);
      break;
      
    case Y:
      retVal = (accelVals[Y] < 0 ? YNegative : YPositive);
      break;

    case Z:
      retVal = (accelVals[Z] < 0 ? ZNegative : ZPositive);
      break;
      
    default:
      printf("Unexpected whichAxis = %d\n", whichAxis);
  }
  
  return retVal;
  
#       undef X
#       undef Y
#       undef Z
} // GetCurrentUpAxis()

/*
TopFrontFacePair GetOrientation() {
  const double* vals = accel_->getValues();
  const double x = vals[0];
  const double y = vals[1];
  const double z = vals[2];
  
  const double g = 9;
  TopFrontFacePair fp = Xz;
  //printf("ActiveBlock accel: %f %f %f\n", x, y, z);
  
  
  if (x > g) {
    fp = Xz;
  } else if (x < -g) {
    return xZ;
  } else if (y > g) {
    fp = Yx;
  } else if (y < -g) {
    fp = yX;
  } else if (z > g) {
    fp = ZX;
  } else if (z < -g) {
    fp = zx;
  } else {
    //printf("WARN: Block is moving. Orientation unknown\n");
  }
  
  return fp;
}
 */


void SetLED(WhichCubeLEDs whichLEDs, u32 color)
{
  static const u8 FIRST_BIT = 0x01;
  u8 shiftedLEDs = static_cast<u8>(whichLEDs);
  for(u8 i=0; i<NUM_CUBE_LEDS; ++i) {
    if(shiftedLEDs & FIRST_BIT) {
      SetLED_helper(ledIndexLUT[prevUpAxis_][i], color);
    }
    shiftedLEDs = shiftedLEDs >> 1;
  }
}

inline void SetLED(u32 ledIndex, u32 color)
{
  SetLED_helper(ledIndexLUT[prevUpAxis_][ledIndex], color);
}


void SetAllLEDs(u32 color) {
  for(int i=0; i<NUM_CUBE_LEDS; ++i) {
    SetLED_helper(i, color);
  }
}

// Returns true if a tap was detected
// Given the unfiltered accel values of the current frame, these values are
// accumulated into a buffer. Once the buffer has filled, apply high-pass
// filter to each axis' buffer. If there is a value that exceeds a threshold
// a tap is considered to have been detected.
// When a tap is detected
bool CheckForTap(f32 accelX, f32 accelY, f32 accelZ)
{
  bool tapDetected = false;
  
  // Add accel values to buffer
  u32 newIdx = accelBufferStartIdx_;
  if (accelBufferSize_ < MAX_ACCEL_BUFFER_SIZE-1) {
    newIdx += accelBufferSize_;
    ++accelBufferSize_;
  } else {
    accelBufferStartIdx_ = (accelBufferStartIdx_+1) % MAX_ACCEL_BUFFER_SIZE;
    accelBufferSize_ = MAX_ACCEL_BUFFER_SIZE;
  }
  newIdx = newIdx % MAX_ACCEL_BUFFER_SIZE;
  
  accelBuffer_[0][newIdx] = accelX;
  accelBuffer_[1][newIdx] = accelY;
  accelBuffer_[2][newIdx] = accelZ;
  
  if (accelBufferSize_ == MAX_ACCEL_BUFFER_SIZE) {
    
    // Compute high-pass filtered values
    for (u8 axis = 0; axis < 3 && !tapDetected; ++axis) {
      
      f32 prevAccelVal = accelBuffer_[axis][accelBufferStartIdx_];
      f32 prevAccelFiltVal = prevAccelVal;
      for (u32 i=1; i<MAX_ACCEL_BUFFER_SIZE; ++i) {
        u32 idx = (accelBufferStartIdx_ + i) % MAX_ACCEL_BUFFER_SIZE;
        f32 currAccelFiltVal = alpha * (prevAccelFiltVal + accelBuffer_[axis][idx] - prevAccelVal);
        prevAccelVal = accelBuffer_[axis][idx];
        prevAccelFiltVal = currAccelFiltVal;
        
        if (currAccelFiltVal > TAP_DETECT_THRESH) {
          PRINT_NAMED_INFO("ActiveBlock", "TapDetected: axis %d, val %f", axis, currAccelFiltVal);
          tapDetected = true;
          
          // Fast forward in buffer so that we don't allow tap detection again until
          // TAP_DETECT_WINDOW_MS later.
          u32 idxOffset = i + (TAP_DETECT_WINDOW_MS / TIMESTEP);
          accelBufferStartIdx_ = (accelBufferStartIdx_ + idxOffset) % MAX_ACCEL_BUFFER_SIZE;
          if (accelBufferSize_ > idxOffset) {
            accelBufferSize_ -= idxOffset;
          } else {
            accelBufferSize_ = 0;
          }
          
          break;
        }
      }
    }
  }
  
  return tapDetected;
}


Result Update() {
  if (active_object_controller.step(TIMESTEP) != -1) {
    
#if(0)
    // Test: Blink all LEDs in order
    static u32 p=0;
    //static BlockLEDPosition prevIdx = TopFrontLeft, idx = TopFrontLeft;
    static WhichCubeLEDs prevIdx = WhichCubeLEDs::TOP_UPPER_LEFT, idx = WhichCubeLEDs::TOP_UPPER_LEFT;
    if (p++ == 100) {
      SetLED(prevIdx, 0);
      if (idx == WhichCubeLEDs::TOP_UPPER_LEFT) {
        // Top upper left is green
        SetLED(idx, 0x00ff0000);
      } else {
        // All other LEDs are red
        SetLED(idx, 0xff000000);
      }
      prevIdx = idx;

      // Increment LED position index
      idx = static_cast<WhichCubeLEDs>(static_cast<u8>(idx)<<1);
      if(idx == WhichCubeLEDs::NONE) {
        idx = WhichCubeLEDs::TOP_UPPER_LEFT;
      }
      p = 0;
    }
    return RESULT_OK;
#endif
    
    // Read incoming messages
    while (receiver_->getQueueLength() > 0) {
      int dataSize = receiver_->getDataSize();
      u8* data = (u8*)receiver_->getData();
      BlockMessages::ProcessMessage(data, dataSize);
      receiver_->nextPacket();
    }
    
    // TODO: Time probably won't be in seconds on the real blocks...
    const double currTime_sec = active_object_controller.getTime();
    
    // Get accel values
    const double* accelVals = accel_->getValues();
    const double accelMagSq = accelVals[0]*accelVals[0] + accelVals[1]*accelVals[1] + accelVals[2]*accelVals[2];
    
    
    
    //////////////////////
    // Check for taps
    //////////////////////
    if (CheckForTap(accelVals[0], accelVals[1], accelVals[2])) {
      BlockMessages::LightCubeMessage msg;
      msg.tag = BlockMessages::LightCubeMessage::Tag_tapped;
      msg.tapped.objectID = blockID_;
      msg.tapped.numTaps = 1;
      msg.tapped.tapNeg = -50;  // Hard-coded tap intensity.
      msg.tapped.tapPos = 50;   // Just make sure that tapPos - tapNeg > BlockTapFilterComponent::kTapIntensityMin
      msg.tapped.tapTime = static_cast<u32>(active_object_controller.getTime() / 0.035f) % std::numeric_limits<u8>::max();  // Each tapTime count should be 35ms
      emitter_->send(msg.GetBuffer(), msg.Size());
    }

    
    // Send ObjectAvailable message
    static u32 objAvailableSendCtr = 0;
    if (++objAvailableSendCtr == OBJECT_AVAILABLE_MESSAGE_PERIOD) {
      BlockMessages::LightCubeMessage msg;
      msg.tag = BlockMessages::LightCubeMessage::Tag_available;
      msg.available.factory_id = factoryID_;
      msg.available.objectType = objectType_;
      discoveryEmitter_->send(msg.GetBuffer(), msg.Size());
      objAvailableSendCtr = 0;
    }
    
    // Send accel data
    if (streamAccel_) {
      BlockMessages::LightCubeMessage msg;
      msg.tag = BlockMessages::LightCubeMessage::Tag_accel;
      msg.accel.objectID = blockID_;
      f32 scaleFactor = 32.f/9.81f;  // 32 on physical blocks ~= 1g
      msg.accel.accel.x = accelVals[0] * scaleFactor;
      msg.accel.accel.y = accelVals[1] * scaleFactor;
      msg.accel.accel.z = accelVals[2] * scaleFactor;
      emitter_->send(msg.GetBuffer(), msg.Size());
    }
    
    // Run FSM
    switch(state_)
    {
      case NORMAL:
      {
        static s32 movingSendCtr = 0;
        
        if(++movingSendCtr == MOVEMENT_UPDATE_PERIOD)
        {
          movingSendCtr = 0;
          
          // Determine if block is moving by seeing if the magnitude of the
          // acceleration vector is different from gravity
          const bool isMoving = !NEAR(accelMagSq, 9.81*9.81, 1.0);
          
          if(isMoving) {
            PRINT_NAMED_INFO("ActiveBlock", "Block %d appears to be moving (UpAxis=%d).", blockID_, prevUpAxis_);
            
            // TODO: There should really be a message ID in here, rather than just determining it from message size on the other end.
            BlockMessages::LightCubeMessage msg;
            msg.tag = BlockMessages::LightCubeMessage::Tag_moved;
            msg.moved.objectID = blockID_;
            msg.moved.accel.x = accelVals[0];
            msg.moved.accel.y = accelVals[1];
            msg.moved.accel.z = accelVals[2];
            msg.moved.axisOfAccel = prevUpAxis_;
            emitter_->send(msg.GetBuffer(), msg.Size());
            
            wasLastMovingTime_sec_ = currTime_sec;
            wasMoving_ = true;
            
          } else if(wasMoving_ && currTime_sec - wasLastMovingTime_sec_ > STOPPED_MOVING_TIME_SEC ) {
            
            PRINT_NAMED_INFO("ActiveBlock", "Block %d stopped moving (UpAxis=%d).", blockID_, prevUpAxis_);
            
            // Send "UpAxisChanged" message now if necessary, so that it's
            //  sent before the stopped message (to mirror what the actual
            //  cube firmware does - see 'robot/syscon/cubes/cubes.cpp')
            if (prevUpAxis_ != nextUpAxis_) {
              prevUpAxis_ = nextUpAxis_;
              BlockMessages::LightCubeMessage msg;
              msg.tag = BlockMessages::LightCubeMessage::Tag_upAxisChanged;
              msg.upAxisChanged.objectID = blockID_;
              msg.upAxisChanged.upAxis = nextUpAxis_;
              emitter_->send(msg.GetBuffer(), msg.Size());
            }
            
            // Send the "Stopped" message.
            // TODO: There should really be a message ID in here, rather than just determining it from message size on the other end.
            BlockMessages::LightCubeMessage msg;
            msg.tag = BlockMessages::LightCubeMessage::Tag_stopped;
            msg.stopped.objectID = blockID_;
            emitter_->send(msg.GetBuffer(), msg.Size());
            
            wasMoving_ = false;
          }
        } // if(SEND_MOVING_MESSAGES_EVERY_N_TIMESTEPS)
        
        // Grab the current UpAxis (from accelerometer)
        UpAxis currentUpAxis = GetCurrentUpAxis();
        
        // Check for a stable UpAxis:
        static s32 nextUpAxisCtr = 0;
        if (currentUpAxis == nextUpAxis_) {
          if (nextUpAxis_ != prevUpAxis_) {
            if (++nextUpAxisCtr > UP_AXIS_STABLE_PERIOD) {
              prevUpAxis_ = nextUpAxis_;
              // Send "UpAxisChanged" message:
              BlockMessages::LightCubeMessage msg;
              msg.tag = BlockMessages::LightCubeMessage::Tag_upAxisChanged;
              msg.upAxisChanged.objectID = blockID_;
              msg.upAxisChanged.upAxis = nextUpAxis_;
              emitter_->send(msg.GetBuffer(), msg.Size());
            }
          } else {
            nextUpAxisCtr = 0;
          }
        } else {
          nextUpAxisCtr = 0;
          nextUpAxis_ = currentUpAxis;
        }
        
        break;
      }
        
      case BEING_CARRIED:
      {
        // Don't check for movement since we're being carried
        break;
      }

        
      case FLASHING_ID:
        if (currTime_sec >= flashIDStartTime_ + flashID_t1_off + flashID_t2_on + flashID_t3_off) {
          state_ = NORMAL;
        } else if (currTime_sec >= flashIDStartTime_ + flashID_t1_off + flashID_t2_on) {
          SetAllLEDs(0);
        } else if (currTime_sec >= flashIDStartTime_ + flashID_t1_off) {
          SetAllLEDs(0xff0000);
        } else if (currTime_sec >= flashIDStartTime_) {
          SetAllLEDs(0);
        }
        break;
      default:
        printf("WARNING (ActiveBlock): Unknown state %d\n", state_);
        break;
    }
    
  
    return RESULT_OK;
  }
  return RESULT_FAIL;
}


}  // namespace ActiveBlock
}  // namespace Cozmo
}  // namespace Anki
