/**
* File: audioCaptureSystem.cpp
*
* Author: Lee Crippen
* Created: 2/6/2017
*
* Description: Android implementation of interface.
*
* Copyright: Anki, Inc. 2017
*
*/

#include "audioCaptureSystem.h"
#include "util/jni/jniUtils.h"
#include "util/logging/logging.h"

#include <android/log.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <mutex>
#include <string>
#include <utility>

#define AUDIOCAPTURE_ERROR( message, args... ) __android_log_print( ANDROID_LOG_ERROR, "AudioCaptureSystem", message, ##args )

namespace {
  Anki::AudioUtil::AudioCaptureSystem::RequestCapturePermissionCallback     _requestCapturePermissionCallback;
  std::mutex                                                                _requestPermCallbackLock;
}

namespace Anki {
namespace AudioUtil {

struct AudioCaptureSystemData
{
  AudioCaptureSystem*               _captureSystem = nullptr;
  bool                              _recording = false;

  // Pair of audio sample buffers to hold recorded audio
  AudioChunk                        _inputBuffers[2] = { AudioChunk(kSamplesPerChunk), AudioChunk(kSamplesPerChunk) };
  uint32_t                          _enqueuedBuffer = 0;

  // Android specific audio recording objects
  SLObjectItf                       _engineObject;
  SLEngineItf                       _engineEngine;
  SLObjectItf                       _recorderObject;
  SLRecordItf                       _recorderRecord;
  SLAndroidSimpleBufferQueueItf     _recorderBufferQueue;

  void HandleCallback()
  {
    if(_recording)
    {
      uint32_t fullBuffer = _enqueuedBuffer;

      // Note that because we're just alternating between buffers here, the data passed to the callback below
      // needs to be copied out (or processed in place) before we fill up the alternate buffer and switch back.
      _enqueuedBuffer = (0 == _enqueuedBuffer) ? 1 : 0;
      (*_recorderBufferQueue)->Enqueue(_recorderBufferQueue, _inputBuffers[_enqueuedBuffer].data(), kBytesPerChunk);

      auto dataCallback = _captureSystem->GetDataCallback();
      if (dataCallback)
      {
        dataCallback(_inputBuffers[fullBuffer].data(), kSamplesPerChunk);
      }
    }
  }

  ~AudioCaptureSystemData()
  {
    // destroy audio recorder object, and invalidate all associated interfaces
    if (_recorderObject != nullptr)
    {
      (*_recorderObject)->Destroy(_recorderObject);
      _recorderObject = nullptr;
      _recorderRecord = nullptr;
      _recorderBufferQueue = nullptr;
    }

    // destroy engine object, and invalidate all associated interfaces
    if (_engineObject != nullptr)
    {
      (*_engineObject)->Destroy(_engineObject);
      _engineObject = nullptr;
      _engineEngine = nullptr;
    }
  }
};

static void HandleCallbackEntry(SLAndroidSimpleBufferQueueItf bufferQueue, void *inUserData)
{
  AudioCaptureSystemData* data = static_cast<AudioCaptureSystemData*>(inUserData);
  if (data) { data->HandleCallback(); }
}

AudioCaptureSystem::AudioCaptureSystem(uint32_t samplesPerChunk, uint32_t sampleRate)
: _samplesPerChunk(samplesPerChunk)
, _sampleRate_hz(sampleRate)
{
  // Not used on android platform
  (void) _samplesPerChunk;
  (void) _sampleRate_hz;
}

void AudioCaptureSystem::Init()
{
  if (_impl || GetPermissionState() != PermissionState::Granted)
  {
    return;
  }

  _impl.reset(new AudioCaptureSystemData{});
  _impl->_captureSystem = this;

  using SetupStep = std::pair<std::string, std::function<SLuint32()>>;
  std::vector<SetupStep> setupSteps;

  // create engine
  setupSteps.emplace_back("create engine", [this] ()
  {
    return slCreateEngine(&(_impl->_engineObject), 0, NULL, 0, NULL, NULL);
  });

  // realize the engine
  setupSteps.emplace_back("realize engine", [this] ()
  {
    return (*_impl->_engineObject)->Realize(_impl->_engineObject, SL_BOOLEAN_FALSE);
  });

  // get the engine interface, which is needed in order to create other objects
  setupSteps.emplace_back("get engine interface", [this] ()
  {
    return (*_impl->_engineObject)->GetInterface(_impl->_engineObject, SL_IID_ENGINE, &(_impl->_engineEngine));
  });

  // create audio recorder
  setupSteps.emplace_back("create audio recorder", [this] ()
  {
    // configure audio source
    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
    SLDataSource audioSrc = {&loc_dev, NULL};

    // configure audio sink
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    SLDataFormat_PCM format_pcm = {
      SL_DATAFORMAT_PCM, 1, SL_SAMPLINGRATE_16,
      SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
      SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN
    };
    SLDataSink audioSnk = {&loc_bq, &format_pcm};

    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};

    return (*_impl->_engineEngine)->CreateAudioRecorder(_impl->_engineEngine, &(_impl->_recorderObject), &audioSrc, &audioSnk, 1, id, req);
  });

