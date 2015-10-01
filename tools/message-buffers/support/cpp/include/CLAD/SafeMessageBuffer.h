/**
 * File: SafeMessageBuffer.h
 *
 * Author: wesley
 * Created: 2/10/2015
 *
 * Description: Utility for safe message serialization and deserialization.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef CLAD_SAFEMESSAGEBUFFER_H_
#define CLAD_SAFEMESSAGEBUFFER_H_

#ifndef __cplusplus
#error "SafeMessageBuffer is a C++ only header!"
#endif

#include <cstdint>
#include <vector>
#include <string>

namespace CLAD
{

class __attribute__((visibility("default"))) SafeMessageBuffer
{
public:
  
  // ========== Constructors / Destructors ==========
  
  SafeMessageBuffer();
  explicit SafeMessageBuffer(size_t inSize);
  SafeMessageBuffer(uint8_t* inBuffer, size_t inBufferSize, bool inOwnsBufferMemory = false);
  
  ~SafeMessageBuffer();

  // ========== Buffer Management ==========
  
  void AllocateBuffer(size_t inBufferSize);
  void SetBuffer(uint8_t* inBuffer, size_t inBufferSize, bool inOwnsBufferMemory = false);
  void ReleaseBuffer();

  size_t  GetBytesWritten() const;
  size_t  GetBytesRead() const;
  size_t  CopyBytesOut(uint8_t* outBuffer, size_t bufferSize) const;
  
  void Clear();

  // ========== Write methods ==========

  bool WriteBytes(const void* srcData, size_t sizeOfWrite);

  template <typename T>
  bool Write(T inVal)
  {
    return WriteBytes(&inVal, sizeof(inVal));
  }
  
  template <typename T>
  bool Write( const std::vector<T>& outVec)
  {
    for(const T& val : outVec) {
      if(!Write(val)) {
        return false;
      }
    }
    return true;
  }
  
  template <typename val_t, size_t length>
  bool WriteFArray(const std::array<val_t, length>& outArray)
  {
    for(const val_t& val : outArray) {
      if(!Write(val)) {
        return false;
      }
    }
    return true;
  }
  
  template <typename val_t, typename length_t>
  bool WriteVArray(val_t* arrayPtr, const length_t numElements)
  {
    bool wroteOK = Write(numElements);
    if (wroteOK && (numElements != 0))
    {
      wroteOK |= WriteBytes(arrayPtr, sizeof(val_t) * numElements);
    }
    return wroteOK;
  }

  template <typename val_t, typename length_t>
  bool WriteVArray( const std::vector<val_t>& outVec )
  {
    if(!Write(static_cast<length_t>(outVec.size()))) {
      return false;
    }
    if(!Write(outVec)) {
      return false;
    }
    return true;
  }
  
  template <typename length_t>
  bool WritePString( const std::string& str )
  {
    if(!Write(static_cast<length_t>(str.length()))) {
      return false;
    }
    if(str.length() > 0) {
      if(!WriteBytes(str.data(), str.length())) {
        return false;
      }
    }
    return true;
  }

  template <typename array_length_t, typename string_length_t>
  bool WritePStringVArray( const std::vector<std::string>& outVec )
  {
    if(!Write(static_cast<array_length_t>(outVec.size()))) {
      return false;
    }
    for (const std::string& str : outVec) {
      if(!WritePString<string_length_t>(str)) {
        return false;
      }
    }
    return true;
  }

  template <size_t length, typename string_length_t>
  bool WritePStringFArray(const std::array<std::string, length>& outArray)
  {
    for(const std::string& val : outArray) {
      if(!WritePString<string_length_t>(val)) {
        return false;
      }
    }
    return true;
  }

  // ========== Read methods ==========

  bool ReadBytes(void* destData, size_t sizeOfRead) const;
  
  template <typename T>
  bool Read(T& outVal) const
  {
    return ReadBytes(&outVal, sizeof(outVal));
  }
  
  template <typename val_t>
  bool Read( std::vector<val_t>& inVec, const size_t num) const
  {
    inVec.clear();
    inVec.reserve(num);
    val_t val;
    for(size_t i = 0; i < num; i++) {
      if(!Read(val)) {
        return false;
      }
      inVec.push_back(val);
    }
    return inVec.size() == num;
  }

  template <typename val_t, size_t length>
  bool ReadFArray( std::array<val_t, length>& inArray ) const
  {
    val_t val;
    for(size_t i = 0; i < length; i++) {
      if(!Read(val)) {
        return false;
      }
      inArray[i] = val;
    }
    return true;
  }
  
  template <typename val_t, typename length_t>
  bool ReadVArray( std::vector<val_t>& inVec ) const
  {
    length_t length;
    if(!Read(length)) {
      return false;
    }
    if(!Read(inVec, static_cast<size_t>(length))) {
      return false;
    }
    return true;
  }
  
  template <typename length_t>
  bool ReadPString( std::string& inStr ) const
  {
    length_t length;
    if(!Read(length)) {
      return false;
    }
    if(!ReadString(inStr, static_cast<size_t>(length))) {
      return false;
    }
    return true;
  }
  
  bool ReadString( std::string& inStr, size_t length) const
  {
    inStr.clear();
    if(length > 0) {
      inStr.reserve(length);
      // This is probably really slow!
      // TODO: figure out how to more quickly unpack a string
      std::string::value_type val;
      for(size_t i = 0; i < length; i++) {
        if(!Read(val)) {
          return false;
        }
        inStr.push_back(val);
      }
    }
    return inStr.length() == length;
  }

  template <typename array_length_t, typename string_length_t>
  bool ReadPStringVArray( std::vector<std::string>& inVec ) const
  {
    array_length_t length;
    if(!Read(length)) {
      return false;
    }
    size_t num = static_cast<size_t>(length);
    inVec.clear();
    inVec.reserve(num);
    std::string val;
    for(size_t i = 0; i < num; i++) {
      if(!ReadPString<string_length_t>(val)) {
        return false;
      }
      inVec.push_back(val);
    }
    return inVec.size() == num;
  }

  template <size_t length, typename string_length_t>
  bool ReadPStringFArray( std::array<std::string, length>& inArray ) const
  {
    std::string val;
    for(size_t i = 0; i < length; i++) {
      if(!ReadPString<string_length_t>(val)) {
        return false;
      }
      inArray[i] = val;
    }
    return true;
  }

  template <typename val_t>
  bool ReadCompoundTypeVec( std::vector<val_t>& inVec, const size_t num) const
  {
    inVec.clear();
    inVec.reserve(num);
    val_t val;
    for(size_t i = 0; i < num; i++) {
      if(!val.Unpack(*this)) {
        return false;
      }
      inVec.push_back(val);
    }
    return inVec.size() == num;
  }

  template <typename val_t, typename length_t>
  bool ReadCompoundTypeVArray( std::vector<val_t>& inVec ) const
  {
    length_t length;
    if(!Read(length)) {
      return false;
    }
    if(!ReadCompoundTypeVec(inVec, static_cast<size_t>(length))) {
      return false;
    }
    return true;
  }

  template <typename val_t, size_t length>
  bool ReadCompoundTypeFArray( std::array<val_t, length>& inArray ) const
  {
    val_t val;
    for(size_t i = 0; i < length; i++) {
      if(!val.Unpack(*this)) {
        return false;
      }
      inArray[i] = val;
    }
    return true;
  }

private:
  
  SafeMessageBuffer(const SafeMessageBuffer&);
  SafeMessageBuffer& operator=(const SafeMessageBuffer&);

  // ========== Member Data ==========

  uint8_t*          _buffer;
  size_t            _bufferSize;
  
  uint8_t*          _writeHead;
  mutable uint8_t*  _readHead;

  bool              _ownsBufferMemory;
};


template<>
bool SafeMessageBuffer::Write(bool inVal);

template<>
bool SafeMessageBuffer::Read(bool& outVal) const;


}// CLAD

#endif // SAFEMESSAGEBUFFER_H_
