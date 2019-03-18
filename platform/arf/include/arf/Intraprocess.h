#pragma once

#include <thread>
#include <vector>
#include <functional>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <memory>

#include <iostream>

#include "util/container/circularBuffer.h"
#include "arf/Types.h"
#include "arf/TypeHelpers.h"
#include "arf/Introspection.h"

// Intraprocess communication ports

// Comment this out to optimize out logging
#define ENABLE_PORT_LOGGING (1)

namespace ARF
{

// =========
// ARF Ports
// =========
//
// Ports are the points of data input/output between Nodes
// Each port is typed to its corresponding data type
//
// Design Notes
// ============
// Port methods must be atomic to minimize lock contention and avoid deadlock
// A method can never leave the port in a locked state, nor can the
// lock be externally accessed
//
// Ports use copy by value for all data
// As such, it is recommended to use pointers or reference counting
// wrappers for large data types
//
// Ports store sets of smart pointers (specifically, std::shared_ptr)
// to their connected ports so they can disconnect themselves upon destruction
// Smart pointer semantics guarantee that each ports 1. will be destructed
// exactly once, and 2. will not be destructed while pointers to the port
// are still held.

// ====================
// Forward declarations
// ====================
template <typename T>
class OutputPort;

template <typename T>
class InputPort;

template <typename DerivedOutputT, typename DerivedInputT>
void connect_ports(const DerivedOutputT &out, const DerivedInputT &in);

template <typename DerivedOutputT, typename DerivedInputT>
void disconnect_ports(const DerivedOutputT &out, const DerivedInputT &in);

// ===========
// Definitions
// ===========
// Base class to inherit from for dynamic casts
class PortBase : public TaggedMixin
{
  public:
    using Ptr = std::shared_ptr<PortBase>;

    PortBase() = default;
    virtual ~PortBase() = default;

    // Disconnect this port from all its connections
    virtual void Shutdown() = 0;
};

// This passkey is used for managing connections between ports
template <typename T>
class PortConnectionKey
{
    friend class OutputPort<T>;
    friend class InputPort<T>;

    template <typename DerivedOutputT, typename DerivedInputT>
    friend void connect_ports(const DerivedOutputT &out, const DerivedInputT &in);

    template <typename DerivedOutputT, typename DerivedInputT>
    friend void disconnect_ports(const DerivedOutputT &out, const DerivedInputT &in);

    PortConnectionKey() {}
};

// This passkey allows OutputPort to push data to InputPort
template <typename T>
class PortWriteKey
{
    friend class OutputPort<T>;

    PortWriteKey() {}
};

// TODO Move utility functions
template <typename Container, typename Item, typename Comparison>
void remove_from_set(const Item &item, Container &container, const Comparison &comp)
{
    typename Container::iterator iter;
    for (iter = container.begin(); iter != container.end(); ++iter)
    {
        if (comp(*iter, item))
        {
            container.erase(iter);
            return;
        }
    }
}

struct SharedPtrComparison
{
    template <typename T>
    bool operator()(const std::shared_ptr<T> &a, T *b) const
    {
        return a.get() == b;
    }
};

// Base class and interface for ports that receive data
template <typename T>
class InputPort : public PortBase
{
  public:
    using DataType = T;
    using Ptr = std::shared_ptr<InputPort<T>>;
    using EventCallback = std::function<void(InputPort<T> *)>;

    // Create an input port with specified buffer size
    // Takes an optional name, which is used for introspection
    InputPort(size_t buffSize, const std::string &name = "", const UUID &parentID = "")
        : PortBase(), _buffer(buffSize)
    {
        InitEvent event;
        event.type = InitEvent::Type::CREATE_INPUT_PORT;
        event.objectID = PortBase::GetUUID();
        event.objectName = name;
        event.parentID = parentID;
        event.dataType = get_type_name<T>();
        event.SetTimeToNow();
        Logger::Inst().LogInitEvent(event);
    }

    // Cleans up an input port by disconnecting from all connected OutputPorts
    virtual ~InputPort()
    {
        Shutdown();
    }