  // realize the audio recorder
  setupSteps.emplace_back("realize audio recorder", [this] ()
  {
    SLresult result = (*_impl->_recorderObject)->Realize(_impl->_recorderObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_CONTENT_UNSUPPORTED == result)
    {
      AUDIOCAPTURE_ERROR("Realize() recorder object resulted in SL_RESULT_CONTENT_UNSUPPORTED. Has android.permission.RECORD_AUDIO been requested?\n");
    }
    return result;
  });

  // get the record interface
  setupSteps.emplace_back("get recorder interface", [this] ()
  {
    return (*_impl->_recorderObject)->GetInterface(_impl->_recorderObject, SL_IID_RECORD, &(_impl->_recorderRecord));
  });

  // get the buffer queue interface
  setupSteps.emplace_back("get bufferqueue interface", [this] ()
  {
    return (*_impl->_recorderObject)->GetInterface(_impl->_recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &(_impl->_recorderBufferQueue));
  });

  // register callback on the buffer queue
  setupSteps.emplace_back("register callback", [this] ()
  {
    return (*_impl->_recorderBufferQueue)->RegisterCallback(_impl->_recorderBufferQueue, HandleCallbackEntry, _impl.get());
  });

  // Go through all the steps. If one fails, cleanup and abandon setup (this replaces old goto functionality)
  for (auto& step : setupSteps)
  {
    SLresult result = step.second();
    if (SL_RESULT_SUCCESS != result)
    {
      AUDIOCAPTURE_ERROR("ERROR: Problem initializing Android mic capture on step: %s. Code: %d\n", step.first.c_str(), result);
      _impl.reset();
      break;
    }
  }
}

AudioCaptureSystem::PermissionState AudioCaptureSystem::GetPermissionState(bool isRepeatRequest) const
{
  auto envWrapper = Util::JNIUtils::getJNIEnvWrapper();
  JNIEnv* env = envWrapper->GetEnv();

  if (nullptr == env)
  {
    PRINT_NAMED_ERROR("AudioCaptureSystem.GetPermissionState.EnvNotFound",
                      "Unable to find JNIEnv variable.");
    return PermissionState::DeniedAllowRetry;
  }

  Util::JClassHandle captureClass{envWrapper->FindClassInProject("com/anki/audioUtil/AudioCaptureSystem"), env};
  if (nullptr == captureClass)
  {
    PRINT_NAMED_ERROR("AudioCaptureSystem.GetPermissionState.ClassNotFound",
                      "Unable to find com.anki.audioUtil.AudioCaptureSystem");
    return PermissionState::DeniedAllowRetry;
  }

  // get method for checking audio capture permission status
  jmethodID hasPermissionMethodID = env->GetStaticMethodID(captureClass.get(), "hasCapturePermission", "()Z");
  jboolean permissionGranted = env->CallStaticBooleanMethod(captureClass.get(), hasPermissionMethodID);

  if (!permissionGranted)
  {
    jmethodID showRationaleMethodID = env->GetStaticMethodID(captureClass.get(), "shouldShowRationale", "()Z");
    jboolean shouldShowRationale = env->CallStaticBooleanMethod(captureClass.get(), showRationaleMethodID);

    // If we were denied permission we can check whether the OS says we should give a rationale.
    // If the OS says don't bother with the rationale and we have requested permission sometime before,
    // we know the user has selected "don't show again", and won't be getting more prompts to allow mic access.
    // That means we need to tell them to go to their settings to enable the access.
    if (!shouldShowRationale && isRepeatRequest)
    {
      return PermissionState::DeniedNoRetry;
    }

    return PermissionState::DeniedAllowRetry;
  }

  return PermissionState::Granted;
}

void AudioCaptureSystem::RequestCapturePermission(RequestCapturePermissionCallback resultCallback) const
{
  auto envWrapper = Util::JNIUtils::getJNIEnvWrapper();
  JNIEnv* env = envWrapper->GetEnv();

  if (nullptr == env)
  {
    PRINT_NAMED_ERROR("AudioCaptureSystem.RequestCapturePermission.EnvNotFound",
                      "Unable to find JNIEnv variable.");
    return;
  }

  Util::JClassHandle captureClass{envWrapper->FindClassInProject("com/anki/audioUtil/AudioCaptureSystem"), env};
  if (nullptr == captureClass)
  {
    PRINT_NAMED_ERROR("AudioCaptureSystem.RequestCapturePermission.ClassNotFound",
                      "Unable to find com.anki.audioUtil.AudioCaptureSystem");
    return;
  }

  // Only lock while updating the capture callback
  {
    std::lock_guard<std::mutex> lock(_requestPermCallbackLock);
    _requestCapturePermissionCallback = std::move(resultCallback);
  }

  // get method for requesting audio capture permission status and call it
  jmethodID methodID = env->GetStaticMethodID(captureClass.get(), "requestCapturePermission", "()V");
  env->CallStaticVoidMethod(captureClass.get(), methodID);
}

AudioCaptureSystem::~AudioCaptureSystem()
{
  StopRecording();
}

void AudioCaptureSystem::StartRecording()
{
  if (_impl && !_impl->_recording)
  {
    _impl->_recording = true;

    // Put us into the recording state
    (*_impl->_recorderRecord)->SetRecordState(_impl->_recorderRecord, SL_RECORDSTATE_RECORDING);
    _impl->_enqueuedBuffer = 0;
    (*_impl->_recorderBufferQueue)->Enqueue(_impl->_recorderBufferQueue, _impl->_inputBuffers[_impl->_enqueuedBuffer].data(), kBytesPerChunk);
  }
}

void AudioCaptureSystem::StopRecording()
{
  if (_impl && _impl->_recording)
  {
    _impl->_recording = false;

    // Do recording teardown and stop
    SLuint32 state = SL_PLAYSTATE_PLAYING;
    (*_impl->_recorderRecord)->SetRecordState(_impl->_recorderRecord, SL_RECORDSTATE_STOPPED);
    while(state != SL_RECORDSTATE_STOPPED)
    {
      (*_impl->_recorderRecord)->GetRecordState(_impl->_recorderRecord, &state);
    }
  }
}

} // end namespace AudioUtil
} // end namespace Anki

extern "C"
{

void JNICALL
Java_com_anki_audioUtil_AudioCaptureSystem_NativeRequestCapturePermissionCallback(JNIEnv* env, jobject clazz, jboolean permissionGranted)
{
  std::lock_guard<std::mutex> lock(_requestPermCallbackLock);
  if (_requestCapturePermissionCallback)
  {
    _requestCapturePermissionCallback();
    _requestCapturePermissionCallback = Anki::AudioUtil::AudioCaptureSystem::RequestCapturePermissionCallback{};
  }
}

} // extern "C"
