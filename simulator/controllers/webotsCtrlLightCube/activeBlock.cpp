/*
 * File:          activeBlock.cpp
 * Date:
 * Description:   Main controller for simulated blocks
 * Author:        
 * Modifications: 
 */

#include "activeBlock.h"
#include "BlockMessages.h"
#include "clad/types/ledTypes.h"
#include "clad/externalInterface/lightCubeMessage.h"
#include "clad/externalInterface/messageFromActiveObject.h"
#include "anki/cozmo/robot/ledController.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "util/logging/logging.h"
#include "util/helpers/templateHelpers.h"
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

#include <array>
#include <map>

webots::Supervisor active_object_controller;


namespace Anki {
namespace Cozmo {
namespace ActiveBlock {

namespace {

  const s32 TIMESTEP = 10; // ms
  
  u32 TimestampToLedFrames(TimeStamp_t ts) { return (u32)(ts / CUBE_LED_FRAME_LENGTH_MS); }

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
  
  constexpr int kNumCubeLeds = Util::EnumToUnderlying(CubeConstants::NUM_CUBE_LEDS);
  constexpr auto NumAxes = Util::EnumToUnderlying(UpAxis::NumAxes);
  
  webots::Receiver* receiver_;
  webots::Emitter*  emitter_;
  webots::Emitter*  discoveryEmitter_;
  
  // Webots comm channel used for the discovery emitter/receiver
  constexpr int kDiscoveryChannel = 99;
  
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
  } BlockState;
  
  // This block's "ActiveID". Note that engine will set its own
  // activeID for this cube, and eventually the messaging will
  // change and this will be removed from the raw cube messages.
  s32 blockID_ = -1;
  
  BlockState state_ = NORMAL;
  
  // Pointers to the LED object to set the simulated cube's lights
  webots::LED* led_[kNumCubeLeds];
  
  // Pointer to the webots MFVec3f field which mirrors the current LED
  // colors so that the webots tests can monitor the current color.
  webots::Field* ledColorField_ = nullptr;
  // Updates to this field must be cached and only executed once per
  // simulation timestep due to a Webots R2018a bug (see COZMO-16021).
  // Store them here (key is LED index, value is RGB color).
  std::map<u32, std::array<double, 3>> pendingLedColors_;

  // Parameters for setting individual LEDs on this cube
  struct LedParams
  {
    std::array<Anki::Cozmo::LightState, kNumCubeLeds> lightStates;
    
    // Keep track of where we are in the phase cycle for each LED
    std::array<TimeStamp_t, kNumCubeLeds> phases;
    
    u8 rotationPeriod = 0;
    TimeStamp_t lastRotation = 0;
    u8 rotationOffset = 0;
  };
  
  LedParams ledParams_;
  
  // To keep track of previous (and next) UpAxis
  //  sent to the robot (via UpAxisChanged msg)
  UpAxis prevUpAxis_ = UpAxis::UnknownAxis;
  UpAxis nextUpAxis_ = UpAxis::UnknownAxis;
  
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
  const u8 ledIndexLUT[NumAxes][kNumCubeLeds] =
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
  
