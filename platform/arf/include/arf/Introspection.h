#pragma once

#include "arf/Types.h"
#include "util/string/stringUtils.h"
#include <vector>
#include <fstream>
#include <iostream>

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
// "Tasks" describe computation involving "Data"

// Describes things that happen to data as they flow through the system
class DataEvent
{
public:

    enum Type
    {
        CREATION = 0, // Data was created with no parents
        FORK,         // Data was created/derived from parent data
        PORT_WRITE,   // Data went from OutputPort to InputPort
        PORT_READ,    // Data read from InputPort
        DESTRUCTION   // End of data lifespan
    };

    Type type;                   // Type of event
    UUID dataID;                 // ID of data in event
    std::vector<UUID> parentIDs; // For FORK: {parent data ID}
                                 // PORT_WRITE: {OutputPort ID, InputPort ID}
                                 // PORT_READ: {InputPort ID}
                                 // DESTRUCTION: {InputPort ID or Node ID}
    MonotonicTime monoTime;      // When this event happened (monotonic)
    WallTime wallTime;           // When this event happened (wall)

    // Create a new event pertaining to a specific datum
    template <typename T>
    DataEvent( const T& data )
    : dataID( data.GetUUID() ) {}

    // Indicates a dependency
    template <typename T>
    void AddParent( const T& tagged )
    {
        parentIDs.emplace_back( tagged.GetUUID() );
    }

    // Update the timestamps to current time
    void SetTimeToNow();
};

// Describes the lifespan of a Task and its dependencies
class TaskLog
{
public:

    enum Status
    {
        PENDING = 0,
        FINISHED,
        CANCELLED,
        ERRORED,
    };

    // IDs of relevant... things
    UUID taskID;                 // ID of task
    UUID sourceID;               // ID of task originator
    UUID threadID;               // ID of thread task ran on (nil if not run)
    std::vector<UUID> dataIDs;   // IDs of data dependencies for this task

    // TODO Do we want wall times too?
    MonotonicTime queueTimeMono; // When this task entered the scheduling queue
    MonotonicTime startTimeMono; // When this task was assigned a thread and began
    MonotonicTime endTimeMono;   // When/if this task finished/cancelled/errored (nil if pending)
    Status status;               // The current status of the task

    TaskLog(); // TODO
};

// Convenience classes for ID'ing things

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

// TODO Copy Vector's per-thread logger cache to minimize lock contention
class Logger
{
public:

    static Logger& Inst();

    bool Initialize( const std::string& logPath );
    bool IsInitialized() const;
    void Shutdown();

    void LogDataEvent( const DataEvent& event );
    void LogTaskLog( const TaskLog& log );

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
template <typename P, typename T,
          typename std::enable_if<HasUUID<T>::value, int>::type  = 0>
void CreateAndLogEvent( const T& t, const std::vector<P*>& parents, DataEvent::Type type )
{
    DataEvent event( t );
    event.type = type;
    for( auto p : parents )
    {
        event.AddParent( *p );
    }
    event.SetTimeToNow();
    Logger::Inst().LogDataEvent( event );
}

// Version for non-UUID'd types
template <typename P, typename T,
            typename std::enable_if<!HasUUID<T>::value, int>::type  = 0>
void CreateAndLogEvent( const T&, const std::vector<P*>& /*parents*/, DataEvent::Type ) {}

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
