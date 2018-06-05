/**
* File: beiConditionDebugFactors.h
*
* Author: ross
* Created: May 18 2018
*
* Description: Helps with aggregating factors that influence the AreConditionsMet() method of IBEIConditions
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Engine_AiComponent_BeiConditions_BEIConditionDebugFactors_H__
#define __Engine_AiComponent_BeiConditions_BEIConditionDebugFactors_H__
#pragma once

#include "engine/aiComponent/beiConditions/iBEICondition_fwd.h"

#include "json/json-forwards.h"

#include <string>
#include <unordered_map>
#include <vector>



namespace Anki {
namespace Cozmo {
  
class BEIConditionDebugFactors
{
public:
  virtual ~BEIConditionDebugFactors() = default;
  
  // Add a key-value pair. supported types are int, float, string, char*, or bool.
  // Always call this on your variables in the same order, or it will look like things "HaveChanged()"
  template <typename T>
  void AddFactor(const std::string& name, const T& value);
  
  void AddChild(const std::string& name, const IBEIConditionPtr child);

  bool HaveChanged() const;
  
  void Reset();
  
  // turns all factors and childrens' factors into json
  Json::Value GetJSON() const;
  
private:
  
  // Helper class with way to much code to accomplish: "union { string, float, int, bool };"
  struct Data
  {
    enum Type {intType, boolType, stringType, floatType};
    Type type;
    union
    {
      int asInt;
      bool asBool;
      std::string asString;
      float asFloat;
    };
    
    explicit Data(int val);
    explicit Data(bool val);
    explicit Data(float val);
    explicit Data(const std::string& val);
    explicit Data(const char* val); // this is cast to string
    Data(const Data& other);
    Data& operator=(const Data& other);
    ~Data();
    
    void SetJSON(Json::Value& value) const;
  };
  
  static constexpr int kBEIConditionDebugFactorsHashConst = 19;
  size_t _hash = 0;
  size_t _previousHash = 0;
  std::vector<std::pair<std::string,Data>> _factors;
  std::vector<std::pair<std::string,const IBEIConditionPtr>> _children;
};
  
template <typename T>
void BEIConditionDebugFactors::AddFactor(const std::string& name, const T& value)
{
  _factors.emplace_back( name, value );
  static std::hash<std::string> stringHasher;
  _hash = _hash * kBEIConditionDebugFactorsHashConst + stringHasher(name);
  _hash = _hash * kBEIConditionDebugFactorsHashConst + std::hash<T>{}(value);
}
 
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_StateConceptStrategies_IBEICondition_H__
