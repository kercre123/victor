/*
 * File:          cozmoWebServer.h
 * Date:          01/29/2018
 * Author:        Paul Terry
 *
 * Description:   A platform-independent container for spinning up all the pieces
 *                required to run Cozmo Web Server Process.
 *
 */

#ifndef ANKI_COZMO_WEB_SERVER_H
#define ANKI_COZMO_WEB_SERVER_H

#include "json/json.h"

#include "coretech/common/shared/types.h"

namespace Anki {
  
// Forward declaration:
namespace Util {
namespace Data {
  class DataPlatform;
}
}

namespace Cozmo {

// Forward declarations
namespace WebService {
  class WebService;
}
// todo if needed class CozmoWebServerContext;

class CozmoWebServerEngine
{
public:

  CozmoWebServerEngine(Util::Data::DataPlatform* dataPlatform);
  ~CozmoWebServerEngine();

  Result Init();

  // Hook this up to whatever is ticking the game "heartbeat"
  Result Update(BaseStationTime_t currTime_nanosec);

protected:

  bool               _isInitialized = false;

  std::unique_ptr<WebService::WebService>     _webService;
  std::unique_ptr<Util::Data::DataPlatform>   _dataPlatform;
  //std::unique_ptr<CozmoWebServerContext>      _context;
};
  

} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_WEB_SERVER_H
