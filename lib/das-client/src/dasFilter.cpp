//
//  DASFilter.cpp
//  das-client
//
//  Created by Mark Pauley on 8/2/15.
//
//

#include "dasFilter.h"
#include <vector>
#include <unordered_map>
#include <cassert>
#include <sstream>

#pragma mark - Implementation
class Anki::DAS::DASFilter::DasFilterImpl {

#pragma mark FilterNode
  class FilterNode {
    std::unordered_map<std::string, FilterNode> _children;
    DASLogLevel _logLevel;

  public:
    FilterNode(DASLogLevel level) : _logLevel(level) { };

    DASLogLevel GetMinLogLevel(std::vector<const std::string>::const_iterator& logger_cur,
                               std::vector<const std::string>::const_iterator& logger_end) const
    {
      if (logger_cur == logger_end) {
        return _logLevel;
      }
      else {
        const auto& it = _children.find(*logger_cur);
        if (it == _children.end()) {
          return _logLevel;
        }
        else {
          return it->second.GetMinLogLevel(++logger_cur,
                                          logger_end);
        }
      }
    }

    void SetMinLogLevel(std::vector<const std::string>::const_iterator& logger_cur,
                        std::vector<const std::string>::const_iterator& logger_end,
                        const DASLogLevel logLevel)
    {
      if (logger_cur == logger_end) {
        // We've hit the end, set the log level as desired.
        SetMinLogLevel(logLevel);
      }
      else {
        auto it = _children.find(*logger_cur);
        if (it == _children.end()) {
          // We haven't hit this path before, make a child here
          // defaulting to the current log level
          auto result = _children.emplace(std::piecewise_construct,
                            std::forward_as_tuple(*logger_cur),
                            std::forward_as_tuple(_logLevel));
          assert(result.second);
          assert(result.first != _children.end());
          it = result.first;
        }
        it->second.SetMinLogLevel(++logger_cur, logger_end, logLevel);
      }
    }

    void SetMinLogLevel(const DASLogLevel logLevel)
    {
      _logLevel = logLevel;
    }

  };// end filter node

#pragma mark FilterImpl
  FilterNode _root;

public:
  DasFilterImpl() : _root(DASLogLevel_NumLevels) { };

  DASLogLevel GetMinLogLevel(const std::string &logger) const
  {
    std::istringstream ss(logger);
    std::string token;
    std::vector<const std::string> splitLogger;
    while(std::getline(ss, token, '.')) {
      splitLogger.emplace_back(token);
    }
    auto loggerBegin = splitLogger.cbegin();
    auto loggerEnd = splitLogger.cend();
    return _root.GetMinLogLevel(loggerBegin, loggerEnd);
  }

  void SetMinLogLevel(const std::string& logger, const DASLogLevel logLevel)
  {
    std::istringstream ss(logger);
    std::string token;
    std::vector<const std::string> splitLogger;
    while(std::getline(ss, token, '.')) {
      splitLogger.emplace_back(token);
    }
    auto loggerBegin = splitLogger.cbegin();
    auto loggerEnd = splitLogger.cend();
    _root.SetMinLogLevel(loggerBegin, loggerEnd, logLevel);
  }

  void SetMinLogLevel(const DASLogLevel logLevel)
  {
    _root.SetMinLogLevel(logLevel);
  }

};


#pragma mark - Interface
namespace Anki {
namespace DAS {

DASFilter::DASFilter()
: pimpl(new DasFilterImpl())
{

}

DASFilter::~DASFilter()
{
  delete pimpl;
}

void DASFilter::ClearLogLevels()
{
  delete pimpl;
  pimpl = new DasFilterImpl();
}

void DASFilter::SetRootLogLevel(DASLogLevel level)
{
  pimpl->SetMinLogLevel(level);
}
void DASFilter::SetMinLogLevel(const std::string& loggerName, DASLogLevel level)
{
  pimpl->SetMinLogLevel(loggerName, level);
}

DASLogLevel DASFilter::GetMinLogLevel(const std::string& loggerName) const
{
  return pimpl->GetMinLogLevel(loggerName);
}

}
}
