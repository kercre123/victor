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
  PowerDecay(float power/*, float duration*/) : _power(power)/*, _duration(duration)*/ {};
  virtual float operator()(float value, float dt_s) override;

private:
  float _power;
  //float _duration;
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
  virtual void InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps) override;
  //virtual void AdditionalInitAccessibleComponents(RobotCompIDSet& components) const override {}
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContextWrapper);
  }
  //virtual void AdditionalUpdateAccessibleComponents(RobotCompIDSet& components) const override {}

  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;

private:
  // -------------------------- Private Member Funcs ---------------------------
  void SubscribeToWebViz();
  void SendDataToWebViz(const CozmoContext* context);
  void UpdateRSPI();
  void TriggerInputEvent(SocialPresenceEvent& inputEvent);

  // -------------------------- Private Member Vars ----------------------------
  Robot* _robot = nullptr;
  std::vector<Signal::SmartHandle> _signalHandles;
  float _lastWebVizSendTime_s = 0.0f;
  float _lastInputEventsUpdateTime_s = 0.0f;
  float _rspi;

  // singleton input events are created once per input type and re-triggered whenever we get that input type
  std::vector<SocialPresenceEvent> _inputEvents;
  // if needed, we can also have dynamic input events, where a new event instance is created for each triggering
  // event, and then culled once it's value drops to zero.
  // Just make sure the rest of the mechanism works either way.
};

}
}




#endif // __VICTOR_SOCIALPRESENCEESTIMATOR_H__
