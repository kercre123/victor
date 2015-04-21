/*
 * File:          activeBlock.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "activeBlock.h"
#include "BlockMessages.h"
#include "anki/cozmo/shared/activeBlockTypes.h"
#include "anki/cozmo/shared/ledTypes.h"
#include "anki/cozmo/robot/ledController.h"
#include <stdio.h>
#include <assert.h>

#include <webots/Supervisor.hpp>

#define TIMESTEP 10  //ms

webots::Supervisor block_controller;


namespace Anki {
  namespace Cozmo {
    namespace ActiveBlock {

      namespace {
      
        webots::Receiver* receiver_;
        webots::Emitter*  emitter_;
        
        webots::Accelerometer* accel_;
        
        typedef enum {
          NORMAL = 0,
          FLASHING_ID
        } BlockState;
        
        s32 blockID_ = -1;
        
        BlockState state_ = NORMAL;
        
        webots::LED* led_[NUM_BLOCK_LEDS];
        LEDParams_t ledParams_[NUM_BLOCK_LEDS];
        
        typedef enum {
          UNKNOWN = -1,
          Xneg = 0,
          Xpos,
          Yneg,
          Ypos,
          Zneg,
          Zpos,
          NUM_UP_AXES
        } UpAxis;
        
        UpAxis currentUpAxis_;
        
        // Lookup table for which four LEDs are on top, given the current up axis
        // (in the order upper left, upper right, lower left, lower right)
        const u8 ledIndexLUT[NUM_UP_AXES][NUM_BLOCK_LEDS] =
        {
          // Xneg (Front Face on top)
          {2, 3, 6, 7, 0, 1, 4, 5},
          
          // Xpos (Back Face on top)
          {4, 5, 0, 1, 6, 7, 2, 3},
          
          // Yneg (Right on top)
          {1, 3, 0, 2, 5, 7, 4, 6},
          
          // Ypos (Left Face on top)
          {2, 0, 3, 1, 6, 4, 7, 5},
          
          // Zneg (Bottom Face on top) -- NOTE: Flipped 180deg around X axis!!
          {3, 2, 1, 0, 7, 6, 5, 4},
          
          // Zpos (Top Face on top)
          {0, 1, 2, 3, 4, 5, 6, 7}
        };
        
        // Flash ID params
        double flashIDStartTime_ = 0;
        
        // For now, flashing ID means turning the LED off for a bit, then red for a bit,
        // then off for a bit, then resuming the regular pattern
        const double flashID_t1_off = 0.025;
        const double flashID_t2_on = 0.1;
        const double flashID_t3_off = 0.025;
        
        
      } // private namespace

      
      // ========== Callbacks for messages from robot =========
      void ProcessFlashIDMessage(const BlockMessages::FlashID& msg)
      {
        state_ = FLASHING_ID;
        flashIDStartTime_ = block_controller.getTime();
        //printf("Starting ID flash\n");
      }
      
      void ProcessSetBlockLightsMessage(const BlockMessages::SetBlockLights& msg)
      {
        for(int i=0; i<NUM_BLOCK_LEDS; ++i) {
          ledParams_[i].onColor  = msg.onColor[i];
          ledParams_[i].offColor = msg.offColor[i];
          ledParams_[i].onPeriod_ms = msg.onPeriod_ms[i];
          ledParams_[i].offPeriod_ms = msg.offPeriod_ms[i];
          ledParams_[i].transitionOffPeriod_ms = msg.transitionOffPeriod_ms[i];
          ledParams_[i].transitionOnPeriod_ms  = msg.transitionOnPeriod_ms[i];
          
          ledParams_[i].nextSwitchTime = 0; // force immediate upate
          ledParams_[i].state = LEDState_t::LED_STATE_OFF;
        }
      }
      
      // ================== End of callbacks ==================
      
      // Returns the ID of the block which is used as the
      // receiver channel for comms with the robot.
      // This is not meant to reflect how actual comms will work.
      s32 GetBlockID() {
        webots::Node* selfNode = block_controller.getSelf();
        
        webots::Field* activeIdField = selfNode->getField("activeID");
        if(activeIdField) {
          return activeIdField->getSFInt32();
        } else {
          printf("Missing activeID field in active block.\n");
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
        for (int i=0; i<NUM_BLOCK_LEDS; ++i) {
          char led_name[NUM_BLOCK_LEDS];
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
        
        
        // Register callbacks
        RegisterCallbackForMessageFlashID(ProcessFlashIDMessage);
        RegisterCallbackForMessageSetBlockLights(ProcessSetBlockLightsMessage);
        
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
      
      
      UpAxis GetCurrentUpAxis(bool& isMoving)
      {
#       define X 0
#       define Y 1
#       define Z 2
        const double* accelVals = accel_->getValues();

        // Determine the axis with the largest absolute acceleration
        UpAxis retVal = UNKNOWN;
        double maxAbsAccel = -1;
        double absAccel[3];

        s32 whichAxis = -1;
        for(s32 i=0; i<3; ++i) {
          absAccel[i] = abs(accelVals[i]);
          if(absAccel[i] > maxAbsAccel) {
            whichAxis = i;
            maxAbsAccel = absAccel[i];
          }
        }

        static const f32 movementAccelThresh = 1.f;
//        printf("Block %d acceleration: (%.3f, %.3f, %.3f)\n", blockID_,
//               accelVals[X], accelVals[Y], accelVals[Z]);
        
        // Return the corresponding signed axis
        switch(whichAxis)
        {
          case X:
            retVal = (accelVals[X] < 0 ? Xneg : Xpos);
            isMoving = (absAccel[X] > 9.81f + movementAccelThresh ||
                        absAccel[X] < 9.81f - movementAccelThresh ||
                        absAccel[Y] > movementAccelThresh ||
                        absAccel[Z] > movementAccelThresh);
            break;
          case Y:
            retVal = (accelVals[Y] < 0 ? Yneg : Ypos);
            isMoving = (absAccel[Y] > 9.81f + movementAccelThresh ||
                        absAccel[Y] < 9.81f - movementAccelThresh ||
                        absAccel[X] > movementAccelThresh ||
                        absAccel[Z] > movementAccelThresh);
            break;
          case Z:
            retVal = (accelVals[Z] < 0 ? Zneg : Zpos);
            isMoving = (absAccel[Z] > 9.81f + movementAccelThresh ||
                        absAccel[Z] < 9.81f - movementAccelThresh ||
                        absAccel[Y] > movementAccelThresh ||
                        absAccel[X] > movementAccelThresh);
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
      
      // Handle the shift by 8 bits to remove alpha channel from 32bit RGBA pixel
      // to make it suitable for webots LED 24bit RGB color
      inline void SetLED_helper(u32 index, u32 rgbaColor) {
        led_[index]->set(rgbaColor>>8);
      }
      
      void SetLED(WhichBlockLEDs whichLEDs, u32 color)
      {
        static const u8 FIRST_BIT = 0x01;
        u8 shiftedLEDs = static_cast<u8>(whichLEDs);
        for(u8 i=0; i<NUM_BLOCK_LEDS; ++i) {
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
        for(int i=0; i<NUM_BLOCK_LEDS; ++i)
        {
          u32 newColor;
          const bool colorUpdated = GetCurrentLEDcolor(ledParams_[i], currentTime, newColor);
          if(colorUpdated) {
            SetLED_helper(ledIndexLUT[currentUpAxis_][i], newColor);
          }
        } // for each LED
      } // ApplyLEDParams()
      
      void SetAllLEDs(u32 color) {
        for(int i=0; i<NUM_BLOCK_LEDS; ++i) {
          SetLED_helper(i, color);
        }
      }
      
      Result Update() {
        if (block_controller.step(TIMESTEP) != -1) {
          
#if(0)
          // Test: Blink all LEDs in order
          static u32 p=0;
          //static BlockLEDPosition prevIdx = TopFrontLeft, idx = TopFrontLeft;
          static WhichBlockLEDs prevIdx = WhichBlockLEDs::TOP_UPPER_LEFT, idx = WhichBlockLEDs::TOP_UPPER_LEFT;
          if (p++ == 100) {
            SetLED(prevIdx, 0);
            if (idx == WhichBlockLEDs::TOP_UPPER_LEFT) {
              // Top upper left is green
              SetLED(idx, 0x00ff0000);
            } else {
              // All other LEDs are red
              SetLED(idx, 0xff000000);
            }
            prevIdx = idx;

            // Increment LED position index
            idx = static_cast<WhichBlockLEDs>(static_cast<u8>(idx)<<1);
            if(idx == WhichBlockLEDs::NONE) {
              idx = WhichBlockLEDs::TOP_UPPER_LEFT;
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
          
          double currTime = block_controller.getTime();
          
          bool isMoving = false;
          currentUpAxis_ = GetCurrentUpAxis(isMoving);
          
          if(isMoving) {
            printf("Block %d appears to have moved.\n", blockID_);
            
            BlockMessages::BlockMoved msg;
            msg.blockID = blockID_;
            const double* accelVals = accel_->getValues();
            msg.xAccel = accelVals[0];
            msg.yAccel = accelVals[1];
            msg.zAccel = accelVals[2];
            
            emitter_->send(&msg, BlockMessages::GetSize(BlockMessages::BlockMoved_ID));
          }
          
          // Run FSM
          switch(state_) {
            case NORMAL:
              // Apply ledParams
              ApplyLEDParams(static_cast<TimeStamp_t>(currTime*1000));
              break;
            case FLASHING_ID:
              if (currTime >= flashIDStartTime_ + flashID_t1_off + flashID_t2_on + flashID_t3_off) {
                state_ = NORMAL;
              } else if (currTime >= flashIDStartTime_ + flashID_t1_off + flashID_t2_on) {
                SetAllLEDs(0);
              } else if (currTime >= flashIDStartTime_ + flashID_t1_off) {
                SetAllLEDs(0xff0000);
              } else if (currTime >= flashIDStartTime_) {
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