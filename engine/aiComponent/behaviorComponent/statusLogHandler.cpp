/**
 * File: statusLogHandler.cpp
 *
 * Author: ross
 * Created: 2018-05-07
 *
 * Description: A handler for status messages and a store for recent statuses
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "engine/aiComponent/behaviorComponent/statusLogHandler.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "util/signals/simpleSignal.hpp"

namespace Anki {
namespace Cozmo {

namespace {
  constexpr unsigned int kStatusHistoryLength = 100;
}

StatusLogHandler::StatusLogHandler( const CozmoContext* context )
  : _context(context)
{
  _statusHistory.reset( new StatusHistory{kStatusHistoryLength} );
  
  auto* ei = _context->GetExternalInterface();
  
  auto handleEvent = [this](const AnkiEvent<ExternalInterface::MessageEngineToGame>& msg) {
    if( msg.GetData().GetTag() == ExternalInterface::MessageEngineToGameTag::Event ) {
      const auto& event = msg.GetData().Get_Event();
      if( event.GetTag() == ExternalInterface::EventTag::status ) {
        SaveStatusHistory( event.Get_status() );
      }
    }
  };
  // this is a good example of where it would be nice to subscribe to a leaf "status" instead of a branch "event"
  _signalHandles.push_back( ei->Subscribe(ExternalInterface::MessageEngineToGameTag::Event, handleEvent) );
  
  // handle request
  auto handleRequest = [this](const AnkiEvent<ExternalInterface::MessageGameToEngine>& msg) {
    if( msg.GetData().GetTag() == ExternalInterface::MessageGameToEngineTag::RobotHistoryRequest ) {
      SendStatusHistory();
    }
  };
  _signalHandles.push_back( ei->Subscribe(ExternalInterface::MessageGameToEngineTag::RobotHistoryRequest, handleRequest) );
}

void StatusLogHandler::SetFeature( const std::string& featureName, const std::string& source )
{
  // send to app
  auto* ei = _context->GetExternalInterface();
  if( ei != nullptr ) {
    ExternalInterface::FeatureStatus status;
    status.featureName = featureName;
    status.source = source;
    ei->Broadcast( ExternalMessageRouter::Wrap(std::move(status)) );
  }
}
  
void StatusLogHandler::SaveStatusHistory( const ExternalInterface::Status& status )
{
  TimeStamp_t timeStamp = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
  static_assert( std::is_same<TimeStamp_t, uint32_t>::value, "CLAD is expecting uint32");
  std::pair<uint32_t,ExternalInterface::Status> entry = std::make_pair( timeStamp, status );
  _statusHistory->push_back( std::move(entry) );
}
  
void StatusLogHandler::SendStatusHistory()
{
  ExternalInterface::RobotHistoryResult response;
  response.messages.reserve( _statusHistory->size() );
  for( size_t i=0; i<_statusHistory->size(); ++i ) {
    const auto& elem = (*_statusHistory.get())[i];
    response.messages.emplace_back( elem.second, elem.first );
  }
  auto* ei = _context->GetExternalInterface();
  if( ei != nullptr ) {
    ei->Broadcast( ExternalMessageRouter::Wrap(std::move(response)) );
  }
}

}
}
