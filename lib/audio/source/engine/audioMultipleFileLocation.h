//////////////////////////////////////////////////////////////////////
//
// audioMultipleFileLocation.h
//
// File location resolving: Supports multiple base paths for file access, searched in reverse order.
// For more details on resolving file location, refer to section "File Location" inside
// "Going Further > Overriding Managers > Streaming / Stream Manager > Low-Level I/O"
// of the SDK documentation. 
//
// Copyright (c) 2014 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_MULTI_FILE_LOCATION_H_
#define _AK_MULTI_FILE_LOCATION_H_

struct AkFileSystemFlags;

#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include <AK/Tools/Common/AkListBareLight.h>
#include "audioEngine/audioTypes.h"
#include "audioEngine/audioEngineConfigDefines.h"
#include <string>
#include <map>
#include <mutex>
#include "zipReader.h"
#ifdef ANDROID
#include <android/asset_manager.h>
#endif

// This file location class supports multiple base paths for Wwise file access.
// Each path will be searched the reverse order of the addition order until the file is found.
template<class OPEN_POLICY>
class AudioMultipleFileLocation
{
protected:

	// Internal user paths.
	struct FilePath
	{
		FilePath *pNextLightItem;
		AkOSChar szPath[1];	//Variable length
	};
public:
	AudioMultipleFileLocation();
	void Term();

	//
	// Global path functions.
	// ------------------------------------------------------

	// Base path is prepended to all file names.
	// Audio source path is appended to base path whenever uCompanyID is AK and uCodecID specifies an audio source.
	// Bank path is appended to base path whenever uCompanyID is AK and uCodecID specifies a sound bank.
	// Language specific dir name is appended to path whenever "bIsLanguageSpecific" is true.
	AKRESULT SetBasePath(const AkOSChar*   in_pszBasePath)
	{
		return AddBasePath(in_pszBasePath);
	}

  // Set file path resolver callback function
  void SetPathResolver(Anki::AudioEngine::AudioFilePathResolverFunc pathResolver);

  // Add Zip file paths to available search paths
  enum class SearchPathLocation { Head, Tail };
  AKRESULT AddZipFile(const std::string& path, SearchPathLocation location);

  // Remove Zip file paths from available search paths
  AKRESULT RemoveZipFile(const std::string& path);

  // Clear missing Zip file paths from available search paths
  void RemoveMissingZipFiles();

#ifdef ANDROID
  // Set Android's Asset menager
  void SetAssetManager(void* assetManager);
  // Set Android's Asset base path
  void SetAssetBasePath(const std::string& basePath);
#endif

  // Add path to available search paths
	AKRESULT AddBasePath(const AkOSChar*   in_pszBasePath);

  // Set path to write to files to
  // Note: If write directory path does not exist it will be created
  void SetWriteBasePath(const std::string& writePath);

  // Open file and get descriptor
	AKRESULT Open( 
    const AkOSChar*     in_pszFileName,     // File name.
    AkOpenMode          in_eOpenMode,       // Open mode.
    AkFileSystemFlags*  in_pFlags,          // Special flags. Can pass NULL.
    bool                in_bOverlapped,     // Overlapped IO open
    AkFileDesc&         out_fileDesc);      // Returned file descriptor.

	AKRESULT Open( 
    AkFileID            in_fileID,          // File ID.
    AkOpenMode          in_eOpenMode,       // Open mode.
    AkFileSystemFlags*  in_pFlags,          // Special flags. Can pass NULL.
    bool                in_bOverlapped,     // Overlapped IO open
    AkFileDesc&         out_fileDesc);      // Returned file descriptor.
  
  
  // Check if a file exist in all file read locations
  AKRESULT CheckFileExists(const AkOSChar * in_fileName);
  
	
	//
	// Path resolving services.
	// ------------------------------------------------------

  // Prefix locale path to file name
  AKRESULT GetFullInPath(const AkOSChar* in_pszFileName,
                         AkFileSystemFlags* in_pFlags,
                         AkOpenMode in_eOpenMode,
                         std::string& out_InPath);

  // Returns AK_Success if input flags are supported and the resulting path is not too long.
  // Returns AK_Fail otherwise.
  AKRESULT OpenWritePathEntry(
    const AkOSChar*     in_pszFileName,     // File name.
    AkOpenMode          in_eOpenMode,       // Open mode.
    AkFileSystemFlags*  in_pFlags,          // Special flags. Can pass NULL.
    bool                in_bOverlapped,     // Overlapped IO open
    AkFileDesc&         out_fileDesc);      // Returned file descriptor.

  AKRESULT OpenPathResolverEntry(
    const AkOSChar*     in_pszFileName,     // File name.
    AkOpenMode          in_eOpenMode,       // Open mode.
    AkFileSystemFlags*  in_pFlags,          // Special flags. Can pass NULL.
    bool                in_bOverlapped,     // Overlapped IO open
    AkFileDesc&         out_fileDesc);      // Returned file descriptor.

  AKRESULT OpenWwiseLocationEntry(
    const AkOSChar*     in_pszFileName,     // File name.
    AkOpenMode          in_eOpenMode,       // Open mode.
    AkFileSystemFlags*  in_pFlags,          // Special flags. Can pass NULL.
    bool                in_bOverlapped,     // Overlapped IO open
    AkFileDesc&         out_fileDesc);      // Returned file descriptor.
  
  static constexpr AkUInt32 const kZipEntryParamValue = 0x007a6970; // 'zip'
  AKRESULT OpenZipEntry(
    const AkOSChar*     in_pszFileName,     // File name.
    AkOpenMode          in_eOpenMode,       // Open mode.
    AkFileSystemFlags*  in_pFlags,          // Special flags. Can pass NULL.
    bool                in_bOverlapped,     // Overlapped IO open
    AkFileDesc&         out_fileDesc);      // Returned file descriptor.

#ifdef ANDROID
  static constexpr AkUInt32 const kAMEntryParamValue = 0x0061736d; // 'asm''
  AKRESULT OpenAssetManagerEntry(
    const AkOSChar*     in_pszFileName,     // File name.
    AkOpenMode          in_eOpenMode,       // Open mode.
    AkFileSystemFlags*  in_pFlags,          // Special flags. Can pass NULL.
    bool                in_bOverlapped,     // Overlapped IO open
    AkFileDesc&         out_fileDesc);      // Returned file descriptor.
#endif

protected:
  
  class ZipReaderItem
  {
  public:
    std::string path;
    Anki::AudioEngine::ZipReader zipReader;
  };
  class ZipFileHandle
  {
  public:
    Anki::AudioEngine::ZipReader* zipReader;
    uint32_t handle;
  };
  AkListBareLight<FilePath>                     m_Locations;
  Anki::AudioEngine::AudioFilePathResolverFunc  m_PathResolver;
  std::vector<ZipReaderItem*>                   m_ZipReaders;
  std::mutex                                    m_ZipReadersMutex;
  std::string                                   m_WriteFilePath;
#ifdef ANDROID
  AAssetManager*  m_AssetManager;
  std::string     m_AssetBasePath;
#endif
};

#include "audioMultipleFileLocation.inl"

#endif //_AK_MULTI_FILE_LOCATION_H_
