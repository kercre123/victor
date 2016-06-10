/**
 * File: messageDebugging
 *
 * Author: Mark Wesley
 * Created: 02/03/15
 *
 * Description: Debug helpers for debugging and logging message data
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#include "util/debug/messageDebugging.h"
#include "util/console/consoleInterface.h"
#include "util/transport/srcBufferSet.h"
#include <assert.h>
#include <stdio.h>



namespace Anki {
namespace Util {

  
CONSOLE_VAR(bool, kEnableVerboseNetworkLogging, "Network", false);
  
  
char NibbleToHexChar(uint32_t inVal)
{
  assert(inVal < 16);
  return "0123456789ABCDEF"[inVal];
}


std::string ConvertMessageBufferToString(const uint8_t* buffer, uint32_t bufferLength, EBytesToTextType printType,
                                         bool addLeadingSpace, uint32_t maxStringLength)
{
  // Calculate if we need to truncate the buffer size printed to fit within maxStringLength
 
  const char* kTruncatedSuffix = "...";
  const bool showHexForUnprintable = (printType == eBTTT_Ascii);
  
  const uint32_t lettersPerByte = (addLeadingSpace || (printType == eBTTT_Decimal)) ? 3 : 2;
  const uint32_t fullStringLen  = lettersPerByte * bufferLength;
  
  const bool isOutputTruncated = (fullStringLen > maxStringLength);
  const uint32_t clampedStringLen = isOutputTruncated ? maxStringLength : fullStringLen;
  
  uint32_t clampedBufferLen = bufferLength;
  if (isOutputTruncated)
  {
    assert(fullStringLen > clampedStringLen);
    const uint32_t extraLettersToSkip = (uint32_t)strlen(kTruncatedSuffix); // need to remove additional bytes to make room for suffix
    const uint32_t numLettersToSkip = (fullStringLen-clampedStringLen) + extraLettersToSkip;
    const uint32_t numBytesToSkip = (numLettersToSkip + (lettersPerByte-1)) / lettersPerByte;
    clampedBufferLen = (bufferLength > numBytesToSkip) ? (bufferLength - numBytesToSkip) : 0;
  }
  
  std::string msgString;
  msgString.reserve(clampedStringLen);
  
  char byteString[4] = {0};
  
  for(uint32_t i=0; i < clampedBufferLen; ++i)
  {
    uint32_t msgChar = buffer[i];
    
    const bool isPrintable = isprint(msgChar);
    const EBytesToTextType printTypeForChar = (showHexForUnprintable && !isPrintable) ? eBTTT_Hex : printType;
    
    switch(printTypeForChar)
    {
      case eBTTT_Ascii:
      {
        int bI = 0;
        if (addLeadingSpace)
        {
          byteString[bI++] = ' ';
        }
        byteString[bI++] = isPrintable ? ' '     : '?';
        byteString[bI++] = isPrintable ? msgChar : '?';
        byteString[bI++] = 0;
        break;
      }
      case eBTTT_Hex:
      {
        int bI = 0;
        if (addLeadingSpace)
        {
          byteString[bI++] = ' ';
        }
        byteString[bI++] = NibbleToHexChar((msgChar >> 4) & 0x0000000f);
        byteString[bI++] = NibbleToHexChar((msgChar     ) & 0x0000000f);
        byteString[bI++] = 0;
        break;
      }
      case eBTTT_Decimal:
      {
        snprintf(byteString, sizeof(byteString), "%03u", msgChar );
        break;
      }
    }
    
    msgString += byteString;
  }
  
  if (isOutputTruncated)
  {
    msgString += kTruncatedSuffix;
  }
  
  return msgString;
}
  
  
void LogVerboseMessage(const char* name, const char* columnLabels, const uint8_t* buffer, uint32_t bufferLength, const SrcBufferSet* srcBuffers)
{
 #if ANKI_NET_MESSAGE_LOGGING_ENABLED
  const char* k_LineBreak = "--------------------------------------------------------------------------------\n";
  printf("%s", k_LineBreak);
  printf("   %s    %s", name, columnLabels);
  if (srcBuffers != nullptr)
  {
    for (uint32_t i=0; i < srcBuffers->GetCount(); ++i)
    {
      printf("%s", srcBuffers->GetBuffer(i).GetDescriptor());
    }
  }
  printf("\n");
  std::string asciiString = ConvertMessageBufferToString(buffer, bufferLength, eBTTT_Ascii);
  std::string hexString   = ConvertMessageBufferToString(buffer, bufferLength, eBTTT_Hex);
  assert(asciiString.size() == hexString.size());
  uint32_t numBytesPrinted = 0;
  const uint32_t charsPerLine = 32 * 3;
  int line = 0;
  while (numBytesPrinted < asciiString.size())
  {
    ++line;
    printf("%02d %s   '%s'\n", line, name, asciiString.substr(numBytesPrinted, charsPerLine).c_str());
    printf("%02d %s 0x'%s'\n", line, name, hexString.substr(numBytesPrinted, charsPerLine).c_str());
    numBytesPrinted += charsPerLine;
  }
  
  printf("%s", k_LineBreak);
 #endif // ANKI_NET_MESSAGE_LOGGING_ENABLED
}



} // end namespace Util
} // end namespace Anki

