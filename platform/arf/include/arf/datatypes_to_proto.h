#pragma once

#include "arf/Types.h"
#include "arf/Introspection.h"

#include "Time.pb.h"
#include "Events.pb.h"
#include "UUID.pb.h"
#include "Logs.pb.h"

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
void ConvertTaskEventToProto(const TaskEvent& log,
                           arf_proto::TaskEvent* proto);
void ConvertProtoToTaskEvent(const arf_proto::TaskEvent& proto,
                           TaskEvent* log);

void ConvertDataLogToProto(const DataLog& log,
                           arf_proto::DataLog* proto);
void ConvertProtoToDataLog(const arf_proto::DataLog& proto,
                           DataLog* log);
}  // namespace ARF