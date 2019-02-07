/**
 * File: simpleArgParser.h
 *
 * Author: Andrew Stein
 * Date:   12/1/2018
 *
 * Description: Very basic command line argument parser. Assumes "--param value" pairs are provided.
 *              If no value is provided (e.g, "--param1 --param2 value"), the value is assumed to be "true".
 *              Use GetArg() to interpret the given parameter as a particular value type as needed.
 *              Add additional template specializations of GetArgHelper() for any special type translations needed.
 *              There is very little error checking, so strings must match. However, on destruction,
 *                a warning will be issued for any unused parameters.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Coretech_Vision_FaceEval_SimpleArgParser_H__
#define __Anki_Coretech_Vision_FaceEval_SimpleArgParser_H__

#include "util/logging/logging.h"
#include <map>
#include <set>

namespace Anki {
namespace Vision {

class SimpleArgParser
{
public:
  
  SimpleArgParser(int argc, char* argv[])
  {
    for(int i=1; i<argc; i+=2)
    {
      const std::string name(argv[i]);
      std::string value("true");
      if( (i+1)<argc && (argv[i+1][0] != '-'))
      {
        value = argv[i+1];
      }
      else
      {
        --i; // make the next +=2 just move us to the next arg
      }
      
      _params[name] = value;
    }
  }
  
  ~SimpleArgParser()
  {
    for(const auto& param : _params)
    {
      if(_used.count(param.first)==0)
      {
        LOG_WARNING("SimpleArgParser.UnusedArgument", "%s", param.first.c_str());
      }
    }
  }
  
  template<typename T>
  bool GetArg(const char* arg, T& value) const
  {
    auto iter = _params.find(arg);
    if(iter == _params.end())
    {
      return false;
    }
    
    value = GetArgHelper<T>(iter->second);
    _used.insert(arg);
    return true;
  }
  
private:
  
  template<typename T>
  T GetArgHelper(const std::string& value) const;
  
  std::map<std::string, std::string> _params;
  mutable std::set<std::string> _used;
};

template<>
inline int SimpleArgParser::GetArgHelper<int>(const std::string& value) const
{
  return std::stoi(value);
}

template<>
inline bool SimpleArgParser::GetArgHelper<bool>(const std::string& value) const
{
  if(value == "true" || value == "TRUE" || value == "True")
  {
    return true;
  }
  else if(value == "false" || value == "FALSE" || value == "False")
  {
    return false;
  }
  return (std::stoi(value) != 0);
}

template<>
inline std::string SimpleArgParser::GetArgHelper<std::string>(const std::string& value) const
{
  return value;
}

template<>
inline float SimpleArgParser::GetArgHelper<float>(const std::string& value) const
{
  return std::stof(value);
}

// NOTE: Assume command line angle arguments are given in degrees (which is generally more natural!)
template<>
inline Radians SimpleArgParser::GetArgHelper<Radians>(const std::string& value) const
{
  return Radians(DEG_TO_RAD(std::stof(value)));
}
  
} // namespace Vision
} // namespave Anki

#endif /* __Anki_Coretech_Vision_FaceEval_SimpleArgParser_H__ */

