// passes data from robot messages to alexa stream
#ifndef ANIMPROCESS_COZMO_ALEXAMICROPHONE_H
#define ANIMPROCESS_COZMO_ALEXAMICROPHONE_H



#include <memory>
#include <mutex>
#include <thread>

#include <AVSCommon/AVS/AudioInputStream.h>

//#include <portaudio.h>
#include <Audio/MicrophoneInterface.h>



namespace Anki {
namespace Vector {
  
  namespace RobotInterface {
    struct MicData;
  }

class AlexaMicrophone : public alexaClientSDK::applicationUtilities::resources::audio::MicrophoneInterface {
public:
  
  /**
   * Creates a @c PortAudioMicrophoneWrapper.
   *
   * @param stream The shared data stream to write to.
   * @return A unique_ptr to a @c PortAudioMicrophoneWrapper if creation was successful and @c nullptr otherwise.
   */
  static std::unique_ptr<AlexaMicrophone> create(std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream);
  
  void ProcessAudio(int16_t* data, size_t size);
  
  /**
   * Stops streaming from the microphone.
   *
   * @return Whether the stop was successful.
   */
  bool stopStreamingMicrophoneData() override;
  
  /**
   * Starts streaming from the microphone.
   *
   * @return Whether the start was successful.
   */
  bool startStreamingMicrophoneData() override;
  
  /**
   * Destructor.
   */
  //~PortAudioMicrophoneWrapper();
  
  size_t GetTotalSamples() const { return _totalSamples; }
  
private:
  bool _streaming = false;
  size_t _totalSamples = 0;
  /**
   * Constructor.
   *
   * @param stream The shared data stream to write to.
   */
  explicit AlexaMicrophone(std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> stream);
  
//  /**
//   * The callback that PortAudio will issue when audio is avaiable to read.
//   *
//   * @param inputBuffer The temporary buffer that microphone audio data will be available in.
//   * @param outputBuffer Not used here.
//   * @param numSamples The number of samples available to consume.
//   * @param timeInfo Time stamps indicated when the first sample in the buffer was captured. Not used here.
//   * @param statusFlags Flags that tell us when underflow or overflow conditions occur. Not used here.
//   * @param userData A user supplied pointer.
//   * @return A PortAudio code that will indicate how PortAudio should continue.
//   */
//  static int PortAudioCallback(
//                               const void* inputBuffer,
//                               void* outputBuffer,
//                               unsigned long numSamples,
//                               const PaStreamCallbackTimeInfo* timeInfo,
//                               PaStreamCallbackFlags statusFlags,
//                               void* userData);
  
  /// Initializes PortAudio
  bool initialize();
  
  /**
   * Get the optional config parameter from @c AlexaClientSDKConfig.json
   * for setting the PortAudio stream's suggested latency.
   *
   * @param[out] suggestedLatency The latency as it is configured in the file.
   * @return  @c true if the suggestedLatency is defined in the config file, @c false otherwise.
   */
  //bool getConfigSuggestedLatency(PaTime& suggestedLatency);
  
  /// The stream of audio data.
  const std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> m_audioInputStream;
  
  /// The writer that will be used to writer audio data into the sds.
  std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream::Writer> m_writer;
  
  /// The PortAudio stream
  //PaStream* m_paStream;
  
  /**
   * A lock to seralize access to startStreamingMicrophoneData() and stopStreamingMicrophoneData() between different
   * threads.
   */
  std::mutex m_mutex;
};
      
} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXAMICROPHONE_H
