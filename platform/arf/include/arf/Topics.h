#pragma once

#include "arf/Intraprocess.h"
#include "arf/Types.h"
#include <unordered_set>
#include <unordered_map>

namespace ARF
{

class TopicRegistration
{
public:
    
    using Ptr = std::shared_ptr<TopicRegistration>;
    using ConstPtr = std::shared_ptr<const TopicRegistration>;

    TopicRegistration() = default;
    // TODO Some cleanup?
    ~TopicRegistration() = default;

    size_t NumInputs() const;
    size_t NumOutputs() const;

    template <typename T>
    bool AddInput( typename InputPort<T>::Ptr& in )
    {
        Lock lock( _mutex );
        // Only have to check inputs because we will be casting outputs later
        if( !CheckInputTypes<T>() ) { return false; }

        // Cast and connect each output
        using OutPtr = typename OutputPort<T>::Ptr;
        for( const PortBase::Ptr& b : _outputs )
        {
            OutPtr bport = std::dynamic_pointer_cast<OutputPort<T>>( b );
            if( nullptr == bport )
            {
                std::cerr << "Error adding port!" << std::endl;
                return false;
            }
            connect_ports( bport, in );
        }
        _inputs.insert( in );
        return true;
    }

    template <typename T>
    bool AddOutput( typename OutputPort<T>::Ptr& out )
    {
        Lock lock( _mutex );
        // Only have to check outputs because we will be casting outputs later
        if( !CheckOutputTypes<T>() ) { return false; }

        // Cast and connect each input
        using InPtr = typename InputPort<T>::Ptr;
        for( const PortBase::Ptr& b : _inputs )
        {
            InPtr bport = std::dynamic_pointer_cast<InputPort<T>>( b );
            if( nullptr == bport )
            {
                std::cerr << "Error adding port!" << std::endl;
                return false;
            }
            connect_ports( out, bport );
        }
        _outputs.insert( out );
        return true;
    }

private:

    mutable Mutex _mutex;
    std::unordered_set<PortBase::Ptr> _inputs;
    std::unordered_set<PortBase::Ptr> _outputs;

    template <typename T>
    bool CheckOutputTypes() const
    {
        for( const PortBase::Ptr& i : _outputs )
        {
            if( nullptr == std::dynamic_pointer_cast<OutputPort<T>>( i ) ) { return false; }
        }
        return true;
    }

    template <typename T>
    bool CheckInputTypes() const
    {
        for( const PortBase::Ptr& i : _inputs )
        {
            if( nullptr == std::dynamic_pointer_cast<InputPort<T>>( i ) ) { return false; }
        }
        return true;
    }
};

// Provides centralized lookup of topic names and abstraction
// of intraprocess communications
// TODO Support interprocess communications too
class TopicDirectory
{
public:

    // Singleton accessor
    static TopicDirectory& Inst();
    
    // Deallocates the singleton
    static void Destruct();

    // Deletes all topics
    // NOTE Does not result in ports being destructed!
    void Clear();
    
    size_t NumTopics() const;

    // Retrieve the topic registration for a particular topic, if it exists
    // If it does not, returns nullptr
    TopicRegistration::ConstPtr GetTopic( const std::string& topic ) const;

    // Subscribes an input port to all outputs on the specified topic
    // Takes a shared_ptr to the base or derived port type
    template <typename InPortPtr>
    bool Subscribe( const std::string& topic, const InPortPtr& p )
    {
        using DataType = typename InPortPtr::element_type::DataType;
        using BasePortType = InputPort<DataType>;
        TopicRegistration::Ptr& reg = LookupTopic( topic );
        typename BasePortType::Ptr upcastP = std::static_pointer_cast<BasePortType>( p );
        return reg->AddInput<DataType>( upcastP );
    }

    // Subscribes an input port to all outputs on the specified topic
    // Takes a shared_ptr to the base or derived port type
    template <typename OutPortPtr>
    bool Advertise( const std::string& topic, const OutPortPtr& p )
    {
        using DataType = typename OutPortPtr::element_type::DataType;
        using BasePortType = OutputPort<DataType>;
        TopicRegistration::Ptr& reg = LookupTopic( topic );
        typename BasePortType::Ptr upcastP = std::static_pointer_cast<BasePortType>( p );
        return reg->AddOutput<DataType>( upcastP );
    }

private:

    static TopicDirectory* _inst; // Singleton instance
    mutable Mutex _mutex;

    // We have to allocate TopicRegistration separately from the map, since we expose
    // TopicRegistration to outside users and operations on the registry
    // may case its contents to move in memory, invalidating pointers/references
    using TopicRegistry = std::unordered_map<std::string, TopicRegistration::Ptr>;
    TopicRegistry _topicRegistry;
    
    TopicDirectory(); // Singleton access only
    ~TopicDirectory();

    TopicRegistration::Ptr& LookupTopic( const std::string& topic )
    {
        Lock lock( _mutex );
        TopicRegistry::iterator item = _topicRegistry.find( topic );
        if( item == _topicRegistry.end() )
        {
            // Create topic and log
            _topicRegistry[topic] = std::make_shared<TopicRegistration>(); 
            return _topicRegistry[topic];
        }
        else
        {
            return item->second;
        }
    }
};

}