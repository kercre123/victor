//
//  httpAdapter_android.cpp
//  driveEngine
//
//  Created by Aubrey Goodman on 3/19/15.
//
//

#include "driveEngine/util/http/httpAdapter_android.h"
#include "driveEngine/util/codeTimer/codeTimer.h"
#include "driveEngine/util/stringUtils/stringUtils.h"
#include "driveEngine/valueObjects/jGlobalObjects/jniUtils.h"
#include "driveEngine/rushHour.h"
#include "util/logging/logging.h"
#include <util/dispatchQueue/dispatchQueue.h>

jobjectArray ConvertStringMapToJObjectArray(JNIEnv* env, const std::map<std::string,std::string>& stringMap, std::vector<jstring>& stringRefs)
{
  jclass clazz = env->FindClass("java/lang/String");
  jobjectArray arr = env->NewObjectArray((jsize)(stringMap.size()*2), clazz, 0);
  int k = 0;
  for (std::map<std::string,std::string>::const_iterator it = stringMap.begin(); it != stringMap.end(); ++it, k+=2) {
    std::string key = it->first;
    std::string val = it->second;
    jstring keyStr = env->NewStringUTF(key.c_str());
    jstring valStr = env->NewStringUTF(val.c_str());
    stringRefs.push_back(keyStr);
    stringRefs.push_back(valStr);
    env->SetObjectArrayElement(arr, k, keyStr);
    env->SetObjectArrayElement(arr, k+1, valStr);
  }
  env->DeleteLocalRef(clazz);
  return arr;
}

namespace Anki {
namespace Util {

HttpAdapter::HttpAdapter(jobject jObject)
: _jObject(jObject)
{
  JNIEnv* env = _jObject.getJNIEnv();
  if( nullptr != env ) {
    _startRequestMethodID =
      env->GetMethodID(_jObject.GetJClass(), "startRequest",
        "(JLjava/lang/String;[Ljava/lang/String;[Ljava/lang/String;[BILjava/lang/String;)V");
  }
  _hashCounter = 42;
}

HttpAdapter::~HttpAdapter()
{
}

void HttpAdapter::StartRequest(const HttpRequest& request, Util::Dispatch::Queue* queue, HttpRequestCallback callback)
{
  JNIEnv* env = _jObject.getJNIEnv();
  JNI_CHECK(env);

  jstring uri = env->NewStringUTF(request.uri.c_str());
  
  // encode headers and params into array [k1,v1,k2,v2,...]
  std::vector<jstring> stringRefs;
  jobjectArray headers = ConvertStringMapToJObjectArray(env, request.headers, stringRefs);
  jobjectArray params = ConvertStringMapToJObjectArray(env, request.params, stringRefs);

  uint64_t hash;
  {
    std::lock_guard<std::mutex> lock(_callbackMutex);
    hash = ++_hashCounter;
    _requestResponseMap.emplace( hash, HttpRequestResponseType{request, queue, callback} );
  }
  
  jint httpMethod = (jint)(request.method);
  jsize bodySize = (jsize)request.body.size();
  
  jbyteArray jbodyArray = env->NewByteArray(bodySize);
  uint8_t bodyArray[bodySize];
  std::copy(request.body.begin(), request.body.end(), bodyArray);
  env->SetByteArrayRegion(jbodyArray, 0, bodySize, (jbyte*)(bodyArray));

  jstring destinationPath = env->NewStringUTF(request.destinationPath.c_str());
  
  PRINT_CHANNELED_INFO("HTTP", "http_adapter.request", "%s :: %s",
                       HttpMethodToString(request.method),
                       request.uri.c_str());
  
  PRINT_NAMED_DEBUG("http_adapter.start_request.call_java", "");
  env->CallVoidMethod(_jObject.getJObject(), _startRequestMethodID, hash, uri, headers, params,
                      jbodyArray, httpMethod, destinationPath);
  PRINT_NAMED_DEBUG("http_adapter.start_request.call_java_done", "");

  for (jstring& stringRef : stringRefs) {
    env->DeleteLocalRef(stringRef);
  }

  env->DeleteLocalRef(uri);
  env->DeleteLocalRef(headers);
  env->DeleteLocalRef(params);
  env->DeleteLocalRef(jbodyArray);
  env->DeleteLocalRef(destinationPath);
}
  
void HttpAdapter::ExecuteCallback(const uint64_t hash, const int responseCode, const std::map<std::string,std::string>& responseHeaders, const std::vector<uint8_t>& responseBody)
{
  PRINT_NAMED_DEBUG("http_adapter.execute_callback", "%llx %d %d",
                    hash, responseCode, responseBody.size());
  HttpRequestResponseType response;
  {
    std::lock_guard<std::mutex> lock(_callbackMutex);
    assert(_requestResponseMap.find(hash) != _requestResponseMap.end());
    response = _requestResponseMap[hash];
    PRINT_NAMED_DEBUG("http_adapter.execute_callback.response_found", "");
  }
  
  PRINT_CHANNELED_INFO("HTTP", "http_adapter.response", "%d :: %s",
                       responseCode,
                       response.request.uri.c_str());

  Util::Dispatch::Async(response.queue, [this, hash, response, responseBody, responseCode, responseHeaders] {
    
    auto start = CodeTimer::Start();
    response.callback(response.request, responseCode, responseHeaders, responseBody);
    int elapsedMillis = CodeTimer::MillisecondsElapsed(start);
    if (elapsedMillis >= kHttpRequestCallbackTookTooLongMilliseconds) {
      // http_adapter.callback_took_too_long - data: duration in ms, sval: uri
      
      // Remove PII
      std::string piiRemovedString = AnkiUtil::RemovePII(response.request.uri);
      
      Util::sEvent("http_adapter.callback_took_too_long",
                   {{DDATA, std::to_string(elapsedMillis).c_str()}},
                   piiRemovedString.c_str());
    }

    PRINT_NAMED_DEBUG("http_adapter.execute_callback.callback_executed", "");
    {
      std::lock_guard<std::mutex> lock(_callbackMutex);
      _requestResponseMap.erase(hash);
      PRINT_NAMED_DEBUG("http_adapter.execute_callback.completed", "");
    }
    
  });
}

}
}