  u32 factoryID_ = 0;
  ObjectType objectType_ = ObjectType::UnknownObject;  
  
} // private namespace

TimeStamp_t GetCurrTimestamp()
{
  const double currTime_sec = active_object_controller.getTime();
  return static_cast<TimeStamp_t>(currTime_sec * 1000.f);
}
  
template <typename T>
void SendMessageHelper(webots::Emitter* emitter, T&& msg)
{
  DEV_ASSERT(emitter != nullptr, "ActiveBlock.SendMessageHelper.NullEmitter");
  
  // Construct a LightCubeMessage union from the passed-in msg (uses LightCubeMessage's specialized rvalue constructors)
  BlockMessages::LightCubeMessage lcm(std::move(msg));
  
  // Stuff this message into a buffer and send it
  u8 buff[lcm.Size()];
  lcm.Pack(buff, lcm.Size());
  emitter->send(buff, (int) lcm.Size());
}
  
// Handle the shift by 8 bits to remove alpha channel from 32bit RGBA pixel
// to make it suitable for webots LED 24bit RGB color
inline void SetLED_helper(u32 index, u32 rgbaColor) {
  
  led_[index]->set(rgbaColor >> 8); // rgba -> 0rgb

  double red = (rgbaColor & (u32)LEDColor::LED_RED) >> (u32)LEDColorShift::LED_RED_SHIFT;
  double green = (rgbaColor & (u32)LEDColor::LED_GREEN) >> (u32)LEDColorShift::LED_GRN_SHIFT;
  double blue = (rgbaColor & (u32)LEDColor::LED_BLUE) >> (u32)LEDColorShift::LED_BLU_SHIFT;
  
  // Store the RGB value, then only send it to Webots once per time step (in Update())
  pendingLedColors_[index] = {{red, green, blue}};
}

// ========== Callbacks for messages from robot =========

void Process_cubeLights(const CubeLights& msg)
{
  const u32 currTimestamp = GetCurrTimestamp();
  
  ledParams_.lightStates = msg.lights;
  ledParams_.phases.fill(currTimestamp);
  
  ledParams_.rotationPeriod = 0;//msg.rotationPeriod_frames;
  ledParams_.lastRotation = currTimestamp;
  ledParams_.rotationOffset = 0;
}

void Process_streamObjectAccel(const StreamObjectAccel& msg)
{
  streamAccel_ = msg.enable;
}
  
// Stubs to make linking work until we clean up how simulated cubes work.
void Process_available(const ObjectAvailable& msg) {}
void Process_moved(const ObjectMoved& msg) {}
void Process_powerLevel(const ObjectPowerLevel& msg) {}
void Process_stopped(const ObjectStoppedMoving& msg) {}
void Process_tapped(const ObjectTapped& msg) {}
void Process_upAxisChanged(const ObjectUpAxisChanged& msg) {}
void Process_accel(const ObjectAccel& msg) {}
  
void ProcessBadTag_LightCubeMessage(BlockMessages::LightCubeMessage::Tag badTag)
{
  PRINT_NAMED_INFO("ActiveBlock.ProcessBadTag_LightCubeMessage",
                   "Ignoring LightCubeMessage with invalid tag: %d",
                   Util::EnumToUnderlying(badTag));
}
// ================== End of callbacks ==================


Result Init()
{
  state_ = NORMAL;

  active_object_controller.step(TIMESTEP);
  
  webots::Node* selfNode = active_object_controller.getSelf();
  
  // Get this block's object type
  webots::Field* typeField = selfNode->getField("objectType");
  if (nullptr == typeField) {
    PRINT_NAMED_ERROR("ActiveBlock.Init.NoObjectType", "Failed to find lightCubeType");
    return RESULT_FAIL;
  }
  
  // Grab ObjectType and its integer value
  const auto& typeString = typeField->getSFString();
  
  // Hack to map Victor's circle and square cube types to "1" and "2"
  if(typeString == "Block_LIGHTCUBE_CIRCLE") {
    objectType_ = ObjectType::Block_LIGHTCUBE1;
    blockID_ = 1;
  }
  else if(typeString == "Block_LIGHTCUBE_SQUARE") {
    objectType_ = ObjectType::Block_LIGHTCUBE2;
    blockID_ = 2;
  }
  else {
    objectType_ = ObjectTypeFromString(typeString);
    
    // Simply set the cube's active ID to be the light cube type's integer value
    int typeInt = -1;
    sscanf(typeString.c_str(), "Block_LIGHTCUBE%d", &typeInt);
    DEV_ASSERT(typeInt != -1, "ActiveBlock.Init.FailedToParseLightCubeInt");
    blockID_ = typeInt;
  }
  DEV_ASSERT(IsValidLightCube(objectType_, false), "ActiveBlock.Init.InvalidObjectType");

  // Generate a factory ID
  // If PROTO factoryID is > 0, use that.
  // Otherwise use blockID as factoryID.
  webots::Field* factoryIdField = selfNode->getField("factoryID");
  if (factoryIdField) {
    s32 fID = factoryIdField->getSFInt32();
    if (fID > 0) {
      factoryID_ = fID;
    } else {
      factoryID_ = blockID_;
    }
  }
  PRINT_NAMED_INFO("ActiveBlock", "Starting active object %d (factoryID %d)", blockID_, factoryID_);
  
  
  // Get all LED handles
  for (int i=0; i<kNumCubeLeds; ++i) {
    char led_name[32];
    sprintf(led_name, "led%d", i);
    led_[i] = active_object_controller.getLED(led_name);
    assert(led_[i] != nullptr);
  }
  
  // Field for monitoring color from webots tests
  ledColorField_ = selfNode->getField("ledColors");
  assert(ledColorField_ != nullptr);
  
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
  discoveryEmitter_->setChannel(kDiscoveryChannel);
  
  // Get accelerometer
  accel_ = active_object_controller.getAccelerometer("accel");
  assert(accel_ != nullptr);
  accel_->enable(TIMESTEP);
  
  wasMoving_ = false;
  wasLastMovingTime_sec_ = 0.0;
  
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
  UpAxis retVal = UpAxis::UnknownAxis;
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
  
  // Return the corresponding signed axis
  switch(whichAxis)
  {
    case X:
      retVal = (accelVals[X] < 0 ? UpAxis::XNegative : UpAxis::XPositive);
      break;
      
    case Y:
      retVal = (accelVals[Y] < 0 ? UpAxis::YNegative : UpAxis::YPositive);
      break;

    case Z:
      retVal = (accelVals[Z] < 0 ? UpAxis::ZNegative : UpAxis::ZPositive);
      break;
      
    default:
      PRINT_NAMED_ERROR("ActiveBlock.GetCurrentUpAxis.UnexpectedAxis",
                        "Unexpected whichAxis = %d",
                        whichAxis);
      break;
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
  for(u8 i=0; i<kNumCubeLeds; ++i) {
    if(shiftedLEDs & FIRST_BIT) {
      SetLED_helper(ledIndexLUT[Util::EnumToUnderlying(prevUpAxis_)][i], color);
    }
    shiftedLEDs = shiftedLEDs >> 1;
  }
}

inline void SetLED(u32 ledIndex, u32 color)
{
  SetLED_helper(ledIndexLUT[Util::EnumToUnderlying(prevUpAxis_)][ledIndex], color);
}


void SetAllLEDs(u32 color) {
  for(int i=0; i<kNumCubeLeds; ++i) {
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


void ApplyLEDParams()
{
  const TimeStamp_t currentTime = GetCurrTimestamp();
  if(ledParams_.rotationPeriod > 0 &&
     TimestampToLedFrames(currentTime - ledParams_.lastRotation) >= ledParams_.rotationPeriod)
  {
    ledParams_.lastRotation = currentTime;
    ++ledParams_.rotationOffset;
    if(ledParams_.rotationOffset >= kNumCubeLeds) {
      ledParams_.rotationOffset = 0;
    }
  }

  for(u8 i=0; i<kNumCubeLeds; ++i) {
    const u32 newColor = GetCurrentLEDcolor(ledParams_.lightStates[i],
                                            currentTime,
                                            ledParams_.phases[i],
                                            CUBE_LED_FRAME_LENGTH_MS);
    const u32 index = (i + ledParams_.rotationOffset) % kNumCubeLeds;
    SetLED_helper(index, newColor);
  }
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
      ObjectTapped msg;
      msg.objectID = blockID_;
      msg.numTaps = 1;
      msg.tapNeg = -50;  // Hard-coded tap intensity.
      msg.tapPos = 50;   // Just make sure that tapPos - tapNeg > BlockTapFilterComponent::kTapIntensityMin
      msg.tapTime = static_cast<u32>(active_object_controller.getTime() / 0.035f) % std::numeric_limits<u8>::max();  // Each tapTime count should be 35ms
      SendMessageHelper(emitter_, std::move(msg));
    }

    
    // Send ObjectAvailable message
    static u32 objAvailableSendCtr = 0;
    if (++objAvailableSendCtr == OBJECT_AVAILABLE_MESSAGE_PERIOD) {
      SendMessageHelper(discoveryEmitter_, ObjectAvailable(factoryID_,
                                                           objectType_,
                                                           0));
      objAvailableSendCtr = 0;
    }
    
    // Send accel data
    if (streamAccel_) {
      ObjectAccel msg;
      msg.objectID = blockID_;
      f32 scaleFactor = 32.f/9.81f;  // 32 on physical blocks ~= 1g
      msg.accel.x = accelVals[0] * scaleFactor;
      msg.accel.y = accelVals[1] * scaleFactor;
      msg.accel.z = accelVals[2] * scaleFactor;
      SendMessageHelper(emitter_, std::move(msg));
    }
    
    // Update lights:
    ApplyLEDParams();
    
    // Set any pending LED color fields. This must be done here since setMFVec3f can only
    // be called once per simulation time step for a given field (known Webots R2018a bug)
    // TODO: Remove?
    if (!pendingLedColors_.empty()) {
      for (const auto& newColorEntry : pendingLedColors_) {
        const auto index = newColorEntry.first;
        const auto& color = newColorEntry.second;
        ledColorField_->setMFVec3f(index, color.data());
      }
      pendingLedColors_.clear();
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
            PRINT_NAMED_INFO("ActiveBlock", "Block %d appears to be moving (UpAxis=%d).", blockID_, Util::EnumToUnderlying(prevUpAxis_));
            
            // TODO: There should really be a message ID in here, rather than just determining it from message size on the other end.
            ObjectMoved msg;
            msg.objectID = blockID_;
            msg.accel.x = accelVals[0];
            msg.accel.y = accelVals[1];
            msg.accel.z = accelVals[2];
            msg.axisOfAccel = prevUpAxis_;
            SendMessageHelper(emitter_, std::move(msg));

            wasLastMovingTime_sec_ = currTime_sec;
            wasMoving_ = true;
            
          } else if(wasMoving_ && currTime_sec - wasLastMovingTime_sec_ > STOPPED_MOVING_TIME_SEC ) {
            
            PRINT_NAMED_INFO("ActiveBlock", "Block %d stopped moving (UpAxis=%d).", blockID_, Util::EnumToUnderlying(prevUpAxis_));
            
            // Send "UpAxisChanged" message now if necessary, so that it's
            //  sent before the stopped message (to mirror what the actual
            //  cube firmware does - see 'robot/syscon/cubes/cubes.cpp')
            if (prevUpAxis_ != nextUpAxis_) {
              prevUpAxis_ = nextUpAxis_;
              ObjectUpAxisChanged msg;
              msg.objectID = blockID_;
              msg.upAxis = nextUpAxis_;
              SendMessageHelper(emitter_, std::move(msg));
            }
            
            // Send the "Stopped" message.
            ObjectStoppedMoving msg;
            msg.objectID = blockID_;
            SendMessageHelper(emitter_, std::move(msg));
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
              ObjectUpAxisChanged msg;
              msg.objectID = blockID_;
              msg.upAxis = nextUpAxis_;
              SendMessageHelper(emitter_, std::move(msg));
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
      
      default:
        PRINT_NAMED_WARNING("ActiveBlock.Update.UnknownState",
                            "Unknown state %d",
                            state_);
        break;
    }
    
  
    return RESULT_OK;
  }
  return RESULT_FAIL;
}


}  // namespace ActiveBlock
}  // namespace Cozmo
}  // namespace Anki
