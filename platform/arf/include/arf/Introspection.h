#pragma once

#include "arf/Types.h"
#include "util/string/stringUtils.h"
#include <vector>
#include <fstream>
#include <iostream>
#include <set>
#include <unordered_map>

namespace ARF
{

// Software for understanding system behavior at runtime
// TODO
// * Export Input/OutputPort connections for visualization
// * Build-in basic stat tracking for Ports, Tasks, Nodes
// * Events tracing UUIDs of data in time
// * Clock/time abstractions

// Implementation TODOs
// * Might want to look at Vector's logging implementation

// Introspection system design notes
// =================================
// System logs are separated into "data", "events", and "tasks"
// "Data" are, well, data products created by system components
// and passed through the system
// "Events" describe the things happening to data, as well as
// the relation of data to each other

// Events
// ======
// Describes things that happen to data as they flow through the system
// Should be atomic "deltas" that can be aggregated to form complete
// logs of object lifespans


class EventBase
{
public:

    EventBase();
    virtual ~EventBase() = default;

    MonotonicTime monoTime;      // When this event happened (monotonic)
    WallTime wallTime;           // When this event happened (wall)

    // Update the timestamps to current time
    void SetTimeToNow();
};

class DataEvent : public EventBase
{
public:

    enum Type
    {
        CREATION = 0, // Data was created
        MOVEMENT,     // Data moved between objects
        DESTRUCTION   // End of data lifespan
    };


    Type type;                   // Type of event
    UUID dataID;                 // ID of data in event
    std::vector<UUID> parentIDs; // For CREATION:    {parent ID(s)}
                                 // For MOVEMENT:    {from ID, to ID}
                                 // For DESTRUCTION: {sink ID}

    DataEvent() = default;

    // Create a new event pertaining to a specific datum
    template <typename T>
    DataEvent( const T& data )
    : dataID( data.GetUUID() ) {}

    // Indicates a dependency
    template <typename T>
    void AddParent( const T& tagged )
    {
        parentIDs.push_back( tagged.GetUUID() );
    }
};

std::ostream& operator<<( std::ostream& os, const DataEvent& event );

// Describes events relating to Task lifespans and dependencies
class TaskEvent : public EventBase
{
public:

    enum Type
    {
        PENDING = 0, // Task entered scheduling queue
        CANCELLED,   // Task cancelled while in queue
        STARTING,    // Task taken off queue and assigned to worker
        RUNNING,     // Task running and using data
        FINISHED,    // Task finished normally
        ERRORED,     // Task finished with error
    };

    Type type;                   // The event type
    UUID taskID;                 // ID of task
    std::vector<UUID> parentIDs; // For PENDING: {source ID}
                                 // For CANCELLED: {}
                                 // For STARTING: {worker ID}
                                 // For RUNNING: {data IDs}
                                 // For FINISHED: {}
                                 // For ERRORED: {}
};

// Describes events concerning system initialization
class InitEvent : public EventBase
{
public:

    enum Type
    {
        CREATE_NODE = 0,      // Creating a Node
        CREATE_INPUT_PORT,    // Creating an InputPort
        CREATE_OUTPUT_PORT,   // Creating an OutputPort
        CREATE_WORKER_THREAD, // Creating a worker thread in Threadpool
        CONNECT_PORTS,        // Connecting two ports
    };

    Type type;              // Type of event

    // For CREATE events
    UUID objectID;          // Created object's UUID
    std::string objectName; // For CREATE_NODE: User-specified Node name
                            // For CREATE_PORTs: Port index/name
                            // For CREATE_WORKER_THREAD: The system thread ID
    
    std::string dataType;   // For CREATE_PORT only, the type of data
    UUID parentID;          // For CREATE_PORT only, the owning Node UUID, if any

    // For CONNECT events
    UUID outputPortID;      // For CONNECT_PORTS only, the connected output port ID
    UUID inputPortID;       // For CONNECT_PORTS only, the connected input port ID
};

std::ostream& operator<<( std::ostream& os, const InitEvent& event );

// Logs
// ====
// Complete data of object lifespans

// Convenience object for comparing mono time members
struct MonoTimeComparator
{
    template <typename T>
    bool operator()( const T& a, const T& b ) const
    {
        return a.monoTime < b.monoTime;
    }
};

// Lifespan records for a single Data object
class DataLog
{
public:

    // Log representation of a Data's movement
    struct MovementLog
    {
        UUID fromID;
        UUID toID;
        MonotonicTime monoTime;
        WallTime wallTime;
    };

    UUID dataID;                           // ID of the data

    std::vector<UUID> creationParentIDs;   // IDs of sources that created the data
    MonotonicTime creationMonoTime;        // Time at which creation occurred
    WallTime creationWallTime;

    // Logs of all port read events ordered in time
    // Have to use a multiset as multiple moves may happen instantaneously
    std::multiset<MovementLog, MonoTimeComparator> movementLogs;

    // TODO Need to think more about destruction, since data will
    // commonly get copied when writing to multiple ports
    UUID destructionParentID;              // ID of Task/Node that destructed the data
    MonotonicTime destructionMonoTime;
    WallTime destructionWallTime;

    DataLog();

    // Adds an event's contents to this log
    // Returns false if the event does not match correctly
    bool ProcessEvent( const DataEvent& event ); 
};

// TODO Logs for Tasks

// Log declaring all Port, Node IDs captured from InitEvents
// TODO Capture connect events between ports?
class InitLog
{
public:

    struct NodeLog
    {
        UUID nodeID;
        std::string name;
        MonotonicTime initTimeMono;
        WallTime initTimeWall;
    };

    struct PortLog
    {
        UUID portID;
        UUID parentID;
        std::vector<UUID> connectedIDs;
        std::string name;
        std::string dataType;
        MonotonicTime initTimeMono;
        WallTime initTimeWall;
    };
    // TODO Declaration for worker thread

    std::vector<NodeLog> nodes;
    using PortMap = std::unordered_map<UUID, PortLog>;
    PortMap inputPorts;
    PortMap outputPorts;

    // Adds an event's contents to this log
    // Returns false if the event cannot be processed
    bool ProcessEvent( const InitEvent& event );
};

// Convenience classes for ID'ing things
// =====================================
// SFINAE trait for tags
template <typename T>
struct HasUUID
{
    // Compiler tests this first, if GetUUID valid then matches
    template <typename U>
    static char test( decltype(std::declval<U>().GetUUID(), int()) );

    // Else falls back to this version
    template<typename U>
    static long test(...);

public:

    enum { value = sizeof(test<T>(0)) == sizeof(char) };
};

// Wraps an item with a UUID 
template <typename T>
class TaggedItem
{
public:

    // TODO Setters/getters
    UUID uuid;
    T item;

    // TODO Ensure copy/move/assign do the right thing
    TaggedItem() : uuid() {}

    TaggedItem( const T& t ) 
    : uuid( Anki::Util::GetUUIDString() ), item( t ) {}

    // NOTE This seems to be contentions, so I'll leave it commented for now
    // template <typename... Args>
    // TaggedItem( Args&& ...args )
    // : uuid( boost::uuids::random_generator() )
    // , item( std::forward<Args>(args) )
    // {}

    const UUID& GetUUID() const
    {
        return uuid;
    }

    bool IsUUIDValid() const
    {
        return ~uuid.empty();
    }
};

// Logging
// =======
std::string to_string( const UUID& uuid );
// Commented out for now since UUIDs are std::strings
// std::ostream& operator<<( std::ostream& os, const UUID& uuid );

// Logger singleton for event streaming
// TODO Chunk logs into files
// TODO Copy Vector's per-thread logger cache to minimize lock contention
class Logger
{
public:

    // Checks to initialize the singleton object
    // Note that this is not thread-safe, and thus should be called from
    // the main thread before any tasks are started
    static Logger& Inst();

    bool Initialize( const std::string& logPath );
    bool IsInitialized() const;
    void Shutdown();

    void LogDataEvent( const DataEvent& event );
    void LogTaskEvent( const TaskEvent& log );
    void LogInitEvent( const InitEvent& init );

    size_t NumEventsLogged() const;

private:

    Logger();
    ~Logger();

    static Logger* _inst;

    mutable Mutex _mutex;
    std::ofstream _logStream;
    size_t _eventCount = 0;
};

// Helper functions that overload for having/not having UUIDs
// We assume that parents, when given, always have UUIDs
// Version for UUID'd types
template <typename T,
          typename std::enable_if<HasUUID<T>::value, int>::type  = 0>
void CreateAndLogEvent( const T& t, const std::vector<UUID>& parents, DataEvent::Type type )
{
    DataEvent event( t );
    event.type = type;
    event.parentIDs = parents;
    event.SetTimeToNow();
    Logger::Inst().LogDataEvent( event );
}

// Version for non-UUID'd types
template <typename T,
          typename std::enable_if<!HasUUID<T>::value, int>::type  = 0>
void CreateAndLogEvent( const T&, const std::vector<UUID>&, DataEvent::Type ) {}

// Mix-in to give IDs to objects
class TaggedMixin
{
public:

    TaggedMixin();

    const UUID& GetUUID() const;

private:

    UUID _uuid;
};

} // end namespace ARF
