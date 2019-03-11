#include "arf/Introspection.h"
#include "arf/datatypes_to_proto.h"

namespace ARF
{

TaggedMixin::TaggedMixin() : _uuid() {}

const UUID& TaggedMixin::GetUUID() const
{
    return _uuid;
}

void DataEvent::SetTimeToNow()
{
    monoTime = get_time<MonotonicClock>();
    wallTime = get_time<WallClock>();
}

TaskLog::TaskLog()
: queueTimeMono( MonotonicTime::zero() )
, startTimeMono( MonotonicTime::zero() )
, endTimeMono( MonotonicTime::zero() )
{}

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
    arf_proto::ArfLogItem proto;
    ConvertDataEventToProto( event, proto.mutable_data_event() );
    int32_t size = proto.ByteSizeLong();
    _logStream.write( reinterpret_cast<char*>( &size ), sizeof( size ) );
    proto.SerializeToOstream( &_logStream );
    _eventCount++;
}

void Logger::LogTaskLog( const TaskLog& log )
{
    Lock lock( _mutex );
    arf_proto::ArfLogItem proto;
    ConvertTaskLogToProto( log, proto.mutable_task_log() );
    int32_t size = proto.ByteSizeLong();
    _logStream.write( reinterpret_cast<char*>( &size ), sizeof( size ) );
    proto.SerializeToOstream( &_logStream );
    _eventCount++;
}

size_t Logger::NumEventsLogged() const
{
    Lock lock( _mutex );
    return _eventCount;
}

}