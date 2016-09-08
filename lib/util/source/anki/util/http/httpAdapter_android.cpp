//
//  httpAdapter_android.cpp
//  driveEngine
//
//  Created by Aubrey Goodman on 3/19/15.
//
//

#ifdef ANDROID

#include "util/http/httpAdapter_android.h"
#include "util/http/httpRequest.h"

#include "util/console/consoleInterface.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/jni/jniUtils.h"
#include "util/logging/logging.h"

CONSOLE_VAR(int, kHttpRequestTimeOutMSec, "Settings.Debug", 10000);

namespace {
std::mutex instancesMutex;
std::unordered_map<uint64_t, Anki::Util::HttpAdapter*> requestInstances;
}

namespace Anki {
namespace Util {

std::atomic<uint64_t> HttpAdapter::_sHashCounter{1};

HttpAdapter::HttpAdapter()
{
  auto envWrapper = JNIUtils::getJNIEnvWrapper();
  JNIEnv* env = envWrapper->GetEnv();
  if (nullptr != env) {
    JClassHandle httpClass{env->FindClass("com/anki/util/http/HttpAdapter"), env};
    if (httpClass == nullptr) {
      ASSERT_NAMED(false, "HttpAdapter_Android.Init.ClassNotFound");
      return;
    }

    // get singleton instance of adapter
    jmethodID instanceMethodID = env->GetStaticMethodID(httpClass.get(), "getInstance", "()Lcom/anki/util/http/HttpAdapter;");
    _javaObject = env->CallStaticObjectMethod(httpClass.get(), instanceMethodID);

    // get global ref to class
    jobject oldObject = _javaObject;
    _javaObject = env->NewGlobalRef(_javaObject);
    env->DeleteLocalRef(oldObject);
    if (_javaObject == nullptr) {
      ASSERT_NAMED(false, "HttpAdapter_Android.Init.ClassGlobalRef");
      return;
    }

    // find StartRequest method
    _startRequestMethodID =
      env->GetMethodID(httpClass.get(), "startRequest",
        "(JLjava/lang/String;[Ljava/lang/String;[Ljava/lang/String;[BILjava/lang/String;I)V");
    if (_startRequestMethodID == nullptr) {
      ASSERT_NAMED(false, "HttpAdapter_Android.Init.StartRequestMethodNotFound");
      return;
    }
  }
  else {
    PRINT_NAMED_ERROR("HttpAdapter_Android.Init", "can't get JVM environment");
  }
}

HttpAdapter::~HttpAdapter()
{
  auto envWrapper = JNIUtils::getJNIEnvWrapper();
  JNIEnv* env = envWrapper->GetEnv();
  if (nullptr != env && nullptr != _javaObject) {
    env->DeleteGlobalRef(_javaObject);
  }
}

void HttpAdapter::StartRequest(const HttpRequest& request,
                               Util::Dispatch::Queue* queue,
                               HttpRequestCallback callback,
                               HttpRequestDownloadProgressCallback progressCallback)
{
  auto envWrapper = JNIUtils::getJNIEnvWrapper();
  JNIEnv* env = envWrapper->GetEnv();
  JNI_CHECK(env);
  if (_javaObject == nullptr || _startRequestMethodID == nullptr) {
    ASSERT_NAMED(false, "HttpAdapter_Android.StartRequest.not_initialized");
    return;
  }

  JStringHandle uri{env->NewStringUTF(request.uri.c_str()), env};

  // encode headers and params into array [k1,v1,k2,v2,...]
  std::vector<jstring> stringRefs;
  JObjectArrayHandle headers{JNIUtils::convertStringMapToJObjectArray(env, request.headers, stringRefs), env};
  JObjectArrayHandle params{JNIUtils::convertStringMapToJObjectArray(env, request.params, stringRefs), env};

  uint64_t hash;
  {
    std::lock_guard<std::mutex> lock(_callbackMutex);
    hash = _sHashCounter.fetch_add(1);
    _requestResponseMap.emplace(hash,
      HttpRequestResponseType{request, queue, callback, progressCallback});
  }
  {
    std::lock_guard<std::mutex> lock(instancesMutex);
    requestInstances.emplace(hash, this);
  }

  jint httpMethod = (jint)(request.method);
  jsize bodySize = (jsize)request.body.size();
  jint requestTimeout = (jint)kHttpRequestTimeOutMSec;

  JByteArrayHandle jbodyArray{env->NewByteArray(bodySize), env};
  env->SetByteArrayRegion(jbodyArray.get(), 0, bodySize, (const jbyte*)(request.body.data()));

  JStringHandle destinationPath{env->NewStringUTF(request.destinationPath.c_str()), env};

  PRINT_CH_INFO("HTTP", "http_adapter.request", "%s :: %s",
                       HttpMethodToString(request.method),
                       request.uri.c_str());

  PRINT_NAMED_DEBUG("http_adapter.start_request.call_java", "");
  env->CallVoidMethod(_javaObject, _startRequestMethodID, hash, uri.get(), headers.get(), params.get(),
                      jbodyArray.get(), httpMethod, destinationPath.get(), requestTimeout);
  PRINT_NAMED_DEBUG("http_adapter.start_request.call_java_done", "");

  for (jstring& stringRef : stringRefs) {
    env->DeleteLocalRef(stringRef);
  }
}

void HttpAdapter::GetHttpRequestResponseTypeFromHash(const uint64_t hash,
                                                     HttpRequestResponseType& response)
{
  std::lock_guard<std::mutex> lock(_callbackMutex);
  response = _requestResponseMap.at(hash);
}


void HttpAdapter::ExecuteCallback(const uint64_t hash,
                                  const int responseCode,
                                  const std::map<std::string,std::string>& responseHeaders,
                                  const std::vector<uint8_t>& responseBody)
{
  PRINT_NAMED_DEBUG("http_adapter.execute_callback", "%llx %d %d",
                    hash, responseCode, responseBody.size());
  HttpRequestResponseType response;
  GetHttpRequestResponseTypeFromHash(hash, response);
  PRINT_NAMED_DEBUG("http_adapter.execute_callback.response_found", "");

  PRINT_CH_INFO("HTTP", "http_adapter.response", "%d :: %s",
                       responseCode,
                       response.request.uri.c_str());

  Dispatch::Async(response.queue, [this, hash, response, responseBody, responseCode, responseHeaders] {
    response.callback(response.request, responseCode, responseHeaders, responseBody);

    PRINT_NAMED_DEBUG("http_adapter.execute_callback.callback_executed", "");
    {
      std::lock_guard<std::mutex> lock(_callbackMutex);
      _requestResponseMap.erase(hash);
      PRINT_NAMED_DEBUG("http_adapter.execute_callback.completed", "");
    }
    {
      std::lock_guard<std::mutex> lock(instancesMutex);
      requestInstances.erase(hash);
    }

  });
}

void HttpAdapter::ExecuteDownloadProgressCallback(const uint64_t hash,
                                                  const int64_t bytesWritten,
                                                  const int64_t totalBytesWritten,
                                                  const int64_t totalBytesExpectedToWrite)
{
  HttpRequestResponseType response;
  GetHttpRequestResponseTypeFromHash(hash, response);

  if (response.progressCallback) {
    Dispatch::Async(response.queue,
      [this, response, bytesWritten, totalBytesWritten, totalBytesExpectedToWrite]() {
        response.progressCallback(response.request, bytesWritten,
                                  totalBytesWritten, totalBytesExpectedToWrite);
    });
  }
}


}
}


