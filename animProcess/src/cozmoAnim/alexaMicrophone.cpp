

#include "cozmoAnim/alexaMicrophone.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/logging/logging.h"
#include "util/console/consoleInterface.h"

#include <cstring>
#include <string>
#include <array>

//#include <rapidjson/document.h>

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include <fcntl.h>
#include <unistd.h>


namespace Anki {
namespace Vector {
  
  namespace {
    CONSOLE_VAR(bool, kWriteMicDataFile, "AAAA", false);
  }

using namespace alexaClientSDK;

using avsCommon::avs::AudioInputStream;

//static const int NUM_INPUT_CHANNELS = 1;
//static const int NUM_OUTPUT_CHANNELS = 0;
//static const double SAMPLE_RATE = 16000;
//static const unsigned long PREFERRED_SAMPLES_PER_CALLBACK = paFramesPerBufferUnspecified;

//static const std::string SAMPLE_APP_CONFIG_ROOT_KEY("sampleApp");
//static const std::string PORTAUDIO_CONFIG_ROOT_KEY("portAudio");
//static const std::string PORTAUDIO_CONFIG_SUGGESTED_LATENCY_KEY("suggestedLatency");

/// String to identify log entries originating from this file.
static const std::string TAG("AlexaMicrophoneWrapper");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<AlexaMicrophone> AlexaMicrophone::create(
                                                                               std::shared_ptr<AudioInputStream> stream) {
  if (!stream) {
    ACSDK_CRITICAL(LX("Invalid stream passed to AlexaMicrophone"));
    return nullptr;
  }
  std::unique_ptr<AlexaMicrophone> alexaMicrophone(new AlexaMicrophone(stream));
  if (!alexaMicrophone->initialize()) {
    ACSDK_CRITICAL(LX("Failed to initialize AlexaMicrophone"));
    return nullptr;
  }
  return alexaMicrophone;
}

AlexaMicrophone::AlexaMicrophone(std::shared_ptr<AudioInputStream> stream)
  : m_audioInputStream{stream}
{
}

//AlexaMicrophone::~AlexaMicrophone() {
//  Pa_StopStream(m_paStream);
//  Pa_CloseStream(m_paStream);
//  Pa_Terminate();
//}

bool AlexaMicrophone::initialize() {
  m_writer = m_audioInputStream->createWriter(AudioInputStream::Writer::Policy::NONBLOCKABLE);
  if (!m_writer) {
    ACSDK_CRITICAL(LX("Failed to create stream writer"));
    return false;
  }
  return true;
}

bool AlexaMicrophone::startStreamingMicrophoneData() {
  std::lock_guard<std::mutex> lock{m_mutex};
  _streaming = true;
//  PaError err = Pa_StartStream(m_paStream);
//  if (err != paNoError) {
//    ACSDK_CRITICAL(LX("Failed to start PortAudio stream"));
//    return false;
//  }
  return true;
}

bool AlexaMicrophone::stopStreamingMicrophoneData() {
  std::lock_guard<std::mutex> lock{m_mutex};
  _streaming = false;
//  PaError err = Pa_StopStream(m_paStream);
//  if (err != paNoError) {
//    ACSDK_CRITICAL(LX("Failed to stop PortAudio stream"));
//    return false;
//  }
  return true;
}

//int AlexaMicrophone::PortAudioCallback(
//                                                  const void* inputBuffer,
//                                                  void* outputBuffer,
//                                                  unsigned long numSamples,
//                                                  const PaStreamCallbackTimeInfo* timeInfo,
//                                                  PaStreamCallbackFlags statusFlags,
//                                                  void* userData) {
//  AlexaMicrophone* wrapper = static_cast<AlexaMicrophone*>(userData);
//  ssize_t returnCode = wrapper->m_writer->write(inputBuffer, numSamples);
//  if (returnCode <= 0) {
//    ACSDK_CRITICAL(LX("Failed to write to stream."));
//    return paAbort;
//  }
//  return paContinue;
//}

//bool AlexaMicrophone::getConfigSuggestedLatency(PaTime& suggestedLatency) {
//  bool latencyInConfig = false;
//  auto config = avsCommon::utils::configuration::ConfigurationNode::getRoot()[SAMPLE_APP_CONFIG_ROOT_KEY]
//  [PORTAUDIO_CONFIG_ROOT_KEY];
//  if (config) {
//    latencyInConfig = config.getValue(
//                                      PORTAUDIO_CONFIG_SUGGESTED_LATENCY_KEY,
//                                      &suggestedLatency,
//                                      suggestedLatency,
//                                      &rapidjson::Value::IsDouble,
//                                      &rapidjson::Value::GetDouble);
//  }
//
//  return latencyInConfig;
//}

void AlexaMicrophone::ProcessAudio(int16_t* data, size_t size)
{
  if( !_streaming || (m_writer == nullptr) ) {
    return;
  }
  
//  static constexpr uint32_t kNumInputChannels               = 4;
//  static constexpr uint32_t kSamplesPerChunkIncoming        = 80;
//  //static constexpr uint32_t kSampleRateIncoming_hz          = 16000;
//  //static constexpr uint32_t kTimePerChunk_ms                = 5;
//  static constexpr uint32_t kChunksPerSEBlock               = 2;
//  static constexpr uint32_t kSamplesPerBlock                = kSamplesPerChunkIncoming * kChunksPerSEBlock;
//  static std::array<int16_t, kSamplesPerBlock * kNumInputChannels> inProcessAudioBlock;
//
//  {
//    // Uninterleave the chunks when copying out of the payload, since that's what SE wants
//    for (uint32_t sampleIdx = 0; sampleIdx < kSamplesPerChunkIncoming; ++sampleIdx)
//    {
//      const uint32_t interleaveBase = (sampleIdx * kNumInputChannels);
//      uint32_t channelIdx = 0;
//      //for (uint32_t channelIdx = 0; channelIdx < kNumInputChannels; ++channelIdx)
//      //{
//        //uint32_t dataOffset = _inProcessAudioBlockFirstHalf ? 0 : kSamplesPerChunkIncoming;
//        //const uint32_t uninterleaveBase = (channelIdx * kSamplesPerBlock);// + dataOffset;
//        //int16_t data[320];
//        inProcessAudioBlock[sampleIdx /*+ uninterleaveBase*/] = payload.data[channelIdx + interleaveBase];
//      //}
//    }
//  }
//
//  int numSamples = kSamplesPerChunkIncoming; //kSamplesPerBlock;
//, "wrote %d samples", (int)numSamples);
//
//  m_writer->write( inProcessAudioBlock.data(), numSamples );
  int numSamples = size;
  _totalSamples += size;
  m_writer->write( data, size );
  
  static int _fd = -1;
  if (kWriteMicDataFile) {
    const auto samples = data;//inProcessAudioBlock.data();
    
    if (_fd < 0) {
      const auto path = "/data/data/com.anki.victor/cache/tts.pcm";
      _fd = open(path, O_CREAT|O_RDWR|O_TRUNC, 0644);
    }
    (void) write(_fd, samples, numSamples * sizeof(short));
  } else if( _fd != -1 ) {
    close(_fd);
    _fd = -1;
  }
  
  
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
