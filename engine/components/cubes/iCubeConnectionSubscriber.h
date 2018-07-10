/**
* File: cubeConnectionSubscriber.cpp
*
* Author: Sam Russell 
* Created: 6/20/18
*
* Description: Interface class for use by the CubeConnectionCoordinator for observer-pattern style
*              tracking of subscribers. Classes which use the CubeConnectionCoordinator to subscribe
*              to cube connections should implement this interface
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_AiComponent_BehaviorComponent_iCubeConnectionSubscriber_H__
#define __Engine_AiComponent_BehaviorComponent_iCubeConnectionSubscriber_H__

#include "util/logging/logging.h"

#include <string>

namespace Anki{
namespace Cozmo{

class ICubeConnectionSubscriber
{
public:
  virtual ~ICubeConnectionSubscriber(){};

  // We must insist... if you want cube connections, provide a debug name
  virtual std::string GetCubeConnectionDebugName() const = 0;

  // Called when:
  // 1. A new INTERACTABLE connection is made successfully when there are non-background subscribers
  // 2. An existing BACKGROUND connection finishes converting to INTERACTABLE
  virtual void ConnectedInteractableCallback() {}

  // Called when:
  // 1. A new BACKGROUND connection is made successfully when there are only BACKGROUND subscribers
  // 2. An existing INTERACTABLE connection finishes converting to BACKGROUND
  virtual void ConnectedBackgroundCallback() {}

  // Called when a connection attempt, either BACKGROUND -or- INTERACTABLE fails
  virtual void ConnectionFailedCallback() {}

  // Subscribers must implement this function for visibility, even if the implementation is empty.
  // The cubeConnectionCoordinator will notify subscribers then dump all subscriptions in the 
  // event that the connection drops unexpectedly. This prevents preservation of invalid state,
  // but means subscribers must re-subscribe to trigger a new connection attempt.
  virtual void ConnectionLostCallback() = 0;

protected:
  ICubeConnectionSubscriber() {}
  //enforce class as abstract
};

#if ANKI_DEV_CHEATS
class TestCubeConnectionSubscriber : public ICubeConnectionSubscriber
{
public:
  TestCubeConnectionSubscriber(int id) : _id(id) {}

  virtual std::string GetCubeConnectionDebugName() const override {return ("TestSubscriber" + std::to_string(_id));};
  virtual void ConnectedInteractableCallback() override {PRINT_NAMED_INFO("CubeConnectionCoordinatorTest.ConnectedInteractable",
                                                                          "Connection succeeded callback received");}
  virtual void ConnectedBackgroundCallback() override {PRINT_NAMED_INFO("CubeConnectionCoordinatorTest.ConnectedBackground",
                                                                        "Connection succeeded callback received");}
  virtual void ConnectionFailedCallback() override {PRINT_NAMED_INFO("CubeConnectionCoordinatorTest.ConnectionFailed",
                                                                     "Connection failed callback received");}
  virtual void ConnectionLostCallback() override {PRINT_NAMED_INFO("CubeConnectionCoordinatorTest.ConnectionLost",
                                                                   "Connection lost callback received");}
private:
  int _id;
};

#endif // ANKI_DEV_CHEATS

} // namespace Cozmo
} // namespace Anki

#endif //__Engine_AiComponent_BehaviorComponent_iCubeConnectionSubscriber_H__