extern "C"
{

void JNICALL
Java_com_anki_util_http_HttpAdapter_NativeHttpRequestCallback(JNIEnv* env, jobject thiz,
                                                                   jlong hash, jint responseCode,
                                                                   jobjectArray responseHeaders,
                                                                   jbyteArray responseBody)
{
  jbyte* bytes = env->GetByteArrayElements(responseBody, NULL);
  if( bytes != NULL ) {
    std::map<std::string,std::string> headers;
    jsize stringCount = env->GetArrayLength(responseHeaders);
    for (jsize i = 0; i < stringCount; i += 2) {
      Anki::Util::JStringHandle jKey{(jstring) env->GetObjectArrayElement(responseHeaders, i), env};
      Anki::Util::JStringHandle jValue{(jstring) env->GetObjectArrayElement(responseHeaders, i + 1), env};
      const char* key = env->GetStringUTFChars(jKey.get(), 0);
      const char* value = env->GetStringUTFChars(jValue.get(), 0);
      headers.emplace(key, value);
      env->ReleaseStringUTFChars(jKey.get(), key);
      env->ReleaseStringUTFChars(jValue.get(), value);
    }

    jsize bytesLen = env->GetArrayLength(responseBody);
    std::vector<uint8_t> body(bytes, bytes + bytesLen);
    env->ReleaseByteArrayElements(responseBody, bytes, JNI_ABORT);
    auto instanceIter = requestInstances.find(hash);
    if (instanceIter != requestInstances.end()) {
      instanceIter->second->ExecuteCallback(hash, responseCode, headers, body);
    }
  }
}

void JNICALL
Java_com_anki_util_http_HttpAdapter_NativeHttpRequestDownloadProgressCallback(JNIEnv* env,
                                                                  jobject thiz,
                                                                  jlong hash,
                                                                  jlong bytesWritten,
                                                                  jlong totalBytesWritten,
                                                                  jlong totalBytesExpectedToWrite)
{
  auto instanceIter = requestInstances.find(hash);
  if (instanceIter != requestInstances.end()) {
    instanceIter->second->ExecuteDownloadProgressCallback(hash, bytesWritten,
                                            totalBytesWritten, totalBytesExpectedToWrite);
  }
}

}

#endif // ANDROID
