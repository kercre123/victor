#include "arf/datatypes_to_proto.h"

namespace ARF
{

void ConvertWallTimeToProto(const WallTime &time,
                            arf_proto::Time *proto)
{
  TimeParts parts = time_to_parts(time);
  proto->set_seconds(parts.secs);
  proto->set_nanoseconds(parts.nanosecs);
}

void ConvertProtoToWallTime(const arf_proto::Time &proto,
                            WallTime *time)
{
  *time = time_from_parts<WallTime>(proto.seconds(), proto.nanoseconds());
}

void ConvertMonoTimeToProto(const MonotonicTime &time,
                            arf_proto::Time *proto)
{
  TimeParts parts = time_to_parts(time);
  proto->set_seconds(parts.secs);
  proto->set_nanoseconds(parts.nanosecs);
}

void ConvertProtoToMonoTime(const arf_proto::Time &proto,
                            MonotonicTime *time)
{
  *time = time_from_parts<WallTime>(proto.seconds(), proto.nanoseconds());
}

void ConvertUUIDToProto(const UUID &uuid,
                        arf_proto::UUID *proto)
{
  proto->set_uuid(uuid.c_str(), uuid.length());
}

void ConvertProtoToUUID(const arf_proto::UUID &proto,
                        UUID *uuid)
{
  *uuid = proto.uuid().data();
}

void ConvertDataEventToProto(const DataEvent &event,
                             arf_proto::DataEvent *proto)
{
  proto->set_type(static_cast<arf_proto::DataEvent_Type>(event.type));
  ConvertUUIDToProto(event.dataID, proto->mutable_data_id());
  proto->clear_parent_ids();
  for (const UUID &id : event.parentIDs)
  {
    arf_proto::UUID *protoID = proto->add_parent_ids();
    ConvertUUIDToProto(id, protoID);
  }
  ConvertMonoTimeToProto(event.monoTime, proto->mutable_mono_time());
  ConvertWallTimeToProto(event.wallTime, proto->mutable_wall_time());
}

void ConvertProtoToDataEvent(const arf_proto::DataEvent &proto,
                             DataEvent *event)
{
  event->type = static_cast<DataEvent::Type>(proto.type());
  ConvertProtoToUUID(proto.data_id(), &event->dataID);
  event->parentIDs.resize(proto.parent_ids_size());
  for (int i = 0; i < proto.parent_ids_size(); ++i)
  {
    ConvertProtoToUUID(proto.parent_ids(i), &event->parentIDs[i]);
  }
  ConvertProtoToMonoTime(proto.mono_time(), &event->monoTime);
  ConvertProtoToWallTime(proto.wall_time(), &event->wallTime);
}

void ConvertTaskEventToProto(const TaskEvent &log,
                             arf_proto::TaskEvent *proto)
{
  proto->set_type(static_cast<arf_proto::TaskEvent_Type>(log.type));
  ConvertUUIDToProto(log.taskID, proto->mutable_task_id());
  proto->clear_parent_ids();
  for (const UUID &id : log.parentIDs)
  {
    arf_proto::UUID *protoID = proto->add_parent_ids();
    ConvertUUIDToProto(id, protoID);
  }
  ConvertMonoTimeToProto(log.monoTime, proto->mutable_mono_time());
  ConvertWallTimeToProto(log.wallTime, proto->mutable_wall_time());
}

void ConvertProtoToTaskEvent(const arf_proto::TaskEvent &proto,
                             TaskEvent *log)
{
  log->type = static_cast<TaskEvent::Type>(proto.type());
  ConvertProtoToUUID(proto.task_id(), &log->taskID);
  log->parentIDs.resize(proto.parent_ids_size());
  for (int i = 0; i < proto.parent_ids_size(); ++i)
  {
    ConvertProtoToUUID(proto.parent_ids(i), &log->parentIDs[i]);
  }
  ConvertProtoToMonoTime(proto.mono_time(), &log->monoTime);
  ConvertProtoToWallTime(proto.wall_time(), &log->wallTime);
}

void ConvertInitEventToProto(const InitEvent &event,
                             arf_proto::InitEvent *proto)
{
  proto->set_type(static_cast<arf_proto::InitEvent_Type>(event.type));
  ConvertMonoTimeToProto(event.monoTime, proto->mutable_mono_time());
  ConvertWallTimeToProto(event.wallTime, proto->mutable_wall_time());

  if (event.type == InitEvent::Type::CONNECT_PORTS)
  {
    ConvertUUIDToProto(event.outputPortID, proto->mutable_output_port_id());
    ConvertUUIDToProto(event.inputPortID, proto->mutable_input_port_id());
  }
  else
  {
    ConvertUUIDToProto(event.objectID, proto->mutable_object_id());
    if (!event.objectName.empty())
    {
      proto->set_object_name(event.objectName);
    }
    if (!event.dataType.empty())
    {
      proto->set_data_type(event.dataType);
    }
    if (!event.parentID.empty())
    {
      ConvertUUIDToProto(event.parentID, proto->mutable_parent_id());
    }
  }
}

void ConvertProtoToInitEvent(const arf_proto::InitEvent &proto,
                             InitEvent *event)
{
  event->type = static_cast<InitEvent::Type>(proto.type());
  if (proto.has_object_id())
  {
    ConvertProtoToUUID(proto.object_id(), &event->objectID);
  }
  if (proto.has_object_name())
  {
    event->objectName = proto.object_name();
  }
  if (proto.has_data_type())
  {
    event->dataType = proto.data_type();
  }
  if (proto.has_parent_id())
  {
    ConvertProtoToUUID(proto.parent_id(), &event->parentID);
  }
  if (proto.has_output_port_id())
  {
    ConvertProtoToUUID(proto.output_port_id(), &event->outputPortID);
  }
  if (proto.has_input_port_id())
  {
    ConvertProtoToUUID(proto.input_port_id(), &event->inputPortID);
  }
  ConvertProtoToMonoTime(proto.mono_time(), &event->monoTime);
  ConvertProtoToWallTime(proto.wall_time(), &event->wallTime);
}

// Helper functions for DataLog hidden from users
void ConvertMovementLogToProto(const DataLog::MovementLog &log,
                               arf_proto::DataLog::MovementLog *proto)
{
  ConvertUUIDToProto(log.fromID, proto->mutable_from_id());
  ConvertUUIDToProto(log.toID, proto->mutable_to_id());
  ConvertMonoTimeToProto(log.monoTime, proto->mutable_mono_time());
  ConvertWallTimeToProto(log.wallTime, proto->mutable_wall_time());
}

void ConvertProtoToMovementLog(const arf_proto::DataLog::MovementLog &proto,
                               DataLog::MovementLog *log)
{
  ConvertProtoToUUID(proto.from_id(), &log->fromID);
  ConvertProtoToUUID(proto.to_id(), &log->toID);
  ConvertProtoToMonoTime(proto.mono_time(), &log->monoTime);
  ConvertProtoToWallTime(proto.wall_time(), &log->wallTime);
}

void ConvertDataLogToProto(const DataLog &log,
                           arf_proto::DataLog *proto)
{
  ConvertUUIDToProto(log.dataID, proto->mutable_data_id());
  for (const UUID &id : log.creationParentIDs)
  {
    arf_proto::UUID *pid = proto->add_creation_parent_ids();
    ConvertUUIDToProto(id, pid);
  }
  if (time_is_valid(log.creationMonoTime))
  {
    ConvertMonoTimeToProto(log.creationMonoTime, proto->mutable_creation_mono_time());
  }
  if (time_is_valid(log.creationWallTime))
  {
    ConvertWallTimeToProto(log.creationWallTime, proto->mutable_creation_wall_time());
  }
  for (const DataLog::MovementLog &l : log.movementLogs)
  {
    arf_proto::DataLog::MovementLog *plog = proto->add_movement_logs();
    ConvertMovementLogToProto(l, plog);
  }
  if (log.destructionParentID.empty())
  {
    ConvertUUIDToProto(log.destructionParentID, proto->mutable_destruction_parent_id());
  }
  if (time_is_valid(log.destructionMonoTime))
  {
    ConvertMonoTimeToProto(log.destructionMonoTime, proto->mutable_destruction_mono_time());
  }
  if (time_is_valid(log.destructionWallTime))
  {
    ConvertWallTimeToProto(log.destructionWallTime, proto->mutable_destruction_wall_time());
  }
}

void ConvertProtoToDataLog(const arf_proto::DataLog &proto,
                           DataLog *log)
{
  ConvertProtoToUUID(proto.data_id(), &log->dataID);
  log->creationParentIDs.resize(proto.creation_parent_ids_size());
  for (int i = 0; i < proto.creation_parent_ids_size(); ++i)
  {
    ConvertProtoToUUID(proto.creation_parent_ids(i), &log->creationParentIDs[i]);
  }
  if (proto.has_creation_mono_time())
  {
    ConvertProtoToMonoTime(proto.creation_mono_time(), &log->creationMonoTime);
  }
  if (proto.has_creation_wall_time())
  {
    ConvertProtoToWallTime(proto.creation_wall_time(), &log->creationWallTime);
  }
  DataLog::MovementLog mlog;
  log->movementLogs.clear();
  for (int i = 0; i < proto.movement_logs_size(); ++i)
  {
    ConvertProtoToMovementLog(proto.movement_logs(i), &mlog);
    log->movementLogs.insert(mlog);
  }
  if (proto.has_destruction_parent_id())
  {
    ConvertProtoToUUID(proto.destruction_parent_id(), &log->destructionParentID);
  }
  if (proto.has_destruction_mono_time())
  {
    ConvertProtoToMonoTime(proto.destruction_mono_time(), &log->destructionMonoTime);
  }
  if (proto.has_destruction_wall_time())
  {
    ConvertProtoToWallTime(proto.destruction_wall_time(), &log->destructionWallTime);
  }
}

// Helper functions for InitLog hidden from user
void ConvertNodeLogToProto(const InitLog::NodeLog &log,
                           arf_proto::InitLog::NodeLog *proto)
{
  ConvertUUIDToProto(log.nodeID, proto->mutable_node_id());
  if (!log.name.empty())
  {
    proto->set_name(log.name);
  }
  ConvertMonoTimeToProto(log.initTimeMono, proto->mutable_init_time_mono());
  ConvertWallTimeToProto(log.initTimeWall, proto->mutable_init_time_wall());
}

void ConvertProtoToNodeLog(const arf_proto::InitLog::NodeLog &proto,
                           InitLog::NodeLog *log)
{
  ConvertProtoToUUID(proto.node_id(), &log->nodeID);
  if (proto.has_name())
  {
    log->name = proto.name();
  }
  ConvertProtoToMonoTime(proto.init_time_mono(), &log->initTimeMono);
  ConvertProtoToWallTime(proto.init_time_wall(), &log->initTimeWall);
}

void ConvertPortLogToProto(const InitLog::PortLog &log,
                           arf_proto::InitLog::PortLog *proto)
{
  ConvertUUIDToProto(log.portID, proto->mutable_port_id());
  if (!log.parentID.empty())
  {
    ConvertUUIDToProto(log.parentID, proto->mutable_parent_id());
  }
  proto->clear_connected_ids();
  for (const UUID &id : log.connectedIDs)
  {
    arf_proto::UUID *pID = proto->add_connected_ids();
    ConvertUUIDToProto(id, pID);
  }
  if (!log.name.empty())
  {
    proto->set_name(log.name);
  }
  proto->set_data_type(log.dataType);
  ConvertMonoTimeToProto(log.initTimeMono, proto->mutable_init_time_mono());
  ConvertWallTimeToProto(log.initTimeWall, proto->mutable_init_time_wall());
}

void ConvertProtoToPortLog(const arf_proto::InitLog::PortLog &proto,
                           InitLog::PortLog *log)
{
  ConvertProtoToUUID(proto.port_id(), &log->portID);
  if (proto.has_parent_id())
  {
    ConvertProtoToUUID(proto.parent_id(), &log->parentID);
  }
  log->connectedIDs.resize(proto.connected_ids_size());
  for (int i = 0; i < proto.connected_ids_size(); ++i)
  {
    ConvertProtoToUUID(proto.connected_ids(i), &log->connectedIDs[i]);
  }
  if (proto.has_name())
  {
    log->name = proto.name();
  }
  log->dataType = proto.data_type();
  ConvertProtoToMonoTime(proto.init_time_mono(), &log->initTimeMono);
  ConvertProtoToWallTime(proto.init_time_wall(), &log->initTimeWall);
}

void ConvertInitLogToProto(const InitLog &log,
                           arf_proto::InitLog *proto)
{
  proto->Clear();
  for (const InitLog::NodeLog &l : log.nodes)
  {
    arf_proto::InitLog::NodeLog *pLog = proto->add_nodes();
    ConvertNodeLogToProto(l, pLog);
  }
  for (auto &item : log.inputPorts)
  {
    arf_proto::InitLog::PortLog *pLog = proto->add_input_ports();
    ConvertPortLogToProto(item.second, pLog);
  }
  for (auto &item : log.outputPorts)
  {
    arf_proto::InitLog::PortLog *pLog = proto->add_output_ports();
    ConvertPortLogToProto(item.second, pLog);
  }
}

void ConvertProtoToInitLog(const arf_proto::InitLog &proto,
                           InitLog *log)
{
  log->nodes.resize(proto.nodes_size());
  for (int i = 0; i < proto.nodes_size(); ++i)
  {
    ConvertProtoToNodeLog(proto.nodes(i), &log->nodes[i]);
  }
  log->inputPorts.clear();
  for (int i = 0; i < proto.input_ports_size(); ++i)
  {
    const arf_proto::InitLog::PortLog &pLog = proto.input_ports(i);
    UUID portID;
    ConvertProtoToUUID(pLog.port_id(), &portID);
    ConvertProtoToPortLog(pLog, &log->inputPorts[portID]);
  }
  log->outputPorts.clear();
  for (int i = 0; i < proto.output_ports_size(); ++i)
  {
    const arf_proto::InitLog::PortLog &pLog = proto.output_ports(i);
    UUID portID;
    ConvertProtoToUUID(pLog.port_id(), &portID);
    ConvertProtoToPortLog(pLog, &log->outputPorts[portID]);
  }
}

} // namespace ARF