/**
* File: victor/dasmgr/dasConfig.h
*
* Description: DASConfig class declarations
*
* Copyright: Anki, inc. 2018
*
*/

#ifndef __victor_dasmgr_dasConfig_h
#define __victor_dasmgr_dasConfig_h

#include <string>

// Forward declarations
namespace Json {
  class Value;
}

namespace Anki {
namespace Victor {

class DASConfig
{
public:
  DASConfig(const std::string & url, size_t queue_threshold_size, size_t max_deferrals_size);

  // DAS endpoint URL
  const std::string & GetURL() const { return _url; }

  // How many events do we buffer before send? Counted by events.
  size_t GetQueueThresholdSize() const { return _queue_threshold_size; }

  // How many queues are we willing to hold for deferred send? Counted by queues, not events.
  size_t GetMaxDeferralsSize() const { return _max_deferrals_size; }

  // Helper methods to parse DAS configuration file.
  // Helper methods return nullptr on error.
  // Configuration format looks like this:
  // {
  //   "dasConfig" : {
  //     "url": "string",
  //     "queue_threshold_size" : uint,
  //     "max_deferrals_size" : uint
  //   }
  // }
  //
  static std::unique_ptr<DASConfig> GetDASConfig(const Json::Value & json);
  static std::unique_ptr<DASConfig> GetDASConfig(const std::string & path);

private:
  std::string _url;
  size_t _queue_threshold_size;
  size_t _max_deferrals_size;
};

} // end namespace Victor
} // end namespace Anki

#endif // __platform_dasmgr_dasConfig_h