extern "C"
{
  
void JNICALL
Java_com_anki_overdrive_http_HttpAdapter_NativeHttpRequestCallback(JNIEnv* env, jobject thiz,
                                                                   jlong hash, jint responseCode,
                                                                   jobjectArray responseHeaders,
                                                                   jbyteArray responseBody)
{
  jbyte* bytes = env->GetByteArrayElements(responseBody, NULL);
  if( bytes != NULL ) {
    std::map<std::string,std::string> headers;
    jsize stringCount = env->GetArrayLength(responseHeaders);
    for (jsize i = 0; i < stringCount; i += 2) {
      jstring jKey = (jstring) env->GetObjectArrayElement(responseHeaders, i);
      jstring jValue = (jstring) env->GetObjectArrayElement(responseHeaders, i + 1);
      const char* key = env->GetStringUTFChars(jKey, 0);
      const char* value = env->GetStringUTFChars(jValue, 0);
      headers.emplace(key, value);
      env->ReleaseStringUTFChars(jKey, key);
      env->ReleaseStringUTFChars(jValue, value);
      env->DeleteLocalRef(jKey);
      env->DeleteLocalRef(jValue);
    }

    jsize bytesLen = env->GetArrayLength(responseBody);
    std::vector<uint8_t> body(bytes, bytes + bytesLen);
    env->ReleaseByteArrayElements(responseBody, bytes, JNI_ABORT);
    Anki::DriveEngine::RushHour::getRefInstance().GetHttpAdapter()->ExecuteCallback(hash,
                                                                                    responseCode,
                                                                                    headers,
                                                                                    body);
  }
}

}
