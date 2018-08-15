/**
 * File: hasSettableParameters_impl.h
 *
 * Authors: Andrew Stein
 * Created: 2015-10-06
 *
 * Description:
 *
 *   Implements the members of the templated HasSettableParameters class.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Anki_Cozmo_Util_HasSettableParameters_Impl_H__
#define __Anki_Cozmo_Util_HasSettableParameters_Impl_H__

#include "engine/utils/hasSettableParameters.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "engine/externalInterface/externalInterface.h"

namespace Anki {
namespace Vector {
 
  using GameToEngTag = ExternalInterface::MessageGameToEngineTag;
  
  template<typename Param_t, GameToEngTag SetMsgTag, typename Value_t>
  HasSettableParameters<Param_t,SetMsgTag,Value_t>::HasSettableParameters(IExternalInterface* externalInterface)
  {
    if(nullptr != externalInterface) {
      auto setParamCallback = [this](const AnkiEvent<ExternalInterface::MessageGameToEngine>& event) {
        this->HandleSetParameters(event);
      };
      
      _eventHandler = externalInterface->Subscribe(SetMsgTag, setParamCallback);
    }
    
    // Make sure there's an entry for all enum values in Param_t, and initialize to zero.
    // NOTE: this assumes there is a NumParameters value!
    for(size_t iParam=0; iParam < static_cast<size_t>(Param_t::NumParameters); ++iParam) {
      _params[static_cast<Param_t>(iParam)] = Value_t{0};
    }
  }
  
  template<typename Param_t, GameToEngTag SetMsgTag, typename Value_t>
  template<typename T>
  T HasSettableParameters<Param_t,SetMsgTag,Value_t>::GetParam(Param_t whichParam)
  {
    if(!_isInitialized) {
      _isInitialized = true;
      SetDefaultParams();
    }
    
    return static_cast<T>(_params[whichParam]);
  }
  
  template<typename Param_t, GameToEngTag SetMsgTag, typename Value_t>
  const typename HasSettableParameters<Param_t,SetMsgTag,Value_t>::ParamContainer& HasSettableParameters<Param_t,SetMsgTag,Value_t>::GetAllParams()
  {
    if(!_isInitialized) {
      _isInitialized = true; // Before SetDefaultParams to avoid infinite loop!
      SetDefaultParams();
    }
    
    return _params;
  }
  
  template<typename Param_t, GameToEngTag SetMsgTag, typename Value_t>
  void HasSettableParameters<Param_t,SetMsgTag,Value_t>::SetAllParams(const ParamContainer& allParams)
  {
    _params = allParams;
    _isInitialized = true;
  }
  
  template<typename Param_t, GameToEngTag SetMsgTag, typename Value_t>
  void HasSettableParameters<Param_t,SetMsgTag,Value_t>::SetParam(Param_t whichParam,
                                                                 Value_t newValue)
  {
    if(!_isInitialized) {
      _isInitialized = true; // Before SetDefaultParams to avoid infinite loop!
      SetDefaultParams();
    }
    
    const Range& range = GetParamRange(whichParam);
    _params[whichParam] = std::max(range.first, std::min(range.second, newValue));
  }
  
  template<typename Param_t, GameToEngTag SetMsgTag, typename Value_t>
  void HasSettableParameters<Param_t,SetMsgTag,Value_t>::HandleSetParameters(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
  {
    using MsgType = typename ExternalInterface::MessageGameToEngine_TagToType<SetMsgTag>::type;
    const MsgType& msg = event.GetData().Get_<SetMsgTag>();
    
    // Note that this won't compile if the message type does not contain "paramNames"
    // and "paramValues"
    if(msg.paramNames.size() != msg.paramValues.size()) {
      PRINT_NAMED_ERROR("AnimationStreamer.HandleSetLiveAnimationParameter.MismatchedLengths",
                        "ParamNames and ParamValues not the same length (%lu & %lu)",
                        (unsigned long)msg.paramNames.size(), (unsigned long)msg.paramValues.size());
    } else {
      
      if(msg.setUnspecifiedToDefault) {
        SetDefaultParams();
      }
      
      for(size_t i=0; i<msg.paramNames.size(); ++i) {
        SetParam(msg.paramNames[i], msg.paramValues[i]);
      }
      
    }
  }
  
  template<typename Param_t, GameToEngTag SetMsgTag, typename Value_t>
  const typename HasSettableParameters<Param_t,SetMsgTag,Value_t>::Range& HasSettableParameters<Param_t,SetMsgTag,Value_t>::GetParamRange(Param_t whichParam)
  {
    // Return specified range if there is one; otherwise return full range of Value_t
    auto rangeIter = _ranges.find(whichParam);
    if(rangeIter != _ranges.end()) {
      return rangeIter->second;
    } else {
      static const Range fullRange(std::numeric_limits<Value_t>::lowest(),
                                   std::numeric_limits<Value_t>::max());
      return fullRange;
    }
  }
  
  template<typename Param_t, GameToEngTag SetMsgTag, typename Value_t>
  void HasSettableParameters<Param_t,SetMsgTag,Value_t>::SetParamRange(Param_t whichParam, Range range)
  {
    _ranges[whichParam] = range;
  }
  
  template<typename Param_t, GameToEngTag SetMsgTag, typename Value_t>
  Result HasSettableParameters<Param_t,SetMsgTag,Value_t>::SetParamsFromJson(const Json::Value& json)
  {
    // Note that we can't loop over the members of the Json here because we don't
    // have a CLAD-generated way to look up an enum from a string value (yet?).
    // Instead, look for each enumerated enum value (which can convert to a string)
    // in the Json's members.
    size_t numFound = 0;
    for(size_t i=0; i<Param_t::NumParameters; ++i)
    {
      Param_t whichParam = static_cast<Param_t>(i);
      Value_t value;
      bool found = GetValueOptional(json, EnumToString(whichParam), value);
      if(found) {
        ++numFound;
        SetParam(whichParam, value);
      }
    }
    
    if(numFound == json.size()) {
      // A matching parameter for each member of the Json
      return RESULT_OK;
    } else {
      PRINT_NAMED_ERROR("HasSettableParameters.SetParamsFromJson.UnusedParams",
                        "%lu members in Json went unused.",
                        (unsigned long)(json.size() - numFound));
      return RESULT_FAIL;
    }
  }
  
} // namespace Vector
} // namespace Anki

#endif // __Anki_Cozmo_Util_HasSettableParameters_Impl_H__
