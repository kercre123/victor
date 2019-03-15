#pragma once

#include "arf/Types.h"
#include "arf/Intraprocess.h"
#include "arf/Threads.h"
#include "arf/Nodes.h"

#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>
#include <nng/protocol/pubsub0/sub.h>

namespace ARF
{

// Interprocess communications
// ===========================
// External (interprocess) communications is provided by a singleton class 
// that connects to internal (intraprocess) ports and routes them to sockets 
//
// We use an implicit (de)serialization interface supporting objects/functors,
// described below.
// 
// TODO Interface for passing destructor/cleanup functor to
// deserializers creating managed objects
//
// Deserializers
// =============
// Deserializers are expected to be copy constructible and implement:
//
// bool operator()( const void* buf, size_t sz, T& t, bool& toFree )
// 
// where buf is the received data in an NNG-allocated buffer with sz bytes,
// and t is the destination deserialized object to write to. The bool toFree 
// should be true if the buffer can be freed after  deserialization, and 
// false if the buffer will be managed by t. The operator should return 
// true if deserialization succeeds, and false otherwise. If deserialization
// fails, buf will be freed.
//
// Serializers
// ===========
// Serialization is a bit trickier than deserialization due to NNG's message 
// allocation requirement for asynchronous operation. There's no good way to jump
// around this, so we ask Serializers to allocate and return a nng_msg*.
// Serializers are thus expected to be copy constructible and implement:
// 
// TODO Add methods to wrap this a little so we can hide NNG
// nng_msg* operator()( const T& t );
// 
// where t is the object to serialize. A nullptr return will be interpreted as a
// serialization error.

// Using this macro for checking NNG return values
#define CHECK_NNG_RV(RV, MSG) if( RV < 0 ) { std::cerr << MSG << std::endl; return false; }

class ExternalCommsRouter : public Node
{
public:

    // Typedefs to hide underlying implementation
    using SerializedMessageType = nng_msg*;

    // Singleton accessor
    static ExternalCommsRouter& Inst();

    // Deallocate the singleton
    // Note that this blocks until all port tasks are done
    static void Destruct();

    // Provide an OutputPort that produces data received from the specified
    // IPC url
    // See above documentation for implicit Deserializer interface
    template <typename T, typename Deserializer>
    typename OutputPort<T>::Ptr Subscribe( const std::string& url,
                                           const Deserializer& des )
    {
        // If already registered, can simply return registered port
        TransceiveRegistration::Ptr& reg = _trxRegistry[ url ];
        if( reg )
        {
            return Node::RetrieveOutputPort<T>( url );
        }

        // First setup NNG socket
        // TODO Print more informative messages in failure
        reg = std::make_shared<TransceiveRegistration>();
        if( !SetupRx( url, *reg ) )
        {
            return nullptr;
        }

        // Create port
        typename OutputPort<T>::Ptr port = std::make_shared<OutputPort<T>>();
        reg->tracker = _taskTracker;
        Node::RegisterPort( port, url );

        // Create callbacks
        TransceiveRegistration* rawReg = reg.get();
        OutputPort<T>* rawPort = port.get();

        // Receives a message on the AIO port and writes it to
        // our internal Port
        reg->rxServiceTask = [des, rawReg, rawPort]( nng_msg* msg )
        {
            // Deserialize and push to port if succeeds
            T deserialized;
            bool freeAfter;
            bool succ = des( nng_msg_body( msg ), 
                             nng_msg_len( msg ), 
                             deserialized, 
                             freeAfter );
            if( succ )
            {
                rawPort->Write( deserialized );
            }
            if( freeAfter || !succ )
            {
                nng_msg_free( msg );
            }
        };

        // Called by NNG on a message receive event
        // Throws the port handling task into the task queue

        int rv = nng_aio_alloc( &reg->aio, &ExternalCommsRouter::HandleRxEvent, static_cast<void*>( rawReg ) );
        if( rv != 0 )
        {
            std::cerr << "Could not allocate AIO: " << url << std::endl;
            // TODO More cleanup here
            return nullptr;
        }

        // Start receiving
        nng_recv_aio( reg->sock, reg->aio );

        return port;
    }

    // Provide an OutputPort that produces data received from the specified
    // IPC url 
    // See above documentation for Serializer implicit interface
    template <typename T, typename Serializer>
    typename InputPort<T>::Ptr Advertise( const std::string& url,
                                          size_t buffSize,
                                          const Serializer& ser )
    {
        // If already registered, can simply return registered port
        TransceiveRegistration::Ptr& reg = _trxRegistry[ url ];
        if( reg )
        {
            return Node::RetrieveInputPort<T>( url );
        }

        // First setup NNG socket
        // TODO Print more informative messages in failure
        reg = std::make_shared<TransceiveRegistration>();
        if( !SetupTx( url, *reg ) )
        {
            return nullptr;
        }

        // Create port
        typename InputPort<T>::Ptr port = std::make_shared<InputPort<T>>( buffSize );
        reg->tracker = _taskTracker;
        Node::RegisterPort( port, url );

        // Create callback
        TransceiveRegistration* rawReg = reg.get();
        InputPort<T>* rawPort = port.get();

        // This handles pulling data from the buffered input port, serializing it,
        // and then offering it to NNG
        rawReg->txServiceTask = [ser, rawReg, rawPort]()
        {
            // Pull data from the port
            // Note that there may not be data ready if the port is buffered and has dropped data!
            T data;
            if( !rawPort->Read( data ) )
            {
                return;
            }

            // Get the serialized message
            nng_msg* msg = ser( data );
            if(!msg )
            {
                std::cerr << "Failed to serialize data" << std::endl;
                return;
            }

            // Queue the message, which takes ownership of the nng_msg memory
            // Note, we don't want multiple threads of execution going through the call to send
            // which should be OK since nng_sendmsg just queues the message internally
            Lock lock( rawReg->mutex );
            if( nng_sendmsg( rawReg->sock, msg, NNG_FLAG_NONBLOCK ) != 0 )
            {
                std::cerr << "Failed to queue message for send" << std::endl;
                nng_msg_free( msg );
                return;
            }
        };

        // This callback schedules a port service task on each input port receive
        // The actual serialization and transmit happens in the task, keeping the port 
        // callback fast
        typename InputPort<T>::EventCallback portHandler = [rawReg]( InputPort<T>* /*p*/ )
        {
            Threadpool::Inst().EnqueueTask( rawReg->txServiceTask, TaskPriority::Standard, rawReg->tracker );
        };

        port->RegisterCallback( portHandler );
        return port;
    }

    // Methods for Serializers to use
    // Allocates a message used internally of size sz bytes
    SerializedMessageType AllocateMessage( size_t sz );

    // Get a void* pointer to the writable body of the message
    void* GetMessageBody( SerializedMessageType msg );

    // Get the number of bytes allocated
    size_t GetMessageSize( SerializedMessageType msg );

private:

    class TransceiveRegistration
    {
    public:

        using Ptr = std::shared_ptr<TransceiveRegistration>;

        // Intraprocess/task use
        TaskTracker::Ptr tracker;
        
        Task txServiceTask;
        std::function<void(nng_msg*)> rxServiceTask;
        
        // Prevents concurrent accesses to the aio object
        Mutex mutex;

        // NNG use
        // NOTE We want to keep the dialer/listener around explicitly so we
        // can query who we've connected to
        nng_socket sock;
        nng_dialer dialer;
        nng_listener listener;
        nng_aio* aio = nullptr;

        ~TransceiveRegistration();
    };

    static ExternalCommsRouter* _inst;

    Mutex _mutex;
    TaskTracker::Ptr _taskTracker;

    using TransceiveRegistry = std::unordered_map<std::string, TransceiveRegistration::Ptr>;
    TransceiveRegistry _trxRegistry;
    
    // Singleton access only
    ExternalCommsRouter();
    ~ExternalCommsRouter();

    // Callback to be used by NNG for receive events
    static void HandleRxEvent( void* data );

    // Set up the registration for receive/transmit at the specified URL
    // Returns success
    bool SetupRx( const std::string& url, TransceiveRegistration& reg );
    bool SetupTx( const std::string& url, TransceiveRegistration& reg );
};

}