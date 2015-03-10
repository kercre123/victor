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
        webots::Accelerometer* accel_;
        
        typedef enum {
          NORMAL = 0,
          FLASHING_ID
        } BlockState;
        
        BlockState state_ = NORMAL;
        
        webots::LED* led_[NUM_BLOCK_LEDS];
        LEDParams ledParams_[NUM_BLOCK_LEDS];
        
        // Top-Front face pairs
        typedef enum {
          ZX = 0,
          Xz,
          zx,
          xZ,
          Yx,
          yX,
          NUM_TOP_FRONT_FACE_PAIRS
        } TopFrontFacePair;
        
        const u8 ledPositionToIdx_[NUM_TOP_FRONT_FACE_PAIRS][NUM_BLOCK_LEDS] =
        {
          // ZX
          { 6, 7, 5, 4, 2, 3, 1, 0 },
          
          // Xz
          { 2, 3, 6, 7, 1, 0, 5, 4},
          
          // zx
          { 1, 0, 2, 3, 5, 4, 6, 7},
          
          // xZ
          { 5, 4, 1, 0, 6, 7, 2, 3},
          
          // Yx
          { 2, 6, 1, 5, 3, 7, 0, 4},
          
          // yX
          { 7, 3, 4, 0, 6, 2, 5, 1}
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
          ledParams_[i].color = msg.color[i];
          ledParams_[i].onPeriod_ms = msg.onPeriod_ms[i];
          ledParams_[i].offPeriod_ms = msg.offPeriod_ms[i];
        }
      }
      
      // ================== End of callbacks ==================
      
      // Returns the ID of the block which is used as the
      // receiver channel for comms with the robot.
      // This is not meant to reflect how actual comms will work.
      s32 GetBlockID() {
        webots::Node* selfNode = block_controller.getSelf();
        
        // Get world root node
        webots::Node* root = block_controller.getRoot();
        
        // Look for the index of selfNode within the root's children
        webots::Field* rootChildren = root->getField("children");
        int numRootChildren = rootChildren->getCount();
        
        for (s32 n = 0 ; n<numRootChildren; ++n) {
          webots::Node* nd = rootChildren->getMFNode(n);
          
          if (nd == selfNode) {
            return n;
          }
        }
        
        return -1;
      }

      
      Result Init()
      {
        state_ = NORMAL;
        
        // Get this block's ID
        s32 blockID = GetBlockID();
        if (blockID < 0) {
          printf("Failed to find blockID\n");
          return RESULT_FAIL;
        }
        printf("Starting ActiveBlock %d\n", blockID);
        
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
        receiver_->setChannel(blockID);
        
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
      
      void ApplyLEDParams(TimeStamp_t currentTime) {
        for(int i=0; i<NUM_BLOCK_LEDS; ++i) {
          if(currentTime > ledParams_[i].nextSwitchTime) {

            const u8 ledIndex = ledPositionToIdx_[GetOrientation()][i];
            
            if(ledParams_[i].isOn) {
              // Time to turn off
              led_[ledIndex]->set(0x0);
              ledParams_[i].nextSwitchTime = currentTime + ledParams_[i].offPeriod_ms;
              ledParams_[i].isOn = false;
            } else {
              // Time to turn on
              led_[ledIndex]->set(ledParams_[i].color);
              ledParams_[i].nextSwitchTime = currentTime + ledParams_[i].onPeriod_ms;
              ledParams_[i].isOn = true;
            }
          }
        } // for each LED
      } // ApplyLEDParams()
      
      void SetLED(BlockLEDPosition p, u32 color) {
        led_[ ledPositionToIdx_[GetOrientation()][p] ]->set(color);
      }
      
      void SetAllLEDs(u32 color) {
        for(int i=0; i<NUM_BLOCK_LEDS; ++i) {
          led_[i]->set(color);
        }
      }
      
      
      Result Update() {
        if (block_controller.step(TIMESTEP) != -1) {
          
#if(0)
          // Test: Blink all LEDs in order
          static u32 p=0;
          static BlockLEDPosition prevIdx = TopFrontLeft, idx = TopFrontLeft;
          if (p++ == 100) {
            SetLED(prevIdx, 0);
            if (idx == TopFrontLeft) {
              // TopFrontLeft is green
              SetLED(idx, 0x00ff00);
            } else {
              // All other LEDs are red
              SetLED(idx, 0xff0000);
            }
            prevIdx = idx;

            // Increment LED position index
            if (idx == BottomBackRight) {
              idx = TopFrontLeft;
            } else {
              idx = (BlockLEDPosition)(idx + 1);
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