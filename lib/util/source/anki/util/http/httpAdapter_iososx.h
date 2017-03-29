//
//  httpAdapter_iososx.h
//  OverDrive
//
//  Created by Aubrey Goodman on 3/5/15.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#include "util/http/abstractHttpAdapter.h"
#import <Foundation/Foundation.h>
#include <mutex>

namespace Anki {
namespace Util {
class HttpAdapter;
} // namespace Util
} // namespace Anki

@interface HttpAdapterURLSessionDelegate : NSObject <NSURLSessionDownloadDelegate,
                                                     NSURLSessionTaskDelegate,
                                                     NSURLSessionDelegate>
{
  Anki::Util::HttpAdapter* m_Adapter;
}

- (id) initWithAdapter:(Anki::Util::HttpAdapter *)adapter;

- (void) clearAdapter;

- (void) URLSession:(NSURLSession *)session didBecomeInvalidWithError:(NSError *)error;

- (void) URLSession:(NSURLSession *)session
didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
  completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition,
                              NSURLCredential *credential))completionHandler;

- (void) URLSessionDidFinishEventsForBackgroundURLSession:(NSURLSession *)session;

- (void) URLSession:(NSURLSession *)session
               task:(NSURLSessionTask *)task
didCompleteWithError:(NSError *)error;

- (void) URLSession:(NSURLSession *)session
               task:(NSURLSessionTask *)task
didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge
  completionHandler:(void (^)(NSURLSessionAuthChallengeDisposition disposition,
                              NSURLCredential *credential))completionHandler;

- (void) URLSession:(NSURLSession *)session
               task:(NSURLSessionTask *)task
    didSendBodyData:(int64_t)bytesSent
     totalBytesSent:(int64_t)totalBytesSent
totalBytesExpectedToSend:(int64_t)totalBytesExpectedToSend;

- (void) URLSession:(NSURLSession *)session
               task:(NSURLSessionTask *)task
willPerformHTTPRedirection:(NSHTTPURLResponse *)response
         newRequest:(NSURLRequest *)request
  completionHandler:(void (^)(NSURLRequest *))completionHandler;

- (void) URLSession:(NSURLSession *)session
       downloadTask:(NSURLSessionDownloadTask *)downloadTask
  didResumeAtOffset:(int64_t)fileOffset
 expectedTotalBytes:(int64_t)expectedTotalBytes;

- (void) URLSession:(NSURLSession *)session
       downloadTask:(NSURLSessionDownloadTask *)downloadTask
       didWriteData:(int64_t)bytesWritten
  totalBytesWritten:(int64_t)totalBytesWritten
totalBytesExpectedToWrite:(int64_t)totalBytesExpectedToWrite;

- (void) URLSession:(NSURLSession *)session
       downloadTask:(NSURLSessionDownloadTask *)downloadTask
didFinishDownloadingToURL:(NSURL *)location;
@end



namespace Anki {
namespace Util {

  class HttpAdapter : public IHttpAdapter
  {
  public:

    explicit HttpAdapter();
    ~HttpAdapter();

    void ShutdownAdapter();

    bool IsAdapterUp();

    void StartRequest(HttpRequest request,
                      Dispatch::Queue* queue,
                      HttpRequestCallback callback,
                      HttpRequestDownloadProgressCallback progressCallback = nullptr);

    void ExecuteCallback(const uint64_t hash,
                         const int responseCode,
                         std::map<std::string,std::string>& responseHeaders,
                         std::vector<uint8_t>& responseBody) { /* unused do nothing */ }

    void ExecuteDownloadProgressCallback(const uint64_t hash,
                                         const int64_t bytesWritten,
                                         const int64_t totalBytesWritten,
                                         const int64_t totalBytesExpectedToWrite) { /* unused */ }

    void DidCompleteWithError(NSURLSessionTask* task,
                              NSError* error);

    void DidFinishDownloadingToURL(NSURLSessionDownloadTask* downloadTask,
                                   NSURL* location);

    void DidWriteData(NSURLSessionDownloadTask* downloadTask,
                      int64_t bytesWritten,
                      int64_t totalBytesWritten,
                      int64_t totalBytesExpectedToWrite);

    int GetHttpStatusCodeForNSError(NSError* error) const {
      if (error) {
        return GetHttpStatusCodeForNSErrorCode(error.code);
      }
      return kHttpResponseCodeOK;
    }

    int GetHttpStatusCodeForNSErrorCode(NSInteger errorCode) const;

  private:
    struct HttpRequestState {
      HttpRequestState()
      : HttpRequestState(HttpRequest(), nullptr, nullptr, nullptr, nullptr) { }
      HttpRequestState(HttpRequest request,
                       Dispatch::Queue* queue,
                       HttpRequestCallback callback,
                       HttpRequestDownloadProgressCallback progressCallback,
                       NSURL* destinationUrl)
                       : request(request)
                       , queue(queue)
                       , callback(callback)
                       , progressCallback(progressCallback)
                       , destinationUrl(destinationUrl) { }
      HttpRequest request;
      Dispatch::Queue* queue;
      HttpRequestCallback callback;
      HttpRequestDownloadProgressCallback progressCallback;
      NSURL* destinationUrl;
    };


    HttpAdapterURLSessionDelegate* _urlSessionDelegate;
    NSURLSession* _urlSession;
    std::map<NSURLSessionTask*, HttpRequestState> _taskToStateMap;
    std::mutex _taskToStateMapMutex;
    std::mutex _adapterStateMutex;
    AdapterState _adapterState;

    AdapterState GetAdapterState();
    void SetAdapterState(const AdapterState state);

    bool SetRequestStateForTask(NSURLSessionTask* task, HttpRequestState requestState);

    size_t EraseRequestStateForTask(NSURLSessionTask* task);

    bool GetRequestStateFromTask(NSURLSessionTask* task, HttpRequestState& requestState);


    void ProcessGetRequest(NSString* httpMethod, HttpRequest& request,
                           Dispatch::Queue* queue, HttpRequestCallback callback,
                           HttpRequestDownloadProgressCallback progressCallback);

    void ProcessUploadRequest(NSString* httpMethod,
                              HttpRequest& request,
                              Dispatch::Queue* queue,
                              HttpRequestCallback callback);

    NSMutableURLRequest* CreateURLRequest(const HttpRequest& request, NSString* httpMethod);

    void SetRequestHeaders(const HttpRequest& request, NSMutableURLRequest* urlRequest);

    void SetResponseHeaders(NSURLResponse* response, std::map<std::string,std::string>& responseHeaders);

    void ConvertParamsToBodyString(const HttpRequest& request, NSMutableString *bodyString);

    void DoCallback(Dispatch::Queue* queue,
                    HttpRequestCallback callback,
                    HttpRequest request,
                    NSData* data,
                    NSURLResponse* response,
                    NSError* error);
  };

} // namespace DriveEngine
} // namespace Anki
