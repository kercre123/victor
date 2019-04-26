#pragma once

#include <functional>
#include <unordered_map>
#include <memory>
#include <arf/Types.h>

namespace ARF
{

// Forward declaration
template <typename T>
class TopicBuffer;

// Functions that can be used with TopicPoster::InvokePost
template <typename T>
using TopicPostOperation = std::function<void(T&)>;

// Functions that can be used with TopicViewer::InvokeView
template <typename T>
using TopicViewOperation = std::function<void(const T&)>;

// Functions that can be registered as notifications
using TopicNotifier = std::function<void()>;

// Functions that can be registered as callbacks
template <typename T>
using TopicCallback = std::function<void(const T&)>;

// Poster object given to users after topic post intent
template <typename T>
class TopicPoster
{
public:

    // Create a new Poster wrapping the specified topic buffer
    TopicPoster() = default;
    
    TopicPoster( TopicBuffer<T>* b )
    : _field( b ) {}

    bool IsInitialized() const
    {
        return nullptr != _field;
    }

    // Post a new value to the field
    void Post( const T& v )
    {
        _field->Post( v );
    }

    // Invalidate the field's currently held value
    void Clear()
    {
        _field->Clear();
    }

    // Lock the topic and run an operation
    void InvokePost( const TopicPostOperation<T>& op )
    {
        _field->InvokePost( op );
    }

private:

    // NOTE _field MUST stay valid for lifespan of Poster!
    TopicBuffer<T>* _field = nullptr;
};

// Viewer object given to users after declaration of topic view intent
template <typename T>
class TopicViewer
{
public:

    // Create a new Viewer wrapping the specified topic buffer
    TopicViewer() = default;
    
    TopicViewer( TopicBuffer<T>* b )
    : _field ( b ) {}

    bool IsInitialized() const
    {
        return nullptr != _field;
    }

    // Retrieve a copy of the topic field
    // If the field is not valid, will return false
    bool Retrieve( T& t ) const
    {
        return _field->Retrieve( t );
    }

    // Read-lock the topic and run an operation
    void InvokeView( const TopicViewOperation<T>& op ) const
    {
        _field->InvokeView( op );
    }

private:

    // NOTE _field MUST stay valid for lifespan of Viewer!
    TopicBuffer<T>* _field = nullptr;
};


// Base to allow type erasure
class TopicBufferBase
{
public:

    using Ptr = std::shared_ptr<TopicBufferBase>;

    virtual ~TopicBufferBase() = default;
};

// An atomic container with a valid bit
template <typename T>
class TopicBuffer : public TopicBufferBase
{
public:

    using Ptr = std::shared_ptr<TopicBuffer<T>>;
    using CleanupCallback = std::function<void(T)>;

    // Constructor that default-constructs field
    // Initializes to invalid state
    TopicBuffer( const CleanupCallback& cb = CleanupCallback() )
    : _isValid( false ), _cleanupCb( cb ) {}

    // Constructor that copy-constructs field for types without a default constructor
    // Initializes to valid state
    TopicBuffer( const T& t, const CleanupCallback& cb = CleanupCallback() )
    : _isValid( true ), _cleanupCb( cb ), _value( t ) {}

    // TODO Constructor that forwards arguments for particularly nasty types?

    // If a cleanup callback is assigned, calls it with the
    // field if the field is valid
    ~TopicBuffer()
    {
        Clear();
    }

    // Returns a copy of the field
    // This can be forbidden by disabling copy construction
    bool Retrieve( T& t ) const
    {
        ReadLock lock( _mutex );
        if( !_isValid ) { return false; }
        t = T( _value ); // Disable copy constructor to forbid
        return true;
    }

    // Read-locks the field and calls the operation with the field
    // Does nothing if the field is invalid
    void InvokeView( const TopicViewOperation<T>& op ) const
    {
        ReadLock lock( _mutex );
        if( !_isValid ) { return; }
        op( _value );
    }

    // Sets the topic to be invalid
    // If a cleanup callback was given, will call it on the field
    void Clear()
    {
        WriteLock lock( _mutex );
        ClearImpl();
    }

    // Posts the new value, overwriting the previous value
    // This can be forbidden by disabling copy construction
    void Post( const T& v )
    {
        WriteLock lock( _mutex );
        _isValid = true;
        _value = T( v ); // Disable copy constructor to forbid
        PostCallbacks(); // Must stay locked to prevent change
    }

    // Write-locks the field and calls the operation with the field
    // Does nothing if the field is invalid
    void InvokePost( const TopicPostOperation<T>& op )
    {
        WriteLock lock( _mutex );
        if( !_isValid ) { return; }
        op( _value );
        PostCallbacks();;
    }

    void RegisterNotifier( const TopicNotifier& t )
    {
        WriteLock lock( _mutex );
        _notifiers.push_back( t );
    }

