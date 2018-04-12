/**
 * File: conditionCompound.cpp
 *
 * Author: ross
 * Created: 2018 Apr 10
 *
 * Description: Condition that handles boolean logic (and/or/not)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/beiConditions/conditions/conditionCompound.h"

#include "engine/aiComponent/beiConditions/beiConditionFactory.h"
#include "lib/util/source/anki/util/logging/logging.h"

namespace Anki {
namespace Cozmo {

namespace {

const char* kCompoundKey = "Compound";
const char* kAndKey = "and";
const char* kOrKey =  "or";
const char* kNotKey = "not";

struct ConditionCompoundConstructable : public ConditionCompound
{
  // this is something that can use the protected ConditionCompound ctor in a std::make_shared
};
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionCompound::ConditionCompound( const Json::Value& config )
  : IBEICondition( config )
{
  const int maxDepth = CreateNode( _root, config );
  
  // If you hit this, try as hard as you can to refactor your conditions. Maybe some of them can
  // go in the behavior as opposed to the json? Maybe some new condition types are needed?
  // The constant can be changed without detrimental effects, but if this becomes large, we will all regret it.
  // The current constant (2) allows for, e.g., AND(NOT(condition1),condition2) and nothing more
  ANKI_VERIFY( maxDepth <= 2, "ConditionCompound.Ctor.MaxDepthExceeded", "Depth=%d", maxDepth );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ConditionCompound::ConditionCompound()
  : IBEICondition( IBEICondition::GenerateBaseConditionConfig(BEIConditionType::Compound) )
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBEIConditionPtr ConditionCompound::CreateAndCondition( const std::vector<IBEIConditionPtr>& subConditions )
{
  DEV_ASSERT( !subConditions.empty(), "Cannot create an AND condition with empty subConditions" );
  IBEIConditionPtr ret = CreateSingleLevelCondition( NodeType::And, subConditions );
  return ret;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBEIConditionPtr ConditionCompound::CreateOrCondition(const std::vector<IBEIConditionPtr>& subConditions)
{
  DEV_ASSERT( !subConditions.empty(), "Cannot create an OR condition with empty subConditions" );
  IBEIConditionPtr ret = CreateSingleLevelCondition( NodeType::Or, subConditions );
  return ret;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBEIConditionPtr ConditionCompound::CreateNotCondition( IBEIConditionPtr subCondition )
{
  IBEIConditionPtr ret = CreateSingleLevelCondition( NodeType::Not, {subCondition} );
  return ret;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBEIConditionPtr ConditionCompound::CreateSingleLevelCondition( NodeType type, const std::vector<IBEIConditionPtr>& subConditions )
{
  auto ret = std::make_shared<ConditionCompoundConstructable>();
  ret->_root.reset( new Node );
  ret->_root->type = type;
  ret->_root->children.reserve( subConditions.size() );
  for( size_t idx=0; idx<subConditions.size(); ++idx ) {
    DEV_ASSERT( subConditions[idx] != nullptr, "Constructing a ConditionCompound with a null condition!" );
    ret->_root->children.emplace_back( new Node );
    ret->_root->children.back()->type = NodeType::Condition;
    ret->_root->children.back()->conditionIndex = idx;
  }
  ret->_operands = subConditions;
  return ret;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionCompound::InitInternal( BehaviorExternalInterface& bei )
{
  for( const auto& operand : _operands ) {
    operand->Init( bei );
  }
}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionCompound::AreConditionsMetInternal( BehaviorExternalInterface& bei ) const
{
  std::vector<bool> conditionValues = EvaluateConditions( bei );
  const bool result = EvaluateNode( _root, conditionValues );
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConditionCompound::SetActiveInternal( BehaviorExternalInterface& bei, bool setActive )
{
  for( const auto& operand : _operands ) {
    operand->SetActive( bei, setActive );
  }
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int ConditionCompound::CreateNode( std::unique_ptr<Node>& node, const Json::Value& config )
{
  int childrenDepth = 0;
  node.reset( new Node );
  if( config[kConditionTypeKey].isString()
      && config[kConditionTypeKey].asString() != kCompoundKey ) // not root level
  {
    node->type = NodeType::Condition;
    IBEIConditionPtr condition = BEIConditionFactory::CreateBEICondition( config, GetDebugLabel() );
    _operands.push_back( condition );
    node->conditionIndex = _operands.size()-1;
    
    _conditionNames.push_back( config[kConditionTypeKey].asString() );
    
  } else if( !config[kNotKey].isNull() ) {
    
    node->type = NodeType::Not;
    const auto& childConfig = config[kNotKey];
    ANKI_VERIFY( !childConfig.isArray(), "ConditionCompound.CreateNode.TooMany", "Too many children for a NOT condition" );
    node->children.reserve(1);
    node->children.push_back({});
    childrenDepth = 1 + CreateNode( node->children.back(), childConfig );
    
  } else {
    
    // AND and OR
    
    const Json::Value* childrenConfig = nullptr;
    if( !config[kAndKey].isNull() ) {
      node->type = NodeType::And;
      childrenConfig = &config[kAndKey];
    } else if( !config[kOrKey].isNull() ) {
      node->type = NodeType::Or;
      childrenConfig = &config[kOrKey];
    } else {
      ASSERT_NAMED( false && "unknown or missing type name", "ConditionCompound.CreateNode.Unknown");
      node->type = NodeType::Invalid;
      return childrenDepth;
    }
    ANKI_VERIFY( childrenConfig->isArray() && (childrenConfig->size() > 1),
                 "ConditionCompound.CreateNode.TooFew",
                 "Not enough children for compound condition" );
    
    node->children.reserve( childrenConfig->size() );
    for( const auto& childConfig : *childrenConfig ) {
      node->children.push_back({});
      int childDepth = 1 + CreateNode( node->children.back(), childConfig );
      childrenDepth = (childDepth > childrenDepth) ? childDepth : childrenDepth;
    }
  }
  
  return childrenDepth;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<bool> ConditionCompound::EvaluateConditions( BehaviorExternalInterface& bei ) const
{
  std::vector<bool> conditionVals;
  conditionVals.reserve( _operands.size() );
  for( const auto& operand : _operands ) {
    const bool met = operand->AreConditionsMet( bei );
    conditionVals.push_back( met );
  }
  return conditionVals;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ConditionCompound::EvaluateNode( const std::unique_ptr<Node>& node, const std::vector<bool>& conditionValues ) const
{
  bool ret = false;
  switch( node->type ) {
    case NodeType::Condition:
    {
      if( ANKI_VERIFY( node->conditionIndex < conditionValues.size(),
                       "CompoundCondition.EvaluateNode.OOB",
                       "Index %zu exceeds operands size %zu",
                       node->conditionIndex, conditionValues.size() ) )
      {
        ret = conditionValues[node->conditionIndex];
      }
    }
      break;
    case NodeType::And:
    {
      ret = !node->children.empty();
      for( const auto& child : node->children ) {
        ret &= EvaluateNode( child, conditionValues );
        if( !ret ) {
          break;
        }
      }
    }
      break;
    case NodeType::Or:
    {
      for( const auto& child : node->children ) {
        ret |= EvaluateNode( child, conditionValues );
        if( ret ) {
          break;
        }
      }
    }
      break;
    case NodeType::Not:
    {
      if( !node->children.empty() ) {
        ret = !EvaluateNode( node->children.front(), conditionValues );
      }
    }
      break;
    case NodeType::Invalid:
    {
      ASSERT_NAMED( false && "unknown type", "ConditionCompound.EvaluateNode.Unknown");
    }
      break;
  }
  return ret;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
IBEICondition::DebugFactorsList ConditionCompound::GetDebugFactors() const
{
  std::string debugStr = GetDebugString( _root );
  DebugFactorsList ret = { {"value", debugStr} };
  return ret;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string ConditionCompound::GetDebugString( const std::unique_ptr<Node>& node ) const
{
  std::string debugStr;
  if( node->type == NodeType::Condition ) {
    IBEIConditionPtr condition = _operands[node->conditionIndex];
    DebugFactorsList factors = condition->GetDebugFactors();
    debugStr += condition->GetDebugLabel();
    if( !factors.empty() ) {
      debugStr += "(";
      for( size_t idx=0; idx<factors.size(); ++idx ) {
        debugStr += factors[idx].name + ":" + factors[idx].value;
        if( idx < factors.size()-1 ) {
          debugStr += ",";
        }
      }
      debugStr += ")";
    }
  } else {
    std::string nodeType;
    if( node->type == NodeType::And ) {
      nodeType = "AND";
    } else if( node->type == NodeType::Or ) {
      nodeType = "OR";
    } else if( node->type == NodeType::Not ) {
      nodeType = "NOT";
    }
    debugStr += nodeType + "{ ";
    size_t idx=0;
    for( const auto& child : node->children ) {
      debugStr += GetDebugString( child );
      if( idx < node->children.size()-1 ) {
        debugStr += ", ";
      }
      ++idx;
    }
    debugStr += " }";
  }
    
  return debugStr;
}

}
}
