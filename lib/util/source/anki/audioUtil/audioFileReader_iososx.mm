/**
* File: audioFileReader.mm
*
* Author: Lee Crippen
* Created: 11/22/16
*
* Description: iOS and OSX implementation of interface.
*
* Copyright: Anki, Inc. 2016
*
*/

#include "audioUtil/audioFileReader.h"

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>

#include <iostream>
#include <thread>

namespace Anki {
namespace AudioUtil {

struct ConvertStruct
{
  ConvertStruct(AudioFileID audioFile)
  : _audioFileID(audioFile)
  {
    UInt32 maxPacketSize = 0;
    UInt32 dataSizeToWrite = sizeof(maxPacketSize);
    OSStatus errorCode = AudioFileGetProperty(audioFile, kAudioFilePropertyPacketSizeUpperBound, &dataSizeToWrite, &maxPacketSize);
    if (errorCode)
    {
      NSError *error = [NSError errorWithDomain:NSOSStatusErrorDomain code:errorCode userInfo:nil];
      std::cerr << [[error localizedDescription] UTF8String] << std::endl;
      return;
    }
    
    _maxPacketSize = maxPacketSize;
  }
  
  AudioFileID                                 _audioFileID{};
  UInt32                                      _maxPacketSize{};
  std::vector<uint8_t>                        _packetDataBuffer;
  std::vector<AudioStreamPacketDescription>   _packetDescBuffer;
  SInt64                                      _packetIndex{};
};

// This callback is responsible pulling a packet (or packets) of (potentially compressed) audio out of a file into
// _packetDataBuffer and _packetDescBuffer, storing the pointers to that data for processing, and holding onto that data
// until this callback is called again.
static OSStatus ConvertAudioDataCallback (AudioConverterRef               inAudioConverter,
                                          UInt32 *                        ioNumberDataPackets,
                                          AudioBufferList *               ioData,
                                          AudioStreamPacketDescription * __nullable * __nullable outDataPacketDescription,
                                          void * __nullable               inUserData)
{
  ConvertStruct* convertData = static_cast<ConvertStruct*>(inUserData);
  
  // Make sure the vector in our struct is sized right for the packet data we'll put into it
  UInt32 numPacketsToPull = *ioNumberDataPackets;
  convertData->_packetDataBuffer.resize(numPacketsToPull * convertData->_maxPacketSize);
  convertData->_packetDescBuffer.resize(numPacketsToPull);
  
  // Now actually read the data into our buffer
  UInt32 numBytesToRead = static_cast<UInt32>(convertData->_packetDataBuffer.size());
  OSStatus errorCode = AudioFileReadPacketData(convertData->_audioFileID, false, &numBytesToRead, convertData->_packetDescBuffer.data(), convertData->_packetIndex, &numPacketsToPull, convertData->_packetDataBuffer.data());
  if (errorCode)
  {
    NSError *error = [NSError errorWithDomain:NSOSStatusErrorDomain code:errorCode userInfo:nil];
    std::cerr << [[error localizedDescription] UTF8String] << std::endl;
    return errorCode;
  }
  
  // Output the number of packets successfully read
  *ioNumberDataPackets = numPacketsToPull;
  if (numPacketsToPull == 0)
  {
    return kAudioFileEndOfFileError;
  }
  
  // If we did pull packet data, fill out the pointers and size of data for the output struct
  ioData->mBuffers[0].mData = convertData->_packetDataBuffer.data();
  ioData->mBuffers[0].mDataByteSize = numBytesToRead;
  
  if (outDataPacketDescription)
  {
    *outDataPacketDescription = convertData->_packetDescBuffer.data();
  }
  
  convertData->_packetIndex += numPacketsToPull;
  
  // No errors
  return 0;
}

struct AudioFileReader::NativeAudioFileData
{
  AudioFileID audioFileID{};
  AudioStreamBasicDescription basicDescription{};
};

bool AudioFileReader::ReadFile(const std::string& audioFilePath)
{
  ClearAudio();
  
  bool success = false;
  
  @autoreleasepool
  {
    NativeAudioFileData fileData;
    auto doReadFile = [this, &fileData, &audioFilePath] () -> bool
    {
      if (!GetNativeFileData(fileData, audioFilePath))
      {
        return false;
      }
      
      if (!ConvertAndStoreSamples(fileData))
      {
        return false;
      }
      
      if (!TrimPrimingAndRemainder(fileData))
      {
        return false;
      }
      
      return true;
    };
    
    success = doReadFile();
    
    if (fileData.audioFileID)
    {
      AudioFileClose(fileData.audioFileID);
    }
  }
  
  return success;
}

bool AudioFileReader::GetNativeFileData(NativeAudioFileData& fileData, const std::string& audioFilePath)
{
  CFURLRef fileUrl = (__bridge CFURLRef)[NSURL fileURLWithPath:[NSString stringWithUTF8String:audioFilePath.c_str()] isDirectory:false];
  OSStatus errorCode = AudioFileOpenURL(fileUrl, kAudioFileReadPermission, 0, &fileData.audioFileID);
  if (errorCode)
  {
    NSError *error = [NSError errorWithDomain:NSOSStatusErrorDomain code:errorCode userInfo:nil];
    std::cerr << [[error localizedDescription] UTF8String] << std::endl;
    return false;
  }
  
  UInt32 dataSizeToWrite = sizeof(fileData.basicDescription);
  errorCode = AudioFileGetProperty(fileData.audioFileID, kAudioFilePropertyDataFormat, &dataSizeToWrite, &fileData.basicDescription);
  if (errorCode)
  {
    NSError *error = [NSError errorWithDomain:NSOSStatusErrorDomain code:errorCode userInfo:nil];
    std::cerr << [[error localizedDescription] UTF8String] << std::endl;
    return false;
  }
  
  return true;
}

bool AudioFileReader::ConvertAndStoreSamples(const NativeAudioFileData& fileData)
{
  // Set up audio converter object
  AudioStreamBasicDescription destinationDesc;
  GetStandardAudioDescriptionFormat(destinationDesc);
  AudioConverterRef audioConverter;
  OSStatus errorCode = AudioConverterNew(&fileData.basicDescription, &destinationDesc, &audioConverter);
  if (errorCode)
  {
    NSError *error = [NSError errorWithDomain:NSOSStatusErrorDomain code:errorCode userInfo:nil];
    std::cerr << [[error localizedDescription] UTF8String] << std::endl;
    return false;
  }
  
  AudioBufferList bufferList{};
  AudioChunk bufferData{};
  
  // Lambda for resetting the state of the buffer data. Note that by calling clear() and resize(), a new chunk of data memory
  // gets allocated if the buffer had just been moved() into the _audioSamples list for storage.
  auto resetBufferData = [samplesPerChunk = _samplesPerChunk, &bufferList, &bufferData]()
  {
    bufferData.clear();
    bufferData.resize(samplesPerChunk);
    bufferList.mBuffers[0].mData = bufferData.data();
    bufferList.mBuffers[0].mDataByteSize = samplesPerChunk * sizeof(bufferData[0]);
    bufferList.mBuffers[0].mNumberChannels = 1;
    bufferList.mNumberBuffers = 1;
  };
  
  // Call it once to init the structures
  resetBufferData();
  
  ConvertStruct convertData{ fileData.audioFileID };
  
  UInt32 numPacketsToConvert = _samplesPerChunk; // 1:1 ratio here since uncompressed audio is 1 frame per packet, and each frame is just a single channel audio sample
  while (true)
  {
    numPacketsToConvert = _samplesPerChunk;
    errorCode = AudioConverterFillComplexBuffer(audioConverter, &ConvertAudioDataCallback, (void*)&convertData, &numPacketsToConvert, &bufferList, NULL);
    if (numPacketsToConvert > 0)
    {
      bufferData.resize(numPacketsToConvert);
      _audioSamples.push_back(std::move(bufferData));
      resetBufferData();
    }
    
    if (errorCode)
    {
      if (errorCode != kAudioFileEndOfFileError)
      {
        NSError *error = [NSError errorWithDomain:NSOSStatusErrorDomain code:errorCode userInfo:nil];
        std::cerr << [[error localizedDescription] UTF8String] << std::endl;
      }
      break;
    }
  }
  
  // Get rid of the audio converter now that we're done with it
  AudioConverterDispose(audioConverter);
  
  return true;
}

bool AudioFileReader::TrimPrimingAndRemainder(const NativeAudioFileData& fileData)
{
  AudioFilePacketTableInfo packetTableInfo{};
  UInt32 dataSizeToWrite = sizeof(packetTableInfo);
  OSStatus errorCode = AudioFileGetProperty(fileData.audioFileID, kAudioFilePropertyPacketTableInfo, &dataSizeToWrite, &packetTableInfo);
  if (errorCode && errorCode != kAudioFileUnsupportedPropertyError)
  {
    NSError *error = [NSError errorWithDomain:NSOSStatusErrorDomain code:errorCode userInfo:nil];
    std::cerr << [[error localizedDescription] UTF8String] << std::endl;
    return false;
  }
    
  // If we have priming frame data from packetInfo, cut those frames out.
  if (packetTableInfo.mNumberValidFrames == 0)
  {
    // It's valid for there to be no frame info in the packetTableInfo - this means there are no priming or remainder frames to trim
    return true;
  }
  
  Float64 sampleRateRatio = (Float64) kSampleRate_hz / fileData.basicDescription.mSampleRate;
  
  // Trim priming off the front
  {
    UInt32 numPriming = static_cast<UInt32>(sampleRateRatio * static_cast<Float64>(packetTableInfo.mPrimingFrames));
    while (numPriming >= _audioSamples.front().size())
    {
      const auto frontSize = _audioSamples.front().size();
      _audioSamples.pop_front();
      numPriming -= frontSize;
    }
    if (numPriming > 0)
    {
      auto& frontRef = _audioSamples.front();
      frontRef.erase(frontRef.begin(), frontRef.begin() + numPriming);
    }
  }
  
  // Trim remainder off the back
  {
    UInt32 numRemainder = static_cast<UInt32>(sampleRateRatio * static_cast<Float64>(packetTableInfo.mRemainderFrames));
    while (numRemainder >= _audioSamples.back().size())
    {
      const auto backSize = _audioSamples.back().size();
      _audioSamples.pop_back();
      numRemainder -= backSize;
    }
    if (numRemainder > 0)
    {
      auto& backRef = _audioSamples.back();
      backRef.erase(backRef.end() - numRemainder, backRef.end());
    }
  }
  return true;
}

void AudioFileReader::DeliverAudio(bool doRealTime, bool addBeginSilence, bool addEndSilence)
{
  auto dataCallback = GetDataCallback();
  if (!dataCallback)
  {
    return;
  }
  
  // Lambda for delivering a single audio chunk
  auto deliverAudioChunk = [&dataCallback, doRealTime] (const AudioChunk& audioChunk)
  {
    const auto updatePeriod = std::chrono::milliseconds((1000 * audioChunk.size()) / kSampleRate_hz);
    std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
    dataCallback(audioChunk.data(), static_cast<uint32_t>(audioChunk.size()));
    auto elapsed = std::chrono::steady_clock::now() - startTime;
    if (doRealTime && elapsed < updatePeriod)
    {
      std::this_thread::sleep_for(updatePeriod - elapsed);
    }
  };
  
  // Static AudioChunk to be used for adding silence
  static const AudioChunk silenceChunk = AudioChunk(std::size_t(_samplesPerChunk), 0);
  const int chunksPerSecond = AudioUtil::kSampleRate_hz / _samplesPerChunk;
  
  // Deliver beginning silence if desired
  if (addBeginSilence)
  {
    for (int i=0; i<chunksPerSecond; ++i)
    {
      deliverAudioChunk(silenceChunk);
    }
  }
  
  // Deliver the stored audio samples
  for (const auto& audioChunk : _audioSamples)
  {
    deliverAudioChunk(audioChunk);
  }
  
  // Deliver ending silence if desired
  if (addEndSilence)
  {
    for (int i=0; i<chunksPerSecond; ++i)
    {
      deliverAudioChunk(silenceChunk);
    }
  }
}

} // namespace AudioUtil
} // namespace Anki
