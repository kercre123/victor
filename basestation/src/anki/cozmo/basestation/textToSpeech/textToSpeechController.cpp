/**
 * File: textToSpeechController.cpp
 *
 * Author: Molly Jameson
 * Created: 3/21/16
 *
 * Overhaul: Andrew Stein / Jordan Rivas, 4/22/16
 *
 * Description: Flite wrapper to save a wav.
 *
 * Copyright: Anki, inc. 2016
 *
 */
extern "C" {
  
#include "flite.h"
  
  cst_voice* register_cmu_us_kal(const char*);
}

#include "anki/cozmo/basestation/textToSpeech/textToSpeechController.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/cozmo/basestation/audio/audioController.h"
#include "anki/cozmo/basestation/audio/audioControllerPluginInterface.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "util/logging/logging.h"
#include "util/hashing/hashing.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/fileUtils/fileUtils.h"

const Anki::Util::Data::Scope kResourceScope = Anki::Util::Data::Scope::Cache;
const char* kFilePrefix = "TextToSpeech_";
const char* kFileExtension = "wav";


namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechController::TextToSpeechController(Util::Data::DataPlatform* dataPlatform,
                                               Audio::AudioController* audioController)
: _dataPlatform( dataPlatform )
, _dispatchQueue( Util::Dispatch::Create("TextToSpeechController_File_Operations") )
, _audioController( audioController )
{
  flite_init();
  
  _voice = register_cmu_us_kal(NULL);
} // TextToSpeechController()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechController::~TextToSpeechController()
{
  // Any tear-down needed for flite? (Un-init or unregister_cmu_us_kal?)
  
  Util::Dispatch::Stop(_dispatchQueue);
  Util::Dispatch::Release(_dispatchQueue);
} // ~TextToSpeechController()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechController::CreateSpeech(const std::string& text, CompletionFunc completion)
{
  const auto lutIter = _filenameLUT.find( text );
  if ( lutIter == _filenameLUT.end() ) {
    // text string is not in LUT check
    Util::Dispatch::Async( _dispatchQueue, [this, text, completion] ()
    {
      // First, check if it already exist
      bool success = false;
      std::string fullPath = MakeFullPath(text);
      std::string* fullPathPointer = nullptr;
      if ( Util::FileUtils::FileExists( fullPath ) ) {
        // Check if file exist on disk
        auto emplaceIter = _filenameLUT.emplace( text, fullPath );
        fullPathPointer = &emplaceIter.first->second;
        success = true;
      }
      else {
        // Create text to speech file
        float duration = flite_text_to_speech( text.c_str(), _voice, fullPath.c_str() );
        success = ( duration > 0.0 );
        auto emplaceIter = _filenameLUT.emplace( text, fullPath );
        fullPathPointer = &emplaceIter.first->second;
      }
      
      if ( completion != nullptr ) {
        ASSERT_NAMED( !(!success && fullPathPointer == nullptr), "TextToSpeechController::CreateSpeech invalid path pointer");
        completion( success, text, (fullPathPointer != nullptr) ? *fullPathPointer : "" );
      }
    });
  }
  
  else if ( completion != nullptr ) {
    // Text to speech file already exist
    completion( true, text, lutIter->second );
  }
} // CreateSpeech()
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechController::LoadSpeechData(const std::string& text, SayTextStyle style, CompletionFunc completion)
{
  using namespace Audio;
  
  // Check if text to speech file exist, if not created it
  CreateSpeech( text, [this, text, completion] (bool successCreateSpeech, const std::string& text, const std::string& fileName)
  {
    if (successCreateSpeech) {
      // Load file into memory
      Util::Dispatch::Async( _dispatchQueue, [this, text, fileName, completion] ()
      {
        const bool successLoadWaveFile = _waveFileReader.LoadWaveFile(fileName, fileName);
        if ( completion != nullptr ) {
         completion( successLoadWaveFile, text, fileName );
        }
      });
    }
    else if ( completion != nullptr ) {
     // Failed to create speech =(
     completion( false, text, fileName );
    }
  });
} // LoadSpeechData()
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechController::UnloadSpeechData(const std::string& text, SayTextStyle style)
{
  const auto iter = _filenameLUT.find( text );
  if ( iter != _filenameLUT.end() ) {
    _waveFileReader.ClearCachedWaveDataWithKey( iter->second );
  }
} // UnloadSpeechData()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool TextToSpeechController::PrepareToSay(const std::string& text, SayTextStyle style, float& out_duration_ms)
{
  using namespace Audio;
  
  // Find file name
  const auto lutIter = _filenameLUT.find( text );
  if ( lutIter == _filenameLUT.end() ) {
    return false;
  }
  
  // Set Wave Portal Plugin buffer
  const AudioWaveFileReader::StandardWaveDataContainer* data = _waveFileReader.GetCachedWaveDataWithKey( lutIter->second );
  if ( data == nullptr ) {
    return false;
  }

  AudioControllerPluginInterface* pluginInterface = _audioController->GetPluginInterface();
  ASSERT_NAMED(pluginInterface != nullptr, "TextToSpeechController.PrepareToSay.NullAudioControllerPluginInterface");

  // Clear previously loaded data
  if ( pluginInterface->WavePortalHasAudioDataInfo() ) {
    pluginInterface->ClearWavePortalAudioDataInfo();
  }

  // TODO: Make use of specified SayTextStyle
  pluginInterface->SetWavePortalAudioDataInfo( data->sampleRate,
                                               data->numberOfChannels,
                                               data->duration_ms,
                                               data->audioBuffer,
                                               static_cast<uint32_t>(data->bufferSize) );
  out_duration_ms = data->duration_ms;
  
  return true;
} // PrepareToSay()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechController::ClearLoadedSpeechData(const std::string& text, SayTextStyle style, SimpleCompletionFunc completion)
{
  using namespace Util;
  Dispatch::Async(_dispatchQueue, [this, text, style, completion]
  {
    const auto lutIter = _filenameLUT.find( text );
    if ( lutIter != _filenameLUT.end() ) {
      // Remove from memory, LUT & disk
      _waveFileReader.ClearCachedWaveDataWithKey( lutIter->second );
      FileUtils::DeleteFile( lutIter->second );
      _filenameLUT.erase( lutIter );
    }
    else {
      // Check if file exist on disk
      const std::string filePath = MakeFullPath( text );
      if ( FileUtils::FileExists( filePath ) ) {
        FileUtils::DeleteFile( filePath );
      }
    }
    
    if ( completion != nullptr ) {
      completion();
    }
  });
} // ClearLoadedSpeechData()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechController::ClearAllLoadedAudioData(bool deleteStaleFiles, SimpleCompletionFunc completion)
{
  using namespace Util;
  Dispatch::Async(_dispatchQueue, [this, deleteStaleFiles, completion] ()
  {
    // Delete all files in memory & LUT map
    for ( auto& iter : _filenameLUT ) {
      _waveFileReader.ClearCachedWaveDataWithKey( iter.second );
      FileUtils::DeleteFile( iter.second );
    }
    _filenameLUT.clear();
    
    // Clear all untrack text to speech files
    if ( deleteStaleFiles ) {
      auto fileNames = FindAllTextToSpeechFiles();
      for ( auto& iter : FindAllTextToSpeechFiles() ) {
        FileUtils::DeleteFile( iter );
      }
    }
    
    if ( completion != nullptr ) {
      completion();
    }
  });
} // ClearAllLoadedAudioData()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string TextToSpeechController::MakeFullPath(const std::string& text)
{
  // In case text contains sensitive data (e.g. player name), hash it so we don't save names
  // into logs if/when there's a message about this filename
  // TODO: Do we need something more secure?
  u32 hashedValue = 0;
  for ( auto& c : text ) {
    Util::AddHash(hashedValue, c, "TextToSpeechController.MakeFullPath.HashText");
  }
  
  // Note that this text-to-hash mapping is only printed in debug mode!
  PRINT_NAMED_DEBUG("TextToSpeechController.MakeFullPath.TextToHash",
                    "'%s' hashed to %d", text.c_str(), hashedValue);
  
  std::string fullPath = _dataPlatform->pathToResource(kResourceScope,
                                                       kFilePrefix + std::to_string(hashedValue) + "." + kFileExtension);
  
  return fullPath;
} // MakeFullPath()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::vector<std::string> TextToSpeechController::FindAllTextToSpeechFiles()
{
  using namespace Util;
  std::vector<std::string> fileNames;
  const std::string dirPath = _dataPlatform->pathToResource(kResourceScope, "");
  const auto dirFiles = FileUtils::FilesInDirectory( dirPath, false, kFileExtension, false );
  const std::string prefixStr( kFilePrefix );
  for ( auto& aFileName : dirFiles ) {
    // Check if file starts with desired prefix
    if ( aFileName.compare( 0, prefixStr.length(), prefixStr ) == 0 ) {
      fileNames.emplace_back(FileUtils::FullFilePath({dirPath, aFileName}));
    }
  }
  return fileNames;
} // FindAllTextToSpeechFiles()


} // end namespace Cozmo
} // end namespace Anki