    // Disconnects from all connections
    // This method must be called during shutdown to avoid a memory leak!
    // TODO Have all ports managed by a parent object to handle cleanup correctly
    void Shutdown() override
    {
        Lock lock(_mutex);
        for (typename OutputPort<T>::Ptr source : _connections)
        {
            source->Disconnect(this, PortConnectionKey<T>());
        }

        // NOTE We do not have to release the lock here. Even though
        // clearing _connections may trigger destructors for formerly linked
        // OutputPorts, they should no longer have pointers to this InputPort
        // after the previous for loop
        _connections.clear();

// Record destruction of buffered items
#ifdef ENABLE_PORT_LOGGING
        for (size_t i = 0; i < _buffer.size(); ++i)
        {
            CreateAndLogEvent(_buffer[i], {GetUUID()}, DataEvent::Type::DESTRUCTION);
        }
        _buffer.clear();
#endif
    }

    // Attempt to retrieve an item from the port
    // Returns true if an item is retrieved, or false if not
    // Takes an optional UUID for the sink reading the data
    bool Read(T &item, const UUID &uuid = "")
    {
        Lock lock(_bufferMutex);
        if (_buffer.empty())
        {
            return false;
        }

        item = _buffer.front();
        _buffer.pop_front();

#ifdef ENABLE_PORT_LOGGING
        CreateAndLogEvent(item, {GetUUID(), uuid}, DataEvent::Type::MOVEMENT);
#endif

        return true;
    }

    // Register a callback to be run on each data input event
    void RegisterCallback(EventCallback &cb)
    {
        Lock lock(_mutex);
        _callbacks.push_back(cb);
    }

    size_t NumConnections() const
    {
        Lock lock(_mutex);
        return _connections.size();
    }

    size_t Size() const
    {
        Lock lock(_bufferMutex);
        return _buffer.size();
    }

    // =================================
    // Methods protected by PortWriteKey
    // =================================
    // Offers data to the port, potentially triggering the owning node
    void Enqueue(const T &item, PortWriteKey<T>)
    {
        Lock lock(_bufferMutex);

// Record destruction of data items from buffer
#ifdef ENABLE_PORT_LOGGING
        if (_buffer.capacity() == _buffer.size())
        {
            CreateAndLogEvent(_buffer.front(), {GetUUID()}, DataEvent::Type::DESTRUCTION);
        }
#endif

        _buffer.push_back(item);
        lock.unlock();

        // NOTE Important to unlock port before triggering callbacks,
        // as callbacks may read from the port
        TriggerEvent();
    }

    // ======================================
    // Methods protected by PortConnectionKey
    // ======================================
    // Connect this input port to the specified output
    // If the port is already connected, does nothing
    void Connect(typename OutputPort<T>::Ptr &source, PortConnectionKey<T>)
    {
        Lock lock(_mutex);
        _connections.insert(source);
    }

    // Disconnect from an OutputPort
    // If the port is not actually connected, does nothing
    void Disconnect(OutputPort<T> *source, PortConnectionKey<T>)
    {
        Lock lock(_mutex);
        SharedPtrComparison equal;
        remove_from_set(source, _connections, equal);
    }

  protected:
    mutable Mutex _mutex;                  // Mutex to protect vector of callbacks
    std::vector<EventCallback> _callbacks; // All registered callbacks
    using ConnectionSet = std::unordered_set<typename OutputPort<T>::Ptr>;
    ConnectionSet _connections; // All connected OutputPorts

    mutable Mutex _bufferMutex; // Mutex to protect item buffer
    Anki::Util::CircularBuffer<T> _buffer;

    // Trigger an event, running all registered callbacks
    // This method should be called by the derived class after data
    // is available for reading
    void TriggerEvent()
    {
        Lock lock(_mutex);
        // TODO Potential for deadlock here if a callback loops back to this port
        // Could solve this by making a copy of _callbacks, but not be worth the overhead
        for (EventCallback &cb : _callbacks)
        {
            cb(this);
        }
    }
};

// Base class and interface for ports that produce data
// At the moment there aren't any derived instances of OutputPort,
// but it is conceivable in the future for explicit interprocess
template <typename T>
class OutputPort : public PortBase
{
  public:
    using DataType = T;
    using Ptr = std::shared_ptr<OutputPort<T>>;

    // Create an OutputPort
    // Optionally takes a name used for introspection
    OutputPort(const std::string &name = "", const std::string &parentID = "")
    {
        // TODO How to get parent name into here?
        InitEvent event;
        event.type = InitEvent::Type::CREATE_OUTPUT_PORT;
        event.objectID = PortBase::GetUUID();
        event.objectName = name;
        event.parentID = parentID;
        event.dataType = get_type_name<T>();
        event.SetTimeToNow();
        Logger::Inst().LogInitEvent(event);
    }

