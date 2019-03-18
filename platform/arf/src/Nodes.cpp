#include "arf/Nodes.h"
#include "arf/Threads.h"

namespace ARF
{

Node::Node( const std::string& name ) 
: _name( name )
{
    InitEvent event;
    event.type = InitEvent::Type::CREATE_NODE;
    event.objectID = GetUUID();
    event.objectName = name;
    event.SetTimeToNow();
    Logger::Inst().LogInitEvent( event );
}

Node::~Node()
{
    Shutdown();
}

void Node::RegisterTasks( const TaskTracker::Ptr& t )
{
    Lock lock( _mutex );
    _taskTracks.emplace_back( t );
}

void Node::Shutdown()
{
    Lock lock( _mutex );
    // Cancel and wait for all tasks first before destructing ports
    for( const TaskTracker::Ptr& t : _taskTracks )
    {
        t->Cancel();
    }
    for( const TaskTracker::Ptr& t : _taskTracks )
    {
        t->WaitUntilAllDone();
    }

    for( auto& item : _portMap )
    {
        item.second->Shutdown();
    }
}

} // end namespace ARF
