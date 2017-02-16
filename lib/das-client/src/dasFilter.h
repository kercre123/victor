//
//  DASFilter.hpp
//  das-client
//
//  Created by Mark Pauley on 8/2/15.
//
//

#ifndef DASFilter_hpp
#define DASFilter_hpp

#include "DAS.h"
#include <string>

namespace Anki
{
namespace DAS
{

class DASFilter
{

public:
  DASFilter();
  ~DASFilter();

  void ClearLogLevels();
  void SetRootLogLevel(DASLogLevel level);
  void SetMinLogLevel(const std::string& loggerName, DASLogLevel level);
  DASLogLevel GetMinLogLevel(const std::string& loggerName) const;

private:
  class DasFilterImpl;

  DasFilterImpl* pimpl;
};


}
}


#endif /* DASFilter_hpp */
