#include "arf/datatypes_to_proto.h"

namespace ARF {

void ConvertWallTimeToProto(const WallTime& time,
                            arf_proto::Time* proto)
{
  TimeParts parts = time_to_parts( time );
  proto->set_seconds( parts.secs );
  proto->set_nanoseconds( parts.nanosecs );
}

void ConvertProtoToWallTime(const arf_proto::Time& proto,
                            WallTime* time)
{
  *time = time_from_parts<WallTime>( proto.seconds(), proto.nanoseconds() );
}

void ConvertMonoTimeToProto(const MonotonicTime& time,
                            arf_proto::Time* proto )
{
  TimeParts parts = time_to_parts( time );
  proto->set_seconds( parts.secs );
  proto->set_nanoseconds( parts.nanosecs );
}

void ConvertProtoToMonoTime(const arf_proto::Time& proto,
                            MonotonicTime* time)
{
  *time = time_from_parts<WallTime>( proto.seconds(), proto.nanoseconds() );

}

void ConvertUUIDToProto(const UUID& uuid,
                        arf_proto::UUID* proto)
{
  proto->set_uuid( uuid.c_str(), uuid.length() );
}

void ConvertProtoToUUID(const arf_proto::UUID& proto,
                        UUID* uuid)
{
  *uuid = proto.uuid().data();
}

void ConvertDataEventToProto(const DataEvent& event,
                             arf_proto::DataEvent* proto)
{
  proto->set_type( static_cast<arf_proto::DataEvent_Type>(event.type) );
  ConvertUUIDToProto( event.dataID, proto->mutable_data_id() );
  proto->clear_parent_ids();
  for( const UUID& id : event.parentIDs )
  {
    arf_proto::UUID* protoID = proto->add_parent_ids();
    ConvertUUIDToProto( id, protoID );
  }
  ConvertMonoTimeToProto( event.monoTime, proto->mutable_mono_time() );
  ConvertWallTimeToProto( event.wallTime, proto->mutable_wall_time() );
}

void ConvertProtoToDataEvent(const arf_proto::DataEvent& proto,
                             DataEvent* event)
{
  event->type = static_cast<DataEvent::Type>( proto.type() );
  ConvertProtoToUUID( proto.data_id(), &event->dataID );
  event->parentIDs.resize( proto.parent_ids_size() );
  for( int i = 0; i < proto.parent_ids_size(); ++i )
  {
    ConvertProtoToUUID( proto.parent_ids( i ), &event->parentIDs[i] );
  }
  ConvertProtoToMonoTime( proto.mono_time(), &event->monoTime );
  ConvertProtoToWallTime( proto.wall_time(), &event->wallTime );
}

void ConvertTaskLogToProto(const TaskLog& log,
                           arf_proto::TaskLog* proto)
{
  ConvertUUIDToProto( log.taskID, proto->mutable_task_id() );
  if( !log.sourceID.empty() )
  {
    ConvertUUIDToProto( log.sourceID, proto->mutable_source_id() );
  }
  else
  {
    proto->clear_source_id();
  }
  if( !log.threadID.empty() )
  {
    ConvertUUIDToProto( log.threadID, proto->mutable_thread_id() );
  }
  else
  {
    proto->clear_thread_id();
  }
  proto->clear_data_ids();
  for( const UUID& id : log.dataIDs )
  {
    arf_proto::UUID* protoID = proto->add_data_ids();
    ConvertUUIDToProto( id, protoID );
  }
  ConvertMonoTimeToProto( log.queueTimeMono, proto->mutable_queue_time_mono() );
  if( time_is_valid( log.startTimeMono ) )
  {
    ConvertMonoTimeToProto( log.startTimeMono, proto->mutable_start_time_mono() );
  }
  if( time_is_valid( log.endTimeMono ) )
  {
    ConvertMonoTimeToProto( log.endTimeMono, proto->mutable_end_time_mono() );
  }
  proto->set_status( static_cast<arf_proto::TaskLog_Status>( log.status ) );
}

void ConvertProtoToTaskLog(const arf_proto::TaskLog& proto,
                           TaskLog* log)
{
  ConvertProtoToUUID( proto.task_id(), &log->taskID );
  if( proto.has_source_id() )
  {
    ConvertProtoToUUID( proto.source_id(), &log->sourceID );
  }
  if( proto.has_thread_id() )
  {
    ConvertProtoToUUID( proto.thread_id(), &log->threadID );
  }
  log->dataIDs.resize( proto.data_ids_size() );
  for( int i = 0; i < proto.data_ids_size(); ++i )
  {
    ConvertProtoToUUID( proto.data_ids(i), &log->dataIDs[i] );
  }
  ConvertProtoToMonoTime( proto.queue_time_mono(), &log->queueTimeMono );
  if( proto.has_start_time_mono() )
  {
    ConvertProtoToMonoTime( proto.start_time_mono(), &log->startTimeMono );
  }
  if( proto.has_end_time_mono() )
  {
    ConvertProtoToMonoTime( proto.end_time_mono(), &log->endTimeMono );
  }
  log->status = static_cast<TaskLog::Status>( proto.status() );
  double t = time_to_float( log->startTimeMono );
  log->startTimeMono = float_to_time<MonotonicTime>( t );
}

}  // namespace ARF