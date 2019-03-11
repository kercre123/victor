#pragma once

#include "arf/Types.h"
#include "arf/Intraprocess.h"
#include "arf/Introspection.h"
#include <unordered_map>

// Software for defining data processing modules that interface through
// inter/intraprocess Ports

namespace ARF
{

// ====================
// Forward declarations
// ====================
class PortBase;
class TaskTracker;

// =========
// ARF Nodes
// =========
// Base class for all computational modules operating in ARF 
// Node should be able to operate asynchronously from each other,
// separated by points of intra/inter-process communications or "Ports"
//
// Nodes can be assigned to manage Ports and TaskTrackers for convenience.
// To ensure proper cleanup, inheriting classes should call Shutdown
// in their destructor to wait until all managed Tasks are done before
// cleaning up all managed Ports and Node resources.

class Node : public TaggedMixin
{
public:

    Node();

    ~Node();

    // Cancel all pending Tasks, wait until all active Tasks are done,
    // and then shutdown all Ports
    // Should be called in the derived Node's destructor to ensure resources
    // stay valid
    void Shutdown();

    // Retrieve an indexed InputPort for this Node
    // Returns null if no Port has been registered at the index, or if
    // there is a type mismatch
    template <typename T>
    std::shared_ptr<InputPort<T>> RetrieveInputPort( int index ) const
    {
        Lock lock( _mutex );
        PortMap::const_iterator iter = _portMap.find( index );
        if( iter == _portMap.end() ) 
        { 
            return nullptr;
        }
        return std::dynamic_pointer_cast<InputPort<T>>( iter->second );
    }

    // Retrieve an indexed OutputPort for this Node
    // Returns null if no Port has been registered at the index, or if
    // there is a type mismatch
    template <typename T>
    std::shared_ptr<OutputPort<T>> RetrieveOutputPort( int index ) const
    {
        Lock lock( _mutex );
        PortMap::const_iterator iter = _portMap.find( index );
        if( iter == _portMap.end() )
        {
            return nullptr;
        }
        return std::dynamic_pointer_cast<OutputPort<T>>( iter->second );
    }

    // Assigns a port to an index and gives management responsibility to this Node
    template <typename PortT>
    bool RegisterPort( const PortT& p, int index )
    {
        Lock lock( _mutex );
        PortBase::Ptr upcastP = std::static_pointer_cast<PortBase>( p );
        if( _portMap.count( index ) > 0 )
        {
            std::cerr << "Port index " << index << " used twice!" << std::endl;
            return false;
        }
        _portMap[index] = upcastP;
        return true;
    }

    // Assigns a tracker and its corresponding tasks to be managed
    void RegisterTasks( const std::shared_ptr<TaskTracker>& tasks );

private:

    mutable Mutex _mutex;
    std::vector<std::shared_ptr<TaskTracker>> _taskTracks;
    using PortMap = std::unordered_map<int, std::shared_ptr<PortBase>>;
    PortMap _portMap;
};

} // end namespace ARF
