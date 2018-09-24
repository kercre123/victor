#ifndef ANIMPROCESS_COZMO_ALEXASPEAKER_H
#define ANIMPROCESS_COZMO_ALEXASPEAKER_H


#include "util/logging/logging.h"
#include <memory>

#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>


namespace Anki {
namespace Vector {
  
  class AlexaSpeaker : public alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface
  {
    using   SourceId = uint64_t;
    SourceId m_sourceID=0;
  public:
    
    
    virtual SourceId   setSource (std::shared_ptr< alexaClientSDK::avsCommon::avs::attachment::AttachmentReader > attachmentReader, const alexaClientSDK::avsCommon::utils::AudioFormat *format=nullptr) override {
      PRINT_NAMED_WARNING("WHATNOW", " speaker set source1");
      return m_sourceID++;
    }
    
    virtual SourceId   setSource (const std::string &url, std::chrono::milliseconds offset=std::chrono::milliseconds::zero()) override
    {
      PRINT_NAMED_WARNING("WHATNOW", " speaker set source2");
      return m_sourceID++;
    }
    
    virtual SourceId   setSource (std::shared_ptr< std::istream > stream, bool repeat) override {
      PRINT_NAMED_WARNING("WHATNOW", " speaker set source3");
      return m_sourceID++;
    }
    
    virtual bool   play (SourceId id) override {
      PRINT_NAMED_WARNING("WHATNOW", " speaker play");
      return true;
    }
    
    virtual bool   stop (SourceId id) override {
      PRINT_NAMED_WARNING("WHATNOW", " speaker stop");
      return true;
    }
    
    virtual bool   pause (SourceId id) override {
      PRINT_NAMED_WARNING("WHATNOW", " speaker pause");
      return true;
    }
    
    virtual bool   resume (SourceId id) override {
      PRINT_NAMED_WARNING("WHATNOW", " speaker resume");
      return true;
    }
    
    virtual std::chrono::milliseconds   getOffset (SourceId id) override {
      PRINT_NAMED_WARNING("WHATNOW", " speaker getOffset");
      return std::chrono::milliseconds{0};
    }
    
    virtual uint64_t   getNumBytesBuffered () override {
      PRINT_NAMED_WARNING("WHATNOW", " speaker getNumBytesBuffered");
      return 0;
    }
    
    virtual void   setObserver (std::shared_ptr< alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerObserverInterface > playerObserver) override {
      PRINT_NAMED_WARNING("WHATNOW", " speaker setObserver");
    }
  };

} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_ALEXASPEAKER_H