    void RegisterCallback( const TopicCallback<T>& t )
    {
        WriteLock lock( _mutex );
        _callbacks.push_back( t );
    }

private:

    mutable Mutex _mutex;
    bool _isValid;
    CleanupCallback _cleanupCb;
    T _value;
    std::vector<TopicNotifier> _notifiers;
    std::vector<TopicCallback<T>> _callbacks;

    void PostCallbacks()
    {
        for( TopicCallback<T>& t : _callbacks )
        {
            t( _value );
        }
        for( TopicNotifier& t : _notifiers )
        {
            t();
        }
    }

    void ClearImpl()
    {
        if( !_isValid ) { return; }
        if( _cleanupCb ) { _cleanupCb( _value ); }
        _isValid = false;
    }
};

class BulletinBoard
{
public:

    using Ptr = std::shared_ptr<BulletinBoard>;

    // Creates a new empty board
    BulletinBoard() = default;

    // TODO Signal shutdown to all field buffers?
    ~BulletinBoard() = default;

    // Initializes a topic of the specified type
    template <typename T>
    bool Declare( const std::string& topic )
    {
        Lock lock( _mutex );
        TopicRegistry::iterator iter = _fields.find( topic );
        // If doesn't exist, create it
        if( iter == _fields.end() )
        {
            _fields[ topic ] = std::make_shared<TopicBuffer<T>>();
            return true;
        }
        // If it does exist, check the type
        else
        {
            using DerivedBuffer = TopicBuffer<T>;
            return nullptr != std::dynamic_pointer_cast<DerivedBuffer>( iter->second );
        }
    }

    // Initializes a topic and initializes it to be valid
    template <typename T>
    bool Declare( const std::string& topic, const T& t )
    {
        Lock lock( _mutex );
        TopicRegistry::iterator iter = _fields.find( topic );
        // If doesn't exist, create it
        if( iter == _fields.end() )
        {
            _fields[ topic ] = std::make_shared<TopicBuffer<T>>( t );
            return true;
        }
        // If it does exist, check the type
        else
        {
            using DerivedBuffer = TopicBuffer<T>;
            return nullptr != std::dynamic_pointer_cast<DerivedBuffer>( iter->second );
        }
    }

    // Return a Poster object corresponding to the given topic
    // If topic is not declared or type is wrong, returns nullptr
    template <typename T>
    TopicPoster<T> MakePoster( const std::string& topic )
    {
        Lock lock( _mutex );
        typename TopicBuffer<T>::Ptr buff = GetDerivedBuffer<T>( topic );
        // If cast failed, we have the wrong type or it isn't declared
        if( !buff ) 
        {
            return TopicPoster<T>();
        }
        else
        {
            return TopicPoster<T>( buff.get() );
        }
    }

    // Return a Poster object corresponding to the given topic
    // If topic is not declared or type is wrong, returns nullptr
    template <typename T>
    TopicViewer<T> MakeViewer( const std::string& topic )
    {
        Lock lock( _mutex );
        typename TopicBuffer<T>::Ptr buff = GetDerivedBuffer<T>( topic );
        // If cast failed, we have the wrong type or it isn't declared
        if( !buff ) 
        {
            return nullptr;
        }
        else
        {
            return TopicViewer<T>( buff.get() );
        }
    }

    template <typename T>
    bool RegisterNotifier( const std::string& topic, const TopicNotifier& t )
    {
        Lock lock( _mutex );
        typename TopicBuffer<T>::Ptr buff = GetDerivedBuffer<T>( topic );
        // If cast failed, we have the wrong type or it isn't declared
        if( !buff ) 
        {
            return false;
        }
        else
        {
            buff->RegisterNotifier( t );
            return true;
        }
    }

    template <typename T>
    bool RegisterCallback( const std::string& topic, const TopicCallback<T>& t )
    {
        Lock lock( _mutex );
        typename TopicBuffer<T>::Ptr buff = GetDerivedBuffer<T>( topic );
        // If cast failed, we have the wrong type or it isn't declared
        if( !buff ) 
        {
            return false;
        }
        else
        {
            buff->RegisterCallback( t );
            return true;
        }
    }

private:

    using TopicRegistry = std::unordered_map<std::string, TopicBufferBase::Ptr>;
    
    Mutex _mutex;
    TopicRegistry _fields;

    // Retrieves the field buffer at the specified topic cast to its derived type
    // If the buffer does not exist or the type is wrong, returns nullptr
    template <typename T>
    typename TopicBuffer<T>::Ptr GetDerivedBuffer( const std::string& topic )
    {
        TopicRegistry::iterator iter = _fields.find( topic );
        if( iter == _fields.end() )
        {
            return nullptr;
        }
        else
        {
            using DerivedBuffer = TopicBuffer<T>;
            return std::dynamic_pointer_cast<DerivedBuffer>( iter->second );
        }
    }
};

}