    // Clean up the port by disconnecting from all connected InputPorts
    ~OutputPort()
    {
        Shutdown();
    }

    // Disconnects from all connections
    // This method must be called during shutdown to avoid a memory leak!
    // TODO Have all ports managed by a parent object to handle cleanup correctly
    void Shutdown() override
    {
        Lock lock(_mutex);
        for (typename InputPort<T>::Ptr sink : _connections)
        {
            sink->Disconnect(this, PortConnectionKey<T>());
        }
        // NOTE We do not have to release the lock here. Even though
        // clearing _connections may trigger destructors for formerly linked
        // InputPorts, they should no longer have pointers to this OutputPort
        // after the previous for loop
        _connections.clear();
    }

    // Passes an item by reference through the port
    // This will immediately push data to the connected InputPorts
    // Takes an optional source UUID, if not specified will use nil
    void Write(const T &item, const UUID &source = "")
    {
#ifdef ENABLE_PORT_LOGGING
        CreateAndLogEvent(item, {source, GetUUID()}, DataEvent::Type::MOVEMENT);
#endif

        // Enqueue the data
        Lock lock(_mutex);
        for (typename InputPort<T>::Ptr s : _connections)
        {
// We log the PORT_WRITE event as happening right
// before the actual call, as we want to know the
// entry point a potentially long trace
#ifdef ENABLE_PORT_LOGGING
            CreateAndLogEvent(item, {GetUUID(), s->GetUUID()}, DataEvent::Type::MOVEMENT);
#endif

            s->Enqueue(item, PortWriteKey<T>());
        }
    }

    size_t NumConnections() const
    {
        Lock lock(_mutex);
        return _connections.size();
    }

    // ======================================
    // Methods protected by PortConnectionKey
    // ======================================
    // Connect this input port to the specified output
    // If the port is already connected, does nothing
    void Connect(typename InputPort<T>::Ptr &p, PortConnectionKey<T>)
    {
        Lock lock(_mutex);
        _connections.insert(p);
    }

    // Disconnect from an OutputPort
    // If the port is not actually connected, does nothing
    void Disconnect(InputPort<T> *sink, PortConnectionKey<T>)
    {
        Lock lock(_mutex);
        SharedPtrComparison equal;
        remove_from_set(sink, _connections, equal);
    }

  private:
    mutable Mutex _mutex;
    using ConnectionSet = std::unordered_set<typename InputPort<T>::Ptr>;
    ConnectionSet _connections; // All connected InputPorts
};

// Connects two ports to each other, taking care of upcasting the shared pointers
// Internally assures that the input and output port data types match such that
// this template will never be matched for mismatched ports
template <typename DerivedOutputT, typename DerivedInputT>
void connect_ports(const DerivedOutputT &out, const DerivedInputT &in)
{
    using DataType = typename DerivedOutputT::element_type::DataType;
    using BaseInput = InputPort<DataType>;
    using BaseOutput = OutputPort<DataType>;

    typename BaseInput::Ptr upcastIn = std::static_pointer_cast<BaseInput>(in);
    typename BaseOutput::Ptr upcastOut = std::static_pointer_cast<BaseOutput>(out);
    upcastOut->Connect(upcastIn, PortConnectionKey<DataType>());
    upcastIn->Connect(upcastOut, PortConnectionKey<DataType>());

    InitEvent event;
    event.type = InitEvent::Type::CONNECT_PORTS;
    event.outputPortID = out->GetUUID();
    event.inputPortID = in->GetUUID();
    Logger::Inst().LogInitEvent(event);
}

template <typename DerivedOutputT, typename DerivedInputT>
void disconnect_ports(const DerivedOutputT &out, const DerivedInputT &in)
{
    using DataType = typename DerivedOutputT::element_type::DataType;
    using BaseInput = InputPort<DataType>;
    using BaseOutput = OutputPort<DataType>;

    typename BaseInput::Ptr upcastIn = std::static_pointer_cast<BaseInput>(in);
    typename BaseOutput::Ptr upcastOut = std::static_pointer_cast<BaseOutput>(out);
    upcastOut->Disconnect(upcastIn.get(), PortConnectionKey<DataType>());
    upcastIn->Disconnect(upcastOut.get(), PortConnectionKey<DataType>());

    // TODO Log this event?
}

} // end namespace ARF
