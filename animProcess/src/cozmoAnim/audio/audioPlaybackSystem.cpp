/***********************************************************************************************************************
 *
 *  AudioPlaybackSystem
 *  Victor / Anim
 *
 *  Created by Jarrod Hatfield on 4/10/2018
 *
 *  Description
 *  + System to load and playback recordings/audio files/etc
 *
 **********************************************************************************************************************/

// #include "cozmoAnim/audio/audioPlaybackSystem.h"
#include "audioPlaybackSystem.h"

#include "cozmoAnim/animContext.h"
#include "cozmoAnim/audio/audioPlaybackJob.h"

#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include "util/threading/threadPriority.h"

#include <thread>


namespace Anki {
namespace Cozmo {
namespace Audio {

namespace {
  const char* kThreadName                 = "MicPlayback";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioPlaybackSystem::AudioPlaybackSystem( const AnimContext* context ) :
  _animContext( context )
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AudioPlaybackSystem::~AudioPlaybackSystem()
{
  // clear our any jobs left in the queue
  while ( !_jobQueue.empty() )
  {
    delete _jobQueue.front();
    _jobQueue.pop();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioPlaybackSystem::Update( BaseStationTime_t currTime_nanosec )
{
  // monitor our current job for completion
  if ( _currentJob && _currentJob->IsComplete() )
  {
    _currentJob.reset();
  }

  // if we're not working on a current job and we have more in the queue, start up the next job
  if ( !_currentJob )
  {
    StartNextJobInQueue();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioPlaybackSystem::PlaybackAudio( const std::string& path )
{
  CozmoAudioController* audioController = _animContext->GetAudioController();
  if ( ( nullptr != audioController ) && IsValidFile( path ) )
  {
    // simply push the job onto the queue and it'll take care of itself
    AudioPlaybackJob* newJob = new AudioPlaybackJob( audioController, path );
    _jobQueue.push( newJob );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AudioPlaybackSystem::IsValidFile( const std::string& path ) const
{
  return Util::FileUtils::FileExists( path );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioPlaybackSystem::StartNextJobInQueue()
{
  // simply move the first job in the queue into our current job
  if ( !_jobQueue.empty() )
  {
    // note: currently this wont stop the current job from playing as it's playing on it's own thread.
    //       maybe we want the ability to stop a playing job, but for now ignoring it
    _currentJob.reset( _jobQueue.front() );
    _jobQueue.pop();

    std::thread( StartAudioPlaybackJob, _currentJob ).detach();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AudioPlaybackSystem::StartAudioPlaybackJob( std::shared_ptr<AudioPlaybackJob> audiojob )
{
  // note: there is currently no callback setup for when playback is finished, so a job is considered complete as soon
  //       as the playback begins.  We'll most likely want to add a callback to this system (audio system already supports callbacks)
  Anki::Util::SetThreadName( pthread_self(), kThreadName );

  // keep updating our job until it's complete
  while ( audiojob && !audiojob->IsComplete() )
  {
    audiojob->Update();
  }
}

} // Audio
} // namespace Cozmo
} // namespace Anki
