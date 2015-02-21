/*
 * File:          activeBlock.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "activeBlock.h"
#include "BlockMessages.h"
#include <stdio.h>
#include <assert.h>

#include <webots/Supervisor.hpp>

#define TIMESTEP 10  //ms

webots::Supervisor block_controller;


namespace Anki {
  namespace Cozmo {
    namespace ActiveBlock {

      namespace {
        static const int NUM_LEDS = 8;
        webots::LED* led_[NUM_LEDS];
        
        webots::Receiver* receiver_;
        webots::Accelerometer* accel_;
        
        typedef enum {
          NORMAL = 0,
          FLASHING_ID
        } BlockState;
        
        BlockState state_ = NORMAL;
        
        // TODO: This will expand as we want the lights to do fancier things
        typedef struct {
          u32 color;
        } LEDParams;
        
        LEDParams ledParams_[NUM_LEDS];
        
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
        for(int i=0; i<NUM_LEDS; ++i) {
          ledParams_[i].color = msg.color[i];
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
        for (int i=0; i<NUM_LEDS; ++i) {
          char led_name[NUM_LEDS];
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

      void SetAllLEDs(u32 color) {
        for(int i=0; i<NUM_LEDS; ++i) {
          led_[i]->set(color);
        }
      }
      
      
      Result Update() {
        if (block_controller.step(TIMESTEP) != -1) {
          
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
              for(int i=0; i<NUM_LEDS; ++i) {
                SetAllLEDs(ledParams_[i].color);
              }
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