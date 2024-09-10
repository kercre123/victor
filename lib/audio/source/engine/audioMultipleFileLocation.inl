//////////////////////////////////////////////////////////////////////
//
// audioFileLocationBase.cpp
//
// Basic file location resolving: Uses simple path concatenation logic.
// Exposes basic path functions for convenience.
// For more details on resolving file location, refer to section "File Location" inside
// "Going Further > Overriding Managers > Streaming / Stream Manager > Low-Level I/O"
// of the SDK documentation. 
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#ifdef AK_WIN
#include <AK/Plugin/AkMP3SourceFactory.h> // For MP3 Codec ID.
#endif
#include <AK/Tools/Common/AkPlatformFuncs.h>
#ifdef AK_SUPPORT_WCHAR
#include <wchar.h>
#endif //AK_SUPPORT_WCHAR
#include <stdio.h>
#include <AK/Tools/Common/AkAssert.h>
#include <AK/Tools/Common/AkObject.h>

#include "audioFileHelpers.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

#include <string>

#ifdef ANDROID
#include "zipReader.h"
#endif

#define MAX_NUMBER_STRING_SIZE      (10)    // 4G
#define ID_TO_STRING_FORMAT_BANK    AKTEXT("%u.bnk")
#define ID_TO_STRING_FORMAT_WEM     AKTEXT("%u.wem")
#define MAX_EXTENSION_SIZE          (4)     // .xxx
#define MAX_FILETITLE_SIZE          (MAX_NUMBER_STRING_SIZE+MAX_EXTENSION_SIZE+1)   // null-terminated

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
AudioMultipleFileLocation<OPEN_POLICY>::AudioMultipleFileLocation()
: m_PathResolver(nullptr)
#ifdef ANDROID
, m_AssetManager(nullptr)
#endif
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
void AudioMultipleFileLocation<OPEN_POLICY>::Term()
{
	FilePath *p = (*m_Locations.Begin());
	while(p)
	{
		FilePath *next = p->pNextLightItem;
		AkDelete(AK::StreamMgr::GetPoolID(), p);
		p = next;
	}
  for (auto* it : m_ZipReaders)
  {
    delete it;
  }
  m_ZipReaders.clear();
	m_Locations.Term();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
AKRESULT AudioMultipleFileLocation<OPEN_POLICY>::Open(
  const AkOSChar*     in_pszFileName,     // File name.
  AkOpenMode          in_eOpenMode,       // Open mode.
  AkFileSystemFlags*  in_pFlags,          // Special flags. Can pass NULL.
  bool                in_bOverlapped,     // Overlapped IO open
  AkFileDesc&         out_fileDesc)       // Returned file descriptor.
{
  // If not read only mode first check write path
  if (in_eOpenMode != AK_OpenModeRead) {
    if ( AK_Success == OpenWritePathEntry(in_pszFileName, in_eOpenMode, in_pFlags, in_bOverlapped, out_fileDesc)) {
      return AK_Success;
    }
  }

  // Search for file in order of priority - See CheckFileExists()
  // Use Path Resolver func
  if ( AK_Success == OpenPathResolverEntry(in_pszFileName, in_eOpenMode, in_pFlags, in_bOverlapped, out_fileDesc)) {
    return AK_Success;
  }

  // Check Wwise locatoins
  if ( AK_Success == OpenWwiseLocationEntry(in_pszFileName, in_eOpenMode, in_pFlags, in_bOverlapped, out_fileDesc)) {
    return AK_Success;
  }

  // Check for file in zip container
  if ( AK_Success == OpenZipEntry(in_pszFileName, in_eOpenMode, in_pFlags, in_bOverlapped, out_fileDesc)) {
    return AK_Success;
  }

  // Check Android Asset Manager
#ifdef ANDROID
  if ( AK_Success == OpenAssetManagerEntry(in_pszFileName, in_eOpenMode, in_pFlags, in_bOverlapped, out_fileDesc)) {
    return AK_Success;
  }
#endif
  
  return AK_Fail;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
AKRESULT AudioMultipleFileLocation<OPEN_POLICY>::Open(
  AkFileID            in_fileID,          // File ID.
  AkOpenMode          in_eOpenMode,       // Open mode.
  AkFileSystemFlags*  in_pFlags,          // Special flags. Can pass NULL.
  bool                in_bOverlapped,     // Overlapped IO open
  AkFileDesc&         out_fileDesc)       // Returned file descriptor.
{
  if ( !in_pFlags ||
       !( (in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC) ||
          (in_pFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC_EXTERNAL) ) ) {
    AKASSERT( !"Unhandled file type" );
    return AK_Fail;
  }
  
  AkOSChar pszTitle[MAX_FILETITLE_SIZE+1];
  if ( in_pFlags->uCodecID == AKCODECID_BANK )
  AK_OSPRINTF( pszTitle, MAX_FILETITLE_SIZE, ID_TO_STRING_FORMAT_BANK, (unsigned int)in_fileID );
  else
  AK_OSPRINTF( pszTitle, MAX_FILETITLE_SIZE, ID_TO_STRING_FORMAT_WEM, (unsigned int)in_fileID );
  
  return Open(pszTitle, in_eOpenMode, in_pFlags, in_bOverlapped, out_fileDesc);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class OPEN_POLICY>
AKRESULT AudioMultipleFileLocation<OPEN_POLICY>::CheckFileExists(const AkOSChar * in_fileName)
{
  // Search for file in order of priority
  // 1. Search using Platform Resolver
  if (m_PathResolver) {
    const std::string fileNameStr(in_fileName);
    std::string out_fullFilePath;
    if ( m_PathResolver(fileNameStr, out_fullFilePath) ) {
      if ( AK_Success == AudioFileHelpers::CheckFileExists(out_fullFilePath.c_str()) ) {
        return AK_Success;
      }
    }
  }
  
  // 2. Search Wwise locations
  for (typename AkListBareLight<FilePath>::Iterator it = m_Locations.Begin(); it != m_Locations.End(); ++it) {
    // Get the full file path, using path concatenation logic.
    const std::string fullFilePath = Anki::Util::FileUtils::FullFilePath({ (*it)->szPath, in_fileName });
    if (AK_Success == AudioFileHelpers::CheckFileExists(fullFilePath.c_str())) {
      return AK_Success;
    }
  }

  // 3. Search in zip files
  {
    uint32_t handle;
    uint32_t size;
    for ( ZipReaderItem* zri : m_ZipReaders ) {
      if ( zri->zipReader.Find(in_fileName, handle, size) ) {
        return AK_Success;
      }
    }
  }
  
#ifdef ANDROID
  // 4. Search Asset Manager
  if (m_AssetManager) {
    std::vector<std::string> pathList;
    if (!m_AssetBasePath.empty()) {
      pathList.push_back(m_AssetBasePath);
    }
    pathList.push_back(in_fileName);
    const std::string fullFilePath = Anki::Util::FileUtils::FullFilePath(std::move(pathList));
    AAsset* asset = AAssetManager_open(m_AssetManager, fullFilePath.c_str(), AASSET_MODE_UNKNOWN);
    const AKRESULT result = (asset != nullptr) ? AK_Success : AK_Fail;
    if (asset != nullptr) {
      AAsset_close(asset);
    }
    return result;
  }
#endif
  
  return AK_Fail;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
AKRESULT AudioMultipleFileLocation<OPEN_POLICY>::GetFullInPath(
  const AkOSChar* in_pszFileName,
  AkFileSystemFlags* in_pFlags,
  AkOpenMode in_eOpenMode,
  std::string& out_InPath)
{
  if ( !in_pszFileName ) {
    AKASSERT( !"Invalid file name" );
    return AK_InvalidParameter;
  }
  // If Read mode append the locale path
  std::vector<std::string> pathList;
  if ((in_pFlags != NULL) &&
      (in_pFlags->bIsLanguageSpecific) &&
      (in_eOpenMode == AK_OpenModeRead)) {
    pathList.push_back(AK::StreamMgr::GetCurrentLanguage());
  }
  // Add file name
  pathList.push_back(in_pszFileName);
  const std::string fullFilePath = Anki::Util::FileUtils::FullFilePath(std::move(pathList));
  if (fullFilePath.length() >= AK_MAX_PATH) {
    AKASSERT( !"Input string too large" );
    return AK_InvalidParameter;
  }
  out_InPath.clear();
  out_InPath = fullFilePath;
  return AK_Success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
AKRESULT AudioMultipleFileLocation<OPEN_POLICY>::OpenWritePathEntry(
  const AkOSChar*     in_pszFileName,     // File name.
  AkOpenMode          in_eOpenMode,       // Open mode.
  AkFileSystemFlags*  in_pFlags,          // Special flags. Can pass NULL.
  bool                in_bOverlapped,     // Overlapped IO open
  AkFileDesc&         out_fileDesc)       // Returned file descriptor.
{
  // Check if the write path has been set
  if (m_WriteFilePath.empty()) {
    return AK_FileNotFound;
  }
  
  // Create full file path
  const std::string fullFilePath = Anki::Util::FileUtils::FullFilePath({ m_WriteFilePath, in_pszFileName });
  if (fullFilePath.length() >= AK_MAX_PATH) {
    AKASSERT( !"File name string too large" );
    return AK_Fail;
  }

  if (AK_Success == OPEN_POLICY::Open(fullFilePath.c_str(), in_eOpenMode, in_bOverlapped, out_fileDesc))
  {
    // Check file descriptor
    AKASSERT(out_fileDesc.hFile != NULL );
    return AK_Success;
  }

  return AK_FileNotFound;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
AKRESULT AudioMultipleFileLocation<OPEN_POLICY>::OpenPathResolverEntry(
  const AkOSChar*     in_pszFileName,     // File name.
  AkOpenMode          in_eOpenMode,       // Open mode.
  AkFileSystemFlags*  in_pFlags,          // Special flags. Can pass NULL.
  bool                in_bOverlapped,     // Overlapped IO open
  AkFileDesc&         out_fileDesc)       // Returned file descriptor.
{
  // Return if path resolver func is not set
  if (m_PathResolver == nullptr) {
    return AK_FileNotFound;
  }
  std::string fullPath;
  std::string inPath;
  AKRESULT result = GetFullInPath(in_pszFileName, in_pFlags, in_eOpenMode, inPath);
  
  if (result != AK_Success) {
    return result;
  }
  
  if (!m_PathResolver(inPath, fullPath))
  {
    return AK_Fail;
  }
  
  if (fullPath.length() >= AK_MAX_PATH) {
    AKASSERT( !"File name string too large" );
    return AK_Fail;
  }
  
  AKRESULT res = OPEN_POLICY::Open(fullPath.c_str(), in_eOpenMode, in_bOverlapped, out_fileDesc);
  if (res == AK_Success)
  {
    // Check file descriptor
    AKASSERT(out_fileDesc.hFile != NULL );
    // If the file is not empty check that the Open mode is Read or Read Write
    AKASSERT(( (out_fileDesc.iFileSize != 0) &&
               ((in_eOpenMode == AK_OpenModeRead) || (in_eOpenMode == AK_OpenModeReadWrite)) ) ||
             !( (in_eOpenMode == AK_OpenModeRead) || (in_eOpenMode == AK_OpenModeReadWrite) ));
    return AK_Success;
  }
  
  return AK_FileNotFound;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
AKRESULT AudioMultipleFileLocation<OPEN_POLICY>::OpenWwiseLocationEntry(
  const AkOSChar*     in_pszFileName,     // File name.
  AkOpenMode          in_eOpenMode,       // Open mode.
  AkFileSystemFlags*  in_pFlags,          // Special flags. Can pass NULL.
  bool                in_bOverlapped,     // Overlapped IO open
  AkFileDesc&         out_fileDesc)       // Returned file descriptor.
{
  // Prefix file name with locale
  std::string out_FileInPath;
  AKRESULT result = GetFullInPath(in_pszFileName, in_pFlags, in_eOpenMode, out_FileInPath);
  if (result != AK_Success) {
    return result;
  }
  
  for (typename AkListBareLight<FilePath>::Iterator it = m_Locations.Begin(); it != m_Locations.End(); ++it) {
    // Create full file path
    const std::string fullFilePath = Anki::Util::FileUtils::FullFilePath({ (*it)->szPath, out_FileInPath });
    if (fullFilePath.length() >= AK_MAX_PATH) {
      AKASSERT( !"File name string too large" );
      return AK_Fail;
    }
    AKRESULT res = OPEN_POLICY::Open(fullFilePath.c_str(), in_eOpenMode, in_bOverlapped, out_fileDesc);
    if (res == AK_Success)
    {
      // Check file descriptor
      AKASSERT(out_fileDesc.hFile != NULL );
      // If the file is not empty check that the Open mode is Read or Read Write
      AKASSERT(( (out_fileDesc.iFileSize != 0) &&
                 ((in_eOpenMode == AK_OpenModeRead) || (in_eOpenMode == AK_OpenModeReadWrite)) ) ||
               !( (in_eOpenMode == AK_OpenModeRead) || (in_eOpenMode == AK_OpenModeReadWrite) ));
      return AK_Success;
    }
  }
  return AK_FileNotFound;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
AKRESULT AudioMultipleFileLocation<OPEN_POLICY>::OpenZipEntry(
  const AkOSChar*     in_pszFileName,     // File name.
  AkOpenMode          in_eOpenMode,       // Open mode.
  AkFileSystemFlags*  in_pFlags,          // Special flags. Can pass NULL.
  bool                in_bOverlapped,     // Overlapped IO open
  AkFileDesc&         out_fileDesc)       // Returned file descriptor.
{
  // Zip Entries are Read only
  if ( in_eOpenMode != AK_OpenModeRead ) {
    return AK_Fail;
  }
  
  // Prefix file name with locale
  std::string out_FileInPath;
  AKRESULT res = GetFullInPath(in_pszFileName, in_pFlags, in_eOpenMode, out_FileInPath);
  if (res != AK_Success) {
    return res;
  }
  if (m_ZipReaders.empty()) {
    return AK_Fail;
  }
  
  uint32_t handle;
  uint32_t size;
  for (ZipReaderItem* zri : m_ZipReaders) {
    bool result = zri->zipReader.Find(out_FileInPath, handle, size);
    if (result) {
      ZipFileHandle* zipFileHandle = new ZipFileHandle();
      zipFileHandle->zipReader = &(zri->zipReader);
      zipFileHandle->handle = handle;
      out_fileDesc.hFile = (AkFileHandle) zipFileHandle;
      out_fileDesc.iFileSize = (AkInt64) size;
      out_fileDesc.uSector = 0;
      out_fileDesc.pCustomParam = (void *) kZipEntryParamValue;
      
      // Check file descriptor
      AKASSERT(out_fileDesc.hFile != NULL );
      return AK_Success;
    }
  }
  return AK_FileNotFound;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#ifdef ANDROID
template<class OPEN_POLICY>
AKRESULT AudioMultipleFileLocation<OPEN_POLICY>::OpenAssetManagerEntry(
  const AkOSChar* in_pszFileName,
  AkOpenMode      in_eOpenMode,       // Open mode.
  AkFileSystemFlags * in_pFlags,      // Special flags. Can pass NULL.
  bool      in_bOverlapped,           // Overlapped IO open
  AkFileDesc &    out_fileDesc)       // Returned file descriptor.
{
  // Android Asset Manger Entries are Read only
  if ( in_eOpenMode != AK_OpenModeRead ) {
    return AK_Fail;
  }
  
  // Prefix file name with locale
  std::string out_FileInPath;
  AKRESULT res = GetFullInPath(in_pszFileName, in_pFlags, in_eOpenMode, out_FileInPath);
  if (res != AK_Success) {
    return res;
  }
  
  if (!m_AssetManager) {
    return AK_Fail;
  }
  
  // Create Path list
  std::vector<std::string> pathList;
  if (!m_AssetBasePath.empty()) {
    pathList.push_back(m_AssetBasePath);
  }
  pathList.push_back(out_FileInPath);
  const std::string fullFilePath = Anki::Util::FileUtils::FullFilePath(std::move(pathList));

  AAsset* asset = AAssetManager_open(m_AssetManager, fullFilePath.c_str(), AASSET_MODE_UNKNOWN);
  if (!asset) {
    return AK_FileNotFound;
  }
  
  out_fileDesc.hFile = (AkFileHandle) asset;
  AkInt64 fileSize = AAsset_getLength((AAsset *) out_fileDesc.hFile);
  out_fileDesc.iFileSize = fileSize;
  out_fileDesc.uSector = 0;
  out_fileDesc.pCustomParam = (void *) kAMEntryParamValue;
  
  // Check file descriptor
  AKASSERT(out_fileDesc.hFile != NULL );
  
  return AK_Success;
}
#endif // ANDROID

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
AKRESULT AudioMultipleFileLocation<OPEN_POLICY>::AddBasePath(const AkOSChar* in_pszBasePath)
{
	AkUInt32 len = (AkUInt32)AKPLATFORM::OsStrLen( in_pszBasePath ) + 1;
	if ( len + AKPLATFORM::OsStrLen( AK::StreamMgr::GetCurrentLanguage() ) >= AK_MAX_PATH )
		return AK_InvalidParameter;

	FilePath * pPath = (FilePath*)AkAlloc(AK::StreamMgr::GetPoolID(), sizeof(FilePath) + sizeof(AkOSChar)*(len - 1));
	if (pPath == NULL)
		return AK_InsufficientMemory;

	// Copy the base path EVEN if the directory does not exist.
	AKPLATFORM::SafeStrCpy( pPath->szPath, in_pszBasePath, len );
	pPath->pNextLightItem = NULL;
	m_Locations.AddFirst(pPath);

	AKRESULT eDirectoryResult = AudioFileHelpers::CheckDirectoryExists( in_pszBasePath );
	if( eDirectoryResult == AK_Fail ) // AK_NotImplemented could be returned and should be ignored.
	{
		return AK_PathNotFound;
	}

	return AK_Success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
void AudioMultipleFileLocation<OPEN_POLICY>::SetPathResolver( Anki::AudioEngine::AudioFilePathResolverFunc pathResolver )
{
	m_PathResolver = pathResolver;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
AKRESULT AudioMultipleFileLocation<OPEN_POLICY>::AddZipFile( const std::string& path,
                                                             SearchPathLocation location )
{
  std::lock_guard<std::mutex> lock(m_ZipReadersMutex);

  if (path.empty())
  {
    return AK_InvalidParameter;
  }

  auto search = std::find_if(m_ZipReaders.begin(), m_ZipReaders.end(),
                             [&path](const ZipReaderItem* p) {
                               return p->path == path;
                             });
  if (search != m_ZipReaders.end())
  {
    return AK_PathNodeAlreadyInList;
  }

  ZipReaderItem* zri = new ZipReaderItem();
  zri->path = path;

  if (location == SearchPathLocation::Head)
  {
    m_ZipReaders.insert(m_ZipReaders.begin(), zri);
  }
  else
  {
    m_ZipReaders.push_back(zri);
  }
  bool success = zri->zipReader.Init(path);
  if (!success)
  {
    if (location == SearchPathLocation::Head)
    {
      m_ZipReaders.erase(m_ZipReaders.begin());
    }
    else
    {
      m_ZipReaders.pop_back();
    }
    delete zri;
  }
  return success ? AK_Success : AK_Fail;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
void AudioMultipleFileLocation<OPEN_POLICY>::RemoveMissingZipFiles()
{
  std::vector<std::string> pathsToRemove;
  {
    std::lock_guard<std::mutex> lock(m_ZipReadersMutex);
    for (const ZipReaderItem* p : m_ZipReaders) {
      std::vector<std::string> parts = Anki::AudioEngine::ZipReader::SplitPath(p->path);
      if (!parts.empty()) {
        if (AK_Fail == AudioFileHelpers::CheckFileExists(parts[0].c_str())) {
          pathsToRemove.push_back(p->path);
        }
      }
    }
  }
  for (auto const& p : pathsToRemove) {
    (void) RemoveZipFile(p);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
AKRESULT AudioMultipleFileLocation<OPEN_POLICY>::RemoveZipFile( const std::string& path )
{
  std::lock_guard<std::mutex> lock(m_ZipReadersMutex);
  auto search = std::find_if(m_ZipReaders.begin(), m_ZipReaders.end(),
                             [&path](const ZipReaderItem* p) {
                               return p->path == path;
                             });
  if (search == m_ZipReaders.end())
  {
    return AK_PathNotFound;
  }
  ZipReaderItem* zri = *search;
  zri->zipReader.Term();
  m_ZipReaders.erase(search);
  delete zri; zri = nullptr;
  return AK_Success;
}

#ifdef ANDROID

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
void AudioMultipleFileLocation<OPEN_POLICY>::SetAssetManager( void* assetManager )
{
  m_AssetManager = (AAssetManager *) assetManager;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
void AudioMultipleFileLocation<OPEN_POLICY>::SetAssetBasePath( const std::string& basePath )
{
  m_AssetBasePath = basePath;
}

#endif // ANDROID

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<class OPEN_POLICY>
void AudioMultipleFileLocation<OPEN_POLICY>::SetWriteBasePath( const std::string& writePath )
{
  m_WriteFilePath = writePath;

  // Create write path if needed
  if (!m_WriteFilePath.empty() &&
      !Anki::Util::FileUtils::CreateDirectory(m_WriteFilePath)) {
    PRINT_NAMED_WARNING("AudioMultipleFileLocation.SetWriteBasePath",
                        "Can NOT create write path '%s'", m_WriteFilePath.c_str());
  }
}
