#pragma once

#include "arf/Types.h"
#include "arf/Introspection.h"

#include "generated/proto/arf/Time.pb.h"
#include "generated/proto/arf/Events.pb.h"
#include "generated/proto/arf/UUID.pb.h"

namespace ARF {
void ConvertWallTimeToProto(const WallTime& time,
                            arf_proto::Time* proto);
void ConvertProtoToWallTime(const arf_proto::Time& proto,
                            WallTime* time);
void ConvertMonoTimeToProto(const MonotonicTime& time,
                            arf_proto::Time* proto);
void ConvertProtoToMonoTime(const arf_proto::Time& proto,
                            MonotonicTime* time);

void ConvertUUIDToProto(const UUID& uuid,
                        arf_proto::UUID* proto);
void ConvertProtoToUUID(const arf_proto::UUID& proto,
                        UUID* uuid);

void ConvertDataEventToProto(const DataEvent& event,
                             arf_proto::DataEvent* proto);
void ConvertProtoToDataEvent(const arf_proto::DataEvent& proto,
                             DataEvent* event);
void ConvertTaskLogToProto(const TaskLog& log,
                           arf_proto::TaskLog* proto);
void ConvertProtoToTaskLog(const arf_proto::TaskLog& proto,
                           TaskLog* log);
}  // namespace ARF