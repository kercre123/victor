#pragma once

#include <vector>
#include <fstream>
#include <iostream>
#include <set>

#include "arf/Types.h"
#include "util/string/stringUtils.h"

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
class DataEvent
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
    MonotonicTime monoTime;      // When this event happened (monotonic)
    WallTime wallTime;           // When this event happened (wall)

    DataEvent();

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

    // Update the timestamps to current time
    void SetTimeToNow();
};

// Describes the lifespan of a Task and its dependencies
class TaskEvent
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

    MonotonicTime monoTime;      // When this event happened (monotonic)
    WallTime wallTime;           // When this event happened (wall)
};

// TODO Events declaring Port/Node IDs

// Logs
// ====
// Complete data of object lifespans

// Convenience object for comparing mono time members
struct MonoTimeComparator
{
    template <typename T>
    bool operator() ( const T& a, const T& b ) const
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
// TODO Logs declaring all Port, Node IDs

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
// TODO Move streaming/destreaming functionality to a different file

template <typename T>
int32_t stream_proto( const T& proto, std::ostream& os )
{
    int32_t size = proto.ByteSizeLong();
    os.write( reinterpret_cast<char*>( &size ), sizeof( size ) );
    proto.SerializeToOstream( &os );
    return size;
}

std::string to_string( const UUID& uuid );
std::ostream& operator<<( std::ostream& os, const UUID& uuid );

// Logger singleton for event streaming
// TODO Chunk logs into files
// TODO Copy Vector's per-thread logger cache to minimize lock contention
class Logger
{
public:

    static Logger& Inst();

    bool Initialize( const std::string& logPath );
    bool IsInitialized() const;
    void Shutdown();

    void LogDataEvent( const DataEvent& event );
    void LogTaskEvent( const TaskEvent& log );

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
void CreateAndLogEvent( const T& t, const std::vector<const UUID*>& parents, DataEvent::Type type )
{
    DataEvent event( t );
    event.type = type;
    for( auto id : parents )
    {
        if( id )
        {
            event.parentIDs.emplace_back( *id );
        }
        else
        {
            event.parentIDs.emplace_back( UUID() );
        }
        
    }
    event.SetTimeToNow();
    Logger::Inst().LogDataEvent( event );
}

// Version for non-UUID'd types
template <typename T,
          typename std::enable_if<!HasUUID<T>::value, int>::type  = 0>
void CreateAndLogEvent( const T&, const std::vector<const UUID*>&, DataEvent::Type ) {}

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
