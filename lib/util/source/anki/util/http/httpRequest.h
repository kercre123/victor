//
//  httpRequest.h
//  driveEngine
//
//  Created by Aubrey Goodman on 3/4/15.
//
//

#ifndef __Anki_Util_Http_HttpRequest_H__
#define __Anki_Util_Http_HttpRequest_H__

#include <map>
#include <string>
#include <vector>

namespace Anki {
namespace Util {

static int32_t kHttpRequestTimeOutMSec = 10000;

enum HttpMethod {
  HttpMethodGet = 0,
  HttpMethodPost,
  HttpMethodPut,
  HttpMethodDelete,
  HttpMethodHead
};

inline const char* HttpMethodToString(const HttpMethod m)
{
  switch(m) {
    case HttpMethodGet: return "GET"; break;
    case HttpMethodPost: return "POST"; break;
    case HttpMethodPut: return "PUT"; break;
    case HttpMethodDelete: return "DELETE"; break;
    case HttpMethodHead: return "HEAD"; break;
    default: return nullptr;
  }
  return nullptr;
}

class HttpRequest
{
public:
  explicit HttpRequest()
  : timeOutMSec(kHttpRequestTimeOutMSec)
  {}

  bool operator<(HttpRequest request) const
  {
    return uri < request.uri;
  }

  std::string uri;

  HttpMethod method;

  std::map<std::string,std::string> headers;

  std::map<std::string,std::string> params;

  std::vector<uint8_t> body;

  std::string storageFilePath;

  int32_t timeOutMSec;

};

// Error codes borrowed from OSX/iOS NSURLError.h
static constexpr int kHttpResponseErrorTimedOut = -1001;
static constexpr int kHttpResponseErrorCannotConnectToHost = -1004;
static constexpr int kHttpResponseErrorNetworkConnectionLost = -1005;
static constexpr int kHttpResponseErrorDNSLookupFailed = -1006;
static constexpr int kHttpResponseErrorNotConnectedToInternet = -1009;

static constexpr int kHttpResponseCodeOK = 200;
static constexpr int kHttpResponseCodeMultipleChoices = 300;
static constexpr int kHttpResponseCodeBadRequest = 400;
static constexpr int kHttpResponseCodeUnauthorized = 401;
static constexpr int kHttpResponseCodeForbidden = 403;
static constexpr int kHttpResponseCodeNotFound = 404;
static constexpr int kHttpResponseCodeProxyAuthenticationRequired = 407;
static constexpr int kHttpResponseCodeConflict = 409;
static constexpr int kHttpResponseCodeRequestEntityTooLarge = 413;

inline bool isHttpSuccessCode(int responseCode) {
  return (responseCode >= kHttpResponseCodeOK
          && responseCode < kHttpResponseCodeMultipleChoices);
}

}
}

#endif
