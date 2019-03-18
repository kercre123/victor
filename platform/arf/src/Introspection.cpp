#include "arf/Introspection.h"
#include "arf/TypeHelpers.h"
#include "arf/datatypes_to_proto.h"
#include <sstream>
#include <iomanip>

namespace ARF
{

std::string to_string(const UUID &uuid)
{
    return uuid;
    // std::stringstream ss;
    // ss << std::hex << std::setfill ('0');
    // for( size_t i = 0; i < uuid.size(); i+=2 )
    // {
    //     ss << std::setw(2) << static_cast<int>(uuid[i]);
    // }
    // return ss.str();
}

std::ostream &operator<<(std::ostream &os, const DataEvent &event)
{
    switch (event.type)
    {
    case DataEvent::Type::CREATION:
    {
        os << "creation: ";
        break;
    }
    case DataEvent::Type::MOVEMENT:
    {
        os << "movement: ";
        break;
    }
    case DataEvent::Type::DESTRUCTION:
    {
        os << "destruction: ";
        break;
    }
    }
    os << to_string(event.dataID) << " time: " << event.monoTime;
    return os;
}

std::ostream &operator<<(std::ostream &os, const InitEvent &event)
{
    switch (event.type)
    {
    case InitEvent::Type::CREATE_NODE:
    {
        os << "create node: " << to_string(event.objectID)
           << " name: " << event.objectName;
        break;
    }
    case InitEvent::Type::CREATE_INPUT_PORT:
    {
        os << "create input port: " << to_string(event.objectID)
           << " name: " << event.objectName
           << " data: " << event.dataType
           << " parent: " << to_string(event.parentID);
        break;
    }
    case InitEvent::Type::CREATE_OUTPUT_PORT:
    {
        os << "create output port: " << to_string(event.objectID)
           << " name: " << event.objectName
           << " data: " << event.dataType
           << " parent: " << to_string(event.parentID);
        break;
    }
    case InitEvent::Type::CREATE_WORKER_THREAD:
    {
        os << "create worker thread: ";
        break;
    }
    case InitEvent::Type::CONNECT_PORTS:
    {
        os << "connect input port: " << to_string(event.inputPortID)
           << " to output port: " << to_string(event.outputPortID);
        break;
    }
    }
    return os;
}

TaggedMixin::TaggedMixin()
    : _uuid(Anki::Util::GetUUIDString())
{
}

const UUID &TaggedMixin::GetUUID() const
{
    return _uuid;
}

EventBase::EventBase()
    : monoTime(MonotonicTime::zero()), wallTime(WallTime::zero())
{
}

void EventBase::SetTimeToNow()
{
    monoTime = get_time<MonotonicClock>();
    wallTime = get_time<WallClock>();
}

DataLog::DataLog()
    : creationMonoTime(MonotonicTime::zero()), creationWallTime(WallTime::zero()), destructionMonoTime(MonotonicTime::zero()), destructionWallTime(WallTime::zero())
{
}

bool DataLog::ProcessEvent(const DataEvent &event)
{
    // If this log is uninitialized
    if (dataID.empty())
    {
        dataID = event.dataID;
    }

    // Bail if ID mismatched
    if (dataID != event.dataID)
    {
        std::cerr << "Data ID mismatch, expected: " << dataID << " got: " << event.dataID << std::endl;
        return false;
    }

    switch (event.type)
    {
    case DataEvent::Type::CREATION:
    {
        if (creationParentIDs.size() > 0)
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
        if (event.parentIDs.size() != 2)
        {
            std::cerr << "Received movement event with incorrect number of parents" << std::endl;
            return false;
        }
        MovementLog log;
        log.fromID = event.parentIDs[0];
        log.toID = event.parentIDs[1];
        log.monoTime = event.monoTime;
        log.wallTime = event.wallTime;
        movementLogs.insert(log);
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

bool InitLog::ProcessEvent(const InitEvent &event)
{
    switch (event.type)
    {
    case InitEvent::Type::CREATE_NODE:
    {
        NodeLog dec;
        dec.nodeID = event.objectID;
        dec.name = event.objectName;
        dec.initTimeMono = event.monoTime;
        dec.initTimeWall = event.wallTime;
        nodes.push_back(dec);
        break;
    }
    case InitEvent::Type::CREATE_INPUT_PORT:
    {
        PortLog &dec = inputPorts[event.objectID];
        dec.portID = event.objectID;
        dec.parentID = event.parentID;
        dec.name = event.objectName;
        dec.dataType = event.dataType;
        dec.initTimeMono = event.monoTime;
        dec.initTimeWall = event.wallTime;
        break;
    }
    case InitEvent::Type::CREATE_OUTPUT_PORT:
    {
        PortLog &dec = outputPorts[event.objectID];
        dec.portID = event.objectID;
        dec.parentID = event.parentID;
        dec.name = event.objectName;
        dec.dataType = event.dataType;
        dec.initTimeMono = event.monoTime;
        dec.initTimeWall = event.wallTime;
        break;
    }
    case InitEvent::Type::CREATE_WORKER_THREAD:
    {
        return false; // TODO
        break;
    }
    case InitEvent::Type::CONNECT_PORTS:
    {
        PortLog &inDec = inputPorts[event.inputPortID];
        PortLog &outDec = outputPorts[event.outputPortID];
        inDec.connectedIDs.push_back(event.outputPortID);
        outDec.connectedIDs.push_back(event.inputPortID);
        break;
    }
    default:
    {
        std::cerr << "Unrecognized event type!" << std::endl;
        return false;
    }
    }
    return true;
}

Logger *Logger::_inst = nullptr;

Logger::Logger() {}

Logger::~Logger()
{
    _logStream.close();
}

Logger &Logger::Inst()
{
    if (!_inst)
    {
        _inst = new Logger();
    }
    return *_inst;
}

bool Logger::Initialize(const std::string &logPath)
{
    Lock lock(_mutex);
    _logStream.open(logPath);
    _eventCount = 0;
    return _logStream.is_open();
}

bool Logger::IsInitialized() const
{
    Lock lock(_mutex);
    return _logStream.is_open();
}

void Logger::Shutdown()
{
    Lock lock(_mutex);
    _logStream.close();
}

void Logger::LogDataEvent(const DataEvent &event)
{
    Lock lock(_mutex);
    arf_proto::ArfEventItem proto;
    ConvertDataEventToProto(event, proto.mutable_data_event());
    stream_proto(proto, _logStream);
    std::cout << event << std::endl;
    _eventCount++;
}

void Logger::LogTaskEvent(const TaskEvent &event)
{
    Lock lock(_mutex);
    arf_proto::ArfEventItem proto;
    ConvertTaskEventToProto(event, proto.mutable_task_event());
    stream_proto(proto, _logStream);
    _eventCount++;
}

void Logger::LogInitEvent(const InitEvent &event)
{
    Lock lock(_mutex);
    arf_proto::ArfEventItem proto;
    ConvertInitEventToProto(event, proto.mutable_init_event());
    stream_proto(proto, _logStream);
    _eventCount++;
}

size_t Logger::NumEventsLogged() const
{
    Lock lock(_mutex);
    return _eventCount;
}

} // namespace ARF