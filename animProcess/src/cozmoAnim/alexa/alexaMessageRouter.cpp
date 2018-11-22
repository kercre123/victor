/**
 * File: alexaMessageRouter.cpp
 *
 * Author: ross
 * Created: Nov 4 2018
 *
 * Description: Subclass of the MessageRouter that accepts MessageRequestObserverInterface observers.
 *              We need this to receive onSendCompleted, which provides error status on failed
 *              attempts to send data to AVS.
 *
 *
 * Copyright: Anki, Inc. 2018
 *
 */

// Since this file uses the sdk, here's the SDK license
/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "cozmoAnim/alexa/alexaMessageRouter.h"

// for output debugging
#include "coretech/common/engine/utils/timer.h"
#include "json/json.h"
#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"
#include <fstream>

namespace Anki {
namespace Vector {

using namespace alexaClientSDK;
#define LOG_CHANNEL "Alexa"

namespace {

  const std::string kMessageOutputFile = "messages.txt";

  CONSOLE_VAR(bool, kLogAlexaMessages, "Alexa.Messaging", false);

  // NOTE: due to me being a lazy developer, this means it _wont_ get sent up, and won't actually work, but
  // you can use this flag to get a dump of the file to check wakeword timing. See kDumpAlexaTriggerAudio in
  // alexaImpl.cpp for a better implementation. This one is only useful if you want to test the exact bytes
  // that would be sent up (but then they don't get sent up....)
  CONSOLE_VAR(bool, kStealAlexaWakewordAudio, "Alexa.Messaging", false);

  // hack to work around not having a data platform at this level. Note that this only works on robot
  std::string GetAlexaCacheFolder()
  {
    static std::string alexaCacheFolder = "";
    if( alexaCacheFolder.empty() ) {
      alexaCacheFolder = "/data/data/com.anki.victor/cache/alexa/";
      if( !alexaCacheFolder.empty() && Util::FileUtils::DirectoryDoesNotExist( alexaCacheFolder ) ) {
        Util::FileUtils::CreateDirectory( alexaCacheFolder );
      }

      // delete the old file
      if( Util::FileUtils::FileExists( alexaCacheFolder + kMessageOutputFile ) ) {
        Util::FileUtils::DeleteFile( alexaCacheFolder + kMessageOutputFile );
      }

      LOG_INFO("AlexaMessageRouter.LogMessages", "Logging outgoing messages to '%s'",
               (alexaCacheFolder + kMessageOutputFile).c_str());
    }
    return alexaCacheFolder;
  }

  // for debugging comms to/from alexa
  void WriteMessageDebugFile( const std::string& message, bool isSend)
  {
    std::string alexaCacheFolder = GetAlexaCacheFolder();

    Json::Value json;
    Json::Reader reader;
    const float bsTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    bool success = reader.parse( message, json );

    // use json-formatted string if possible
    const std::string& messageStr = success ? json.toStyledString() : message;
    const std::string messageType = isSend ? "SEND" : "RECV";

    const bool append = true;
    const bool ok = Util::FileUtils::WriteFile( alexaCacheFolder + kMessageOutputFile,
                                                std::to_string(bsTime) + " " + messageType + ": " +
                                                messageStr.c_str(),
                                                append );
      ANKI_VERIFY(ok, "AlexaMessageRouter.CouldNotLogMessage",
                  "could not write to file '%s'",
                  (alexaCacheFolder + kMessageOutputFile).c_str());
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AlexaMessageRouter::AlexaMessageRouter( const std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::MessageRequestObserverInterface>>& observers,
                                        std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
                                        std::shared_ptr<avsCommon::avs::attachment::AttachmentManager> attachmentManager,
                                        std::shared_ptr<acl::TransportFactoryInterface> transportFactory,
                                        const std::string& avsEndpoint )
: MessageRouter{ authDelegate, attachmentManager, transportFactory, avsEndpoint }
, _observers{ observers }
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AlexaMessageRouter::sendMessage( std::shared_ptr<avsCommon::avs::MessageRequest> request )
{
  for( auto& observer : _observers ) {
    request->addObserver( observer );
  }

  if( kLogAlexaMessages ) {
    const int numAttachments = request->attachmentReadersCount();
    LOG_DEBUG("AlexaMessageRouter.SendMessage", "sending message to AVS (%d attachments)", numAttachments);

    const bool isSend = true;
    WriteMessageDebugFile( request->getJsonContent(), isSend );

    if( kStealAlexaWakewordAudio ) {
      for( int attachment = 0; attachment < numAttachments; ++attachment ) {
        // TODO:(bn) move to a thread / queue?
        auto reader = request->getAttachmentReader(attachment);
        if( !reader ) {
          LOG_ERROR("AlexaMessageRouter.CouldNotGetAttachmentReader",
                    "null attachment reader for attachment id %d",
                    attachment);
          continue;
        }

        // TODO:(bn) make helper for dumping an attachment
        std::string alexaCacheFolder = GetAlexaCacheFolder();
        static int sAttachmentIdx = 0;
        const std::string filename = alexaCacheFolder + "attachment_" + std::to_string(sAttachmentIdx) + ".pcm";
        std::ofstream fileOut;
        std::ios_base::openmode mode = std::ios::out | std::ofstream::binary;
        fileOut.open(filename,mode);
        if( fileOut.is_open() ) {
          const size_t buffSizebytes = 1024;
          char buf[buffSizebytes] = {0};

          size_t read = 1;
          while ( read > 0 ) {
            using AVSReadStatus = alexaClientSDK::avsCommon::avs::attachment::AttachmentReader::ReadStatus;
            AVSReadStatus status;
            read = reader->reader->read(buf, buffSizebytes, &status);
            if( status != AVSReadStatus::OK ) {
              break;
            }
            fileOut.write(buf, read);
          }
          fileOut.flush();
          fileOut.close();
          // reset reader
          // reader->reader->seek(0);
          // TODO:(bn) this doesn't work at all, not sure if there's a better way to do this so for now I'm
          // using a different approach in alexaImpl.cpp to non-destructively save this data

          LOG_INFO("AlexaMessageRouter.WroteAttachment",
                   "Wrote attachment id '%d' to file '%s' for reader with name '%s'",
                   attachment,
                   filename.c_str(),
                   reader->name.c_str());
        }
      }
    }
  }

  MessageRouter::sendMessage( request );
}

void AlexaMessageRouter::consumeMessage(const std::string& contextId, const std::string& message)
{
  LOG_DEBUG("AlexaMessageRouter.ConsumeMessage", "consuming message cid '%s', message len %zu",
           contextId.c_str(),
           message.size());

  if( kLogAlexaMessages ) {
    const bool isSend = false;
    WriteMessageDebugFile( message, isSend );
  }

  alexaClientSDK::acl::MessageRouter::consumeMessage(contextId, message);
}


}  // namespace Vector
}  // namespace Anki
