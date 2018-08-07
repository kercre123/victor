/**
 * File: conditionCompound.h
 *
 * Author: ross
 * Created: 2018 Apr 10
 *
 * Description: Condition that handles boolean logic (and/or/not)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Engine_AiComponent_BeiConditions_Conditions_ConditionCompound_H__
#define __Engine_AiComponent_BeiConditions_Conditions_ConditionCompound_H__

#include "engine/aiComponent/beiConditions/iBEICondition.h"

namespace Anki {
namespace Vector {
  
class UnitTestKey;

class ConditionCompound : public IBEICondition
{
public:
  explicit ConditionCompound(const Json::Value& config);
  
  // static methods to construct a boolean condition from subConditions
  static IBEIConditionPtr CreateAndCondition(const std::vector<IBEIConditionPtr>& subConditions, const std::string& ownerDebugLabel);
  static IBEIConditionPtr CreateOrCondition (const std::vector<IBEIConditionPtr>& subConditions, const std::string& ownerDebugLabel);
  static IBEIConditionPtr CreateNotCondition(IBEIConditionPtr subCondition, const std::string& ownerDebugLabel);

  virtual void InitInternal(BehaviorExternalInterface& behaviorExternalInterface) override;
  virtual bool AreConditionsMetInternal(BehaviorExternalInterface& behaviorExternalInterface) const override;
  virtual void SetActiveInternal(BehaviorExternalInterface& behaviorExternalInterface, bool setActive) override;
  virtual void GetRequiredVisionModes(std::set<VisionModeRequest>& requests) const override;
  
  virtual void BuildDebugFactorsInternal( BEIConditionDebugFactors& factors ) const override;
  
protected:
  explicit ConditionCompound( const std::string& ownerDebugLabel ); // for use by static methods
  
private:
  enum class NodeType : uint8_t {
    Invalid=0,
    And,
    Or,
    Not,
    Condition, // leaf node
  };
  
  struct Node {
    NodeType type = NodeType::Invalid;
    std::vector<std::unique_ptr<Node>> children;
    size_t conditionIndex = -1; // if a leaf, the index in _operands
  };
  
  // helper for the public static construction methods
  static IBEIConditionPtr CreateSingleLevelCondition( NodeType type,
                                                      const std::vector<IBEIConditionPtr>& subConditions,
                                                      const std::string& ownerDebugLabel );
  
  // recursively initializes node with config, returns the depth
  int CreateNode( std::unique_ptr<Node>& node, const Json::Value& config, const bool isRoot );
  
  // recursively evaluates the tree and returns the result given conditionValues
  bool EvaluateNode( const std::unique_ptr<Node>& node, const std::vector<bool>& conditionValues ) const;
  
  // returns all _operands' AreConditionsMet()
  std::vector<bool> EvaluateConditions( BehaviorExternalInterface& bei ) const;
  
  // recursively generates a debug string starting at node, consisting of the boolean operations and the _operands' names
  std::string GetDebugString( const std::unique_ptr<Node>& node ) const;
  
  std::vector<IBEIConditionPtr> _operands;
  std::unique_ptr<Node> _root;
  
  std::vector<std::string> _conditionNames; // for debugging, matches _operands
  
public:
  std::vector<IBEIConditionPtr> TESTONLY_GetOperands(UnitTestKey key) const;
};

}
}


#endif
