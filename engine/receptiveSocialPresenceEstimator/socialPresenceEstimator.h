/**
 * File: socialPresenceEstimator.h
 *
 * Author: Andrew Stout
 * Created: 2019-04-02
 *
 * Description: Estimates whether someone receptive to social engagement is available.
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __VICTOR_SOCIALPRESENCEESTIMATOR_H__
#define __VICTOR_SOCIALPRESENCEESTIMATOR_H__

#include "engine/aiComponent/behaviorComponent/userIntents.h"
#include "engine/components/mics/micDirectionTypes.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/robot.h"
#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

namespace Anki {
namespace Vector {

// forward declarations
class Robot;


class IDecayFunction {
public:
  IDecayFunction() {};
  virtual ~IDecayFunction() {};
  // TODO: I'm sure there's a way to make this pure virtual and have it work. Figure it out.
  virtual float operator()(float value, float dt_s) {return value;};
};

class ExponentialDecay : public IDecayFunction {
public:
  ExponentialDecay(float ratePerSec) : _ratePerSec(ratePerSec) {};
  virtual float operator()(float value, float dt_s) override;

private:
  float _ratePerSec;
};

class PowerDecay: public IDecayFunction {
public:
  PowerDecay(float power) : _power(power) {};
  virtual float operator()(float value, float dt_s) override;

private:
  float _power;
};


// class for social presence event ("evidence")
class SocialPresenceEvent {
public:
  // constructor
  SocialPresenceEvent(std::string name,
      std::shared_ptr<IDecayFunction> decayFunction,
      float independentEffect,
      float independentEffectMax,
      float reinforcementEffect,
      float reinforcementEffectMax,
      bool resetPriorOnTrigger = false);
  // destructor
  virtual ~SocialPresenceEvent();

  // accessors
  std::string GetName() const { return _name; };
  float GetValue() const { return _value; };
  float GetIndependentEffect() const { return _independentEffect; };
  float GetIndependentEffectMax() const { return _independentEffectMax; };
  float GetReinforcementEffect() const { return _reinforcementEffect; };
  float GetReinforcementEffectMax() const { return _reinforcementEffectMax; };
  bool GetReset() const { return _resetPriorOnTrigger; };

  // methods
  void Update(float dt_s);
  void Trigger(float& rspi);
  void Reset() { _value = 0.0; };

private:
  // private member vars
  float _value;
  std::string _name;
  // decay function
  std::shared_ptr<IDecayFunction> _decay;
  // independent effect - the effect if the RSPI starts below the...
  float _independentEffect;
  // independent effect maximum - the highest this event can increase the RSPI to, given that it started below
  float _independentEffectMax;
  // reinforcement effect - the effect if the RSPI starts above the independent effect maximum
  float _reinforcementEffect;
  // reinforcement effect maximum - the highest this event can increase the RSPI to
  float _reinforcementEffectMax;
  // should all prior events be reset on trigger
  bool _resetPriorOnTrigger;
};


class SocialPresenceEstimator : public IDependencyManagedComponent<RobotComponentID>,
                                private Util::noncopyable
{
public:
  explicit SocialPresenceEstimator();
  ~SocialPresenceEstimator();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void GetInitDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::AIComponent);
    dependencies.insert(RobotComponentID::MicComponent);
  };
  virtual void InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps) override;
  //virtual void AdditionalInitAccessibleComponents(RobotCompIDSet& components) const override {}
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
    dependencies.insert(RobotComponentID::TouchSensor);
  }
  virtual void AdditionalUpdateAccessibleComponents(RobotCompIDSet& components) const override {

  }
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;


  // ******** Input Event Handlers ********

  void OnNewUserIntent(const UserIntentTag tag);
  void OnRobotObservedFace(const AnkiEvent<ExternalInterface::MessageEngineToGame>& msg);
  void OnRobotObservedMotion(const AnkiEvent<ExternalInterface::MessageEngineToGame>& msg);
  void OnRobotObservedSalientPoint(const AnkiEvent<ExternalInterface::MessageEngineToGame>& msg);
  bool OnMicPowerSample(double micPowerLevel, MicDirectionConfidence conf, MicDirectionIndex dir);


private:
  // -------------------------- Private Member Funcs ---------------------------
  void SubscribeToWebViz();
  void SendDataToWebViz(const CozmoContext* context);
  void UpdateInputs(const RobotCompMap& dependentComps);
  void UpdateRSPI();
  void TriggerInputEvent(SocialPresenceEvent* inputEvent);
  void LogInputEvent(SocialPresenceEvent* inputEvent);

  void PollTouch(const TouchSensorComponent& touchSensorComponent);



  // -------------------------- Private Member Vars ----------------------------
  Robot* _robot = nullptr;
  std::vector<Signal::SmartHandle> _signalHandles;
  float _lastWebVizSendTime_s = 0.0f;
  float _lastInputEventsUpdateTime_s = 0.0f;
  float _rspi;

  // input events
  // singleton input events are created once per input type and re-triggered whenever we get that input type
  // name decay, independent effect, independent effect max, reinforcement effect, reinforcement effect max, reset
  SocialPresenceEvent _SPEUserIntent =
      SocialPresenceEvent("UserIntent", std::make_shared<ExponentialDecay>(0.1f), 1.0f, 1.0f, 1.0f, 1.0f, true);
  SocialPresenceEvent _SPEFace =
      SocialPresenceEvent("Face", std::make_shared<ExponentialDecay>(0.1), 0.8f, 1.0f, 0.8f, 1.0f, false);
  SocialPresenceEvent _SPEMotion =
      SocialPresenceEvent("Motion", std::make_shared<ExponentialDecay>(0.2), 0.2f, 0.5f, 0.3f, 0.8f, false);
  SocialPresenceEvent _SPEHand =
      SocialPresenceEvent("Hand", std::make_shared<ExponentialDecay>(0.2f), 0.3f, 0.5f, 0.35f, 0.9f, false);
  SocialPresenceEvent _SPEPerson =
      SocialPresenceEvent("Person", std::make_shared<ExponentialDecay>(0.1f), 0.6f, 0.75f, 0.6f, 1.0f, false);
  SocialPresenceEvent _SPESleep =
      SocialPresenceEvent("Sleep", std::make_shared<PowerDecay>(1.1f), -0.9f, 0.0f, -0.9f, 0.0f, true);
  SocialPresenceEvent _SPEQuiet =
      SocialPresenceEvent("Quiet", std::make_shared<PowerDecay>(1.2f), -0.9f, 0.0f, -0.9f, 0.0f, true);
  SocialPresenceEvent _SPEShutUp =
      SocialPresenceEvent("ShutUp", std::make_shared<PowerDecay>(1.15f), -0.9f, 0.0f, -0.9f, 0.0f, true);
  SocialPresenceEvent _SPETouch =
      SocialPresenceEvent("Touch", std::make_shared<ExponentialDecay>(0.2f), 0.8f, 1.0f, 0.8f, 1.0f, false);
  SocialPresenceEvent _SPESound =
      SocialPresenceEvent("Sound", std::make_shared<ExponentialDecay>(0.2f), 0.2f, 0.5f, 0.3f, 0.8f, false);
  // these ones are temporary for testing, obviously
  SocialPresenceEvent _spete1 = SocialPresenceEvent("ExplicitPositive", std::make_shared<ExponentialDecay>(0.1f), 1.0f, 1.0f, 1.0f, 1.0f, true);
  SocialPresenceEvent _spete2 = SocialPresenceEvent("ImplicitPositive", std::make_shared<ExponentialDecay>(0.3f), 0.5f, 1.0f, 0.5f, 1.0f);
  SocialPresenceEvent _spete3 = SocialPresenceEvent("ExplicitInhibitor", std::make_shared<ExponentialDecay>(0.1f), -1.0f, 0, -1.0, 0, true);
  SocialPresenceEvent _spete4 = SocialPresenceEvent("PowerDecayNegative", std::make_shared<PowerDecay>(1.2f), -1.0f, 0.0f, -1.0f, 0.0f, true);
  SocialPresenceEvent _spete5 = SocialPresenceEvent("PowerDecayPositive", std::make_shared<PowerDecay>(1.2f), 1.0f, 1.0f, 1.0f, 1.0f, true);

  // if needed, we can also have dynamic input events, where a new event instance is created for each triggering
  // event, and then culled once it's value drops to zero.
  // Just make sure the rest of the mechanism works either way.

  std::vector<SocialPresenceEvent*> _inputEvents = {
      &_SPEUserIntent,
      &_SPEFace,
      &_SPEMotion,
      &_SPEHand,
      &_SPEPerson,
      &_SPESleep,
      &_SPEQuiet,
      &_SPEShutUp,
      &_SPETouch,
      &_SPESound,
      &_spete1,
      &_spete2,
      &_spete3,
      &_spete4,
      &_spete5
  };

  // subscription handles
  uint32_t _newUserIntentHandle;
  Signal::SmartHandle _faceHandle;
  Signal::SmartHandle _motionHandle;
  Signal::SmartHandle _salientHandle;
  uint32_t _micPowerSampleHandle;

};

}
}




#endif // __VICTOR_SOCIALPRESENCEESTIMATOR_H__
