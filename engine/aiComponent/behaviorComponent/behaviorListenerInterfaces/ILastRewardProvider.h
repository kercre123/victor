/**
 * File: ILastRewardProvider.h
 *
 * Author: Andrew Stout
 * Created: 01/16/19
 *
 * Description: Interface for behaviors that provide a reward signal. (For Adaptive Behavior Coordinator, 2019 R&D project.)
 *
 * Copyright: Anki, Inc. 2019
 **/

#ifndef VICTOR_ILASTREWARDPROVIDER_H
#define VICTOR_ILASTREWARDPROVIDER_H

namespace Anki {
namespace Vector {

class ILastRewardProvider {
public:
  virtual ~ILastRewardProvider() {}
  virtual float GetLastReward() const = 0;
};


class RewardProvidingBehavior: public ICozmoBehavior, public ILastRewardProvider {
public:
  static constexpr float kRewardMax = 1.0f;
  static constexpr float kRewardMid = 0.5f;
  static constexpr float kRewardMin = 0.0f;

  RewardProvidingBehavior(const Json::Value& config): ICozmoBehavior(config) {}
  virtual ~RewardProvidingBehavior() {}
};

}
}

#endif //VICTOR_ILASTREWARDPROVIDER_H
