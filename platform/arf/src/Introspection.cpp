#include "arf/Introspection.h"
#include "arf/datatypes_to_proto.h"
#include <sstream>
#include <iomanip>

namespace ARF
{

std::string to_string( const UUID& uuid )
{
    std::stringstream ss;
    ss << std::hex << std::setfill ('0');
    for( size_t i = 0; i < uuid.size(); i+=2 )
    {
        ss << std::setw(2) << static_cast<int>(uuid[i]);
    }
    return ss.str();
}

std::ostream& operator<<( std::ostream& os, const UUID& uuid )
{
    std::string uuid_string = to_string(uuid);
    os.write(uuid_string.c_str(), uuid_string.length());
    return os;
}

TaggedMixin::TaggedMixin() : _uuid() {}

const UUID& TaggedMixin::GetUUID() const
{
    return _uuid;
}

DataEvent::DataEvent()
: monoTime( MonotonicTime::zero() )
, wallTime( WallTime::zero() )
{}


void DataEvent::SetTimeToNow()
{
    monoTime = get_time<MonotonicClock>();
    wallTime = get_time<WallClock>();
}

DataLog::DataLog()
: creationMonoTime( MonotonicTime::zero() )
, creationWallTime( WallTime::zero() )
, destructionMonoTime( MonotonicTime::zero() )
, destructionWallTime( WallTime::zero() )
{}

bool DataLog::ProcessEvent( const DataEvent& event )
{
    // If this log is uninitialized
    if( dataID.empty() )
    {
        dataID = event.dataID;
    }

    // Bail if ID mismatched
    if( dataID != event.dataID ) 
    { 
        std::cerr << "Data ID mismatch, expected: " << dataID << " got: " << event.dataID << std::endl;
        return false; 
    }

    switch( event.type )
    {
        case DataEvent::Type::CREATION:
        {
            if( creationParentIDs.size() > 0 )
            {
                std::cerr << "Already processed creation for this data!" << std::endl;
                return false;
            }
            creationParentIDs = event.parentIDs;
            creationMonoTime = event.monoTime;
            creationWallTime = event.wallTime;
            break;
        }
        case DataEvent::Type::MOVEMENT:
        {
            if( event.parentIDs.size() != 2 )
            {
                std::cerr << "Received movement event with incorrect number of parents" << std::endl;
                return false;
            }
            MovementLog log;
            log.fromID = event.parentIDs[0];
            log.toID = event.parentIDs[1];
            log.monoTime = event.monoTime;
            log.wallTime = event.wallTime;
            movementLogs.insert( log );
            break;
        }
        case DataEvent::Type::DESTRUCTION:
        {
            // TODO
            break;
        }
        default:
        {
            std::cerr << "Unrecognized data event type!" << std::endl;
            return false;
            break;
        }
    }
    return true;
}

Logger* Logger::_inst = new Logger();

Logger::Logger() {}

Logger::~Logger()
{
    _logStream.close();
}

Logger& Logger::Inst()
{
    return *_inst;
}

bool Logger::Initialize( const std::string& logPath )
{
    Lock lock( _mutex );
    _logStream.open( logPath );
    _eventCount = 0;
    return _logStream.is_open();
}

bool Logger::IsInitialized() const
{
    Lock lock( _mutex );
    return _logStream.is_open();
}

void Logger::Shutdown()
{
    Lock lock( _mutex );
    _logStream.close();
}

void Logger::LogDataEvent( const DataEvent& event )
{
    Lock lock( _mutex );
    arf_proto::ArfEventItem proto;
    ConvertDataEventToProto( event, proto.mutable_data_event() );
    stream_proto( proto, _logStream );
    _eventCount++;
}

void Logger::LogTaskEvent( const TaskEvent& event )
{
    Lock lock( _mutex );
    arf_proto::ArfEventItem proto;
    ConvertTaskEventToProto( event, proto.mutable_task_event() );
    stream_proto( proto, _logStream );
    _eventCount++;
}

size_t Logger::NumEventsLogged() const
{
    Lock lock( _mutex );
    return _eventCount;
}

}