/**
* File: audioDataTypes.mm
*
* Author: Lee Crippen
* Created: 12/14/2016
*
* Description: iOS and OSX implementation of interface.
*
* Copyright: Anki, Inc. 2016
*
*/

#include "audioDataTypes.h"

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>

namespace Anki {
namespace AudioUtil {
  
static AudioStreamBasicDescription CreateStandardDescription()
{
  AudioStreamBasicDescription format{};
  format.mSampleRate = kSampleRate_hz;
  format.mFormatID = kAudioFormatLinearPCM;
  format.mFramesPerPacket = 1; //For uncompressed audio, the value is 1. For variable bit-rate formats, the value is a larger fixed number, such as 1024 for AAC
  format.mChannelsPerFrame = 1;
  format.mBytesPerFrame = format.mChannelsPerFrame * 2;
  format.mBytesPerPacket = format.mFramesPerPacket * format.mBytesPerFrame;
  format.mBitsPerChannel = 16;
  format.mReserved = 0;
  format.mFormatFlags =  kAudioFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
  return format;
}

void GetStandardAudioDescriptionFormat(AudioStreamBasicDescription& description_out)
{
  static AudioStreamBasicDescription referenceDescription = CreateStandardDescription();
  description_out = referenceDescription;
}


} // end namespace AudioUtil
} // end namespace Anki
