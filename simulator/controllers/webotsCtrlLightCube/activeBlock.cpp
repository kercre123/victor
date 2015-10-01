/*
 * File:          activeBlock.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "activeBlock.h"
#include "BlockMessages.h"
#include "clad/types/activeObjectTypes.h"
#include "clad/robotInterface/lightCubeMessage.h"
#include "anki/cozmo/robot/ledController.h"

#include <stdio.h>
#include <assert.h>

#include <webots/Supervisor.hpp>
#include <webots/Receiver.hpp>
#include <webots/Emitter.hpp>
#include <webots/Accelerometer.hpp>
#include <webots/LED.hpp>

webots::Supervisor block_controller;


namespace Anki {
  namespace Cozmo {
    
    namespace ActiveBlock {

      namespace {

        const s32 TIMESTEP = 10; // ms
        
        // how frequently (in terms of timesteps to send messages that the block is moving)
        // (This is just to throttle movement messages a bit)
        const s32 MOVEMENT_UPDATE_FREQUENCY = 10;
        
        // once we start moving, we have to stop for this long to send StoppedMoving message
        const double STOPPED_MOVING_TIME_SEC = 0.5f;
        
        webots::Receiver* receiver_;
        webots::Emitter*  emitter_;
        
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
        const f32 RC = 1.0/(CUTOFF_FREQ*2*3.14);
        const f32 dt = 0.001*TIMESTEP;
        const f32 alpha = RC/(RC + dt);
        
        
        typedef enum {
          NORMAL = 0,
          FLASHING_ID,
          BEING_CARRIED
        } BlockState;
        
        s32 blockID_ = -1;
        
        BlockState state_ = NORMAL;
        
        webots::LED* led_[NUM_CUBE_LEDS];
        LightState ledParams_[NUM_CUBE_LEDS];
        
        UpAxis currentUpAxis_;
        
        bool wasMoving_;
        double wasLastMovingTime_sec_;
        UpAxis startingUpAxis_;
        
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
        led_[index]->set(rgbaColor>>8);
      }
      
      
      // ========== Callbacks for messages from robot =========
      void Process_flashID(const FlashObjectIDs& msg)
      {
        state_ = FLASHING_ID;
        flashIDStartTime_ = block_controller.getTime();
        //printf("Starting ID flash\n");
      }
      
      void Process_setCubeLights(const CubeLights& msg)
      {
        /*
        // See if the message is actually changing anything about the block's current
        // state. If not, don't update anything.
        bool isDifferent = false;
        for(int i=0; i<NUM_CUBE_LEDS && !isDifferent; ++i) {
          isDifferent = (ledParams_[i].onColor  != msg.lights[i].onColor  ||
                         ledParams_[i].offColor != msg.lights[i].offColor ||
                         ledParams_[i].onPeriod_ms  != msg.lights[i].onPeriod_ms ||
                         ledParams_[i].offPeriod_ms != msg.lights[i].offPeriod_ms ||
                         ledParams_[i].transitionOffPeriod_ms != msg.lights[i].transitionOffPeriod_ms ||
                         ledParams_[i].transitionOnPeriod_ms  != msg.lights[i].transitionOnPeriod_ms);
        }
        
        if(isDifferent) {
          for(int i=0; i<NUM_CUBE_LEDS; ++i) {
            ledParams_[i].onColor  = msg.lights[i].onColor;
            ledParams_[i].offColor = msg.lights[i].offColor;
            ledParams_[i].onPeriod_ms = msg.lights[i].onPeriod_ms;
            ledParams_[i].offPeriod_ms = msg.lights[i].offPeriod_ms;
            ledParams_[i].transitionOffPeriod_ms = msg.lights[i].transitionOffPeriod_ms;
            ledParams_[i].transitionOnPeriod_ms  = msg.lights[i].transitionOnPeriod_ms;
            
            ledParams_[i].nextSwitchTime = 0; // force immediate upate
            ledParams_[i].state = LED_STATE_OFF;
          }
        } else {
          printf("Ignoring SetBlockLights message with parameters identical to current state.\n");
        }
         */
        
        // Set lights immediately
        for (u32 i=0; i<NUM_CUBE_LEDS; ++i) {
          SetLED_helper(i, msg.lights[i].onColor);
        }
        
      }
      
      void Process_setObjectBeingCarried(const ObjectBeingCarried& msg)
      {
        const bool isBeingCarried = static_cast<bool>(msg.isBeingCarried);
        state_ = (isBeingCarried ? BEING_CARRIED : NORMAL);
      }
        
      void ProcessBadTag_LightCubeMessage(BlockMessages::LightCubeMessage::Tag badTag)
      {
        printf("Ignoring LightCubeMessage with invalid tag: %d\n", badTag);
      }
      // ================== End of callbacks ==================
      
      // Returns the ID of the block which is used as the
      // receiver channel for comms with the robot.
      // This is not meant to reflect how actual comms will work.
      s32 GetBlockID() {
        webots::Node* selfNode = block_controller.getSelf();
        
        webots::Field* activeIdField = selfNode->getField("ID");
        if(activeIdField) {
          return activeIdField->getSFInt32();
        } else {
          printf("Missing ID field in active block.\n");
          return -1;
        }
      }

      
      Result Init()
      {
        state_ = NORMAL;
        
        // Get this block's ID
        blockID_ = GetBlockID();
        if (blockID_ < 0) {
          printf("Failed to find blockID\n");
          return RESULT_FAIL;
        }
        printf("Starting ActiveBlock %d\n", blockID_);
        
        // Get all LED handles
        for (int i=0; i<NUM_CUBE_LEDS; ++i) {
          char led_name[32];
          sprintf(led_name, "led%d", i);
          led_[i] = block_controller.getLED(led_name);
          assert(led_[i] != nullptr);
        }
        
        // Get radio receiver
        receiver_ = block_controller.getReceiver("receiver");
        assert(receiver_ != nullptr);
        receiver_->enable(TIMESTEP);
        receiver_->setChannel(blockID_);
        
        // Get radio emitter
        emitter_ = block_controller.getEmitter("emitter");
        assert(emitter_ != nullptr);
        emitter_->setChannel(blockID_);
        
        // Get accelerometer
        accel_ = block_controller.getAccelerometer("accel");
        assert(accel_ != nullptr);
        accel_->enable(TIMESTEP);
        
        wasMoving_ = false;
        wasLastMovingTime_sec_ = 0.0;
        startingUpAxis_ = Unknown;
        
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
        UpAxis retVal = Unknown;
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
            SetLED_helper(ledIndexLUT[currentUpAxis_][i], color);
          }
          shiftedLEDs = shiftedLEDs >> 1;
        }
      }
      
      inline void SetLED(u32 ledIndex, u32 color)
      {
        SetLED_helper(ledIndexLUT[currentUpAxis_][ledIndex], color);
      }

      
      void ApplyLEDParams(TimeStamp_t currentTime)
      {
        for(int i=0; i<NUM_CUBE_LEDS; ++i)
        {
          u32 newColor;
          const bool colorUpdated = GetCurrentLEDcolor(ledParams_[i], currentTime, newColor);
          if(colorUpdated) {
            SetLED_helper(ledIndexLUT[currentUpAxis_][i], newColor);
          }
        } // for each LED
      } // ApplyLEDParams()
      
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
                printf("ActiveBlock.TapDetected: axis %d, val %f\n", axis, currAccelFiltVal);
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
        if (block_controller.step(TIMESTEP) != -1) {
          
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
          const double currTime_sec = block_controller.getTime();
          
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
            emitter_->send(msg.GetBuffer(), msg.Size());
          }

          
          
          
          // Run FSM
          switch(state_)
          {
            case NORMAL:
            {
              static s32 movingSendCtr = 0;
              
              if(movingSendCtr++ == MOVEMENT_UPDATE_FREQUENCY)
              {
                movingSendCtr = 0;
                
                // Determine if block is moving by seeing if the magnitude of the
                // acceleration vector is different from gravity
                const bool isMoving = !NEAR(accelMagSq, 9.81*9.81, 1.0);
                
                if(!isMoving) {
                  currentUpAxis_ = GetCurrentUpAxis();
                }
                
                if(isMoving) {
                  printf("Block %d appears to be moving (UpAxis=%d).\n", blockID_, currentUpAxis_);
                  
                  // TODO: There should really be a message ID in here, rather than just determining it from message size on the other end.
                  BlockMessages::LightCubeMessage msg;
                  msg.tag = BlockMessages::LightCubeMessage::Tag_moved;
                  msg.moved.objectID = blockID_;
                  msg.moved.accel.x = accelVals[0];
                  msg.moved.accel.y = accelVals[1];
                  msg.moved.accel.z = accelVals[2];
                  msg.moved.upAxis = currentUpAxis_;
                  emitter_->send(msg.GetBuffer(), msg.Size());
                  
                  if(!wasMoving_) {
                    // Store the Up axis when we first start movement
                    startingUpAxis_ = currentUpAxis_;
                  }
                  
                  wasLastMovingTime_sec_ = currTime_sec;
                  wasMoving_ = true;
                  
                } else if(wasMoving_ && currTime_sec - wasLastMovingTime_sec_ > STOPPED_MOVING_TIME_SEC ) {
                  
                  printf("Block %d stopped moving (UpAxis=%d).\n", blockID_, currentUpAxis_);
                  
                  // TODO: There should really be a message ID in here, rather than just determining it from message size on the other end.
                  BlockMessages::LightCubeMessage msg;
                  msg.tag = BlockMessages::LightCubeMessage::Tag_stopped;
                  msg.stopped.objectID = blockID_;
                  msg.stopped.upAxis   = currentUpAxis_;
                  msg.stopped.rolled   = currentUpAxis_ != startingUpAxis_; // we rolled if we have a different up axis from when we started moving
                  emitter_->send(msg.GetBuffer(), msg.Size());
                  
                  wasMoving_ = false;
                  startingUpAxis_ = Unknown;
                }
              } // if(SEND_MOVING_MESSAGES_EVERY_N_TIMESTEPS)
              
              // Apply ledParams
              //ApplyLEDParams(static_cast<TimeStamp_t>(currTime_sec*1000));
              
              break;
            }
              
            case BEING_CARRIED:
            {
              // Just apply ledParams, don't check for movement since we're being carried
              //ApplyLEDParams(static_cast<TimeStamp_t>(currTime_sec*1000));
              
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
