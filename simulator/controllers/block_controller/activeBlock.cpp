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

webots::Robot block_controller;


namespace Anki {
  namespace Cozmo {
    namespace ActiveBlock {

      namespace {
        static const int NUM_LEDS = 8;
        webots::LED* led_[NUM_LEDS];
        
        webots::Receiver* receiver_;
        webots::Accelerometer* accel_;
        
      } // private namespace

      
      // ========== Callbacks for messages from robot =========
      void ProcessSetBlockLightsMessage(const BlockMessages::SetBlockLights& msg)
      {
        for(int i=0; i<NUM_LEDS; ++i) {
          led_[i]->set(msg.color[i]);
        }
      }
      
      // ================== End of callbacks ==================
      
      
      void Init()
      {
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
        
        // Get accelerometer
        accel_ = block_controller.getAccelerometer("accel");
        assert(accel_ != nullptr);
        accel_->enable(TIMESTEP);
        
        
        // Register callbacks
        RegisterCallbackForMessageSetBlockLights(ProcessSetBlockLightsMessage);
        
      }

      void DeInit() {
        if (receiver_) {
          receiver_->disable();
        }
        if (accel_) {
          accel_->disable();
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
          
          /*
          static int p =0;
          static bool red = true;
          if (p++ > 100) {
            p = 0;
            //printf("BLOCK LED: %d\n", red);
            if (red) {
              led_[0]->set(0xff0000);
            } else {
              led_[0]->set(0x00ff00);
            }
            red = !red;
          }
           */
        
          return RESULT_OK;
        }
        return RESULT_FAIL;
      }


    }  // namespace ActiveBlock
  }  // namespace Cozmo
}  // namespace Anki