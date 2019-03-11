#pragma once

#include <string>
#include <functional>
#include <vector>

#include <nng/nng.h>

inline void fatal(const char* s, int rv)
{
  printf("fatal: (%i) %s\n", rv, s);
}

namespace ARF
{
  void Init(int argc, char** argv, const char* name);

  class PubHandleBase
  {
  protected:
    PubHandleBase(const std::string& topic_name);
    void _publish(const std::string& data) const;
    void _publish(void* bytes, size_t size) const;

  private:
    nng_socket sock;
    nng_socket tcp_sock;
  };

  template<typename T>
  class PubHandle : PubHandleBase
  {
  public:
    PubHandle() : PubHandleBase("") { }
    PubHandle(const std::string& topic_name) : PubHandleBase(topic_name) { }
    
    void publish(const T& data) const { _publish((void*)&data, sizeof(data)); }
    void publish_string(const std::string& data) const { _publish(data); }
  };

  class SubHandleBase
  {
  protected:
    SubHandleBase(const std::string& topic_name);
    void Tick();
    virtual void OnMsg(void* bytes, size_t size) = 0;
    
  private:
    nng_socket sock;
    friend class NodeHandle;
  };

  template<typename T>
  class SubHandle : public SubHandleBase
  {
    typedef std::function<void(const T&)> tCallback;

  public:
    SubHandle(const std::string& topic_name, const tCallback& callback) : SubHandleBase(topic_name) , _callback(callback) { }

  private:
    virtual void OnMsg(void* bytes, size_t /*size*/) override {
        T* msg = static_cast<T*>(bytes);
        _callback(*msg);
    }
    
    tCallback _callback;
  };
    
  template<>
  class SubHandle<std::string> : public SubHandleBase
  {
    typedef std::function<void(const std::string&)> tCallback;
        
    public:
        SubHandle(const std::string& topic_name, const tCallback& callback) : SubHandleBase(topic_name) , _callback(callback) { }
        
    private:
        virtual void OnMsg(void* bytes, size_t size) override {
            std::string msg(static_cast<char*>(bytes), size);
            _callback(msg);
        }
        tCallback _callback;
    };
  
  template<typename T>
  class SharedTopic
  {
  public:
    SharedTopic(const std::string& topic_name);
    
    // Blocking, Non-locking interface:
    const T& WaitForData();
    bool IsStillValid(); // Returns true if the data returned by WaitForData has not yet been overwritten.
    
    // Non-Blocking, Locking interface:
    const T* Lock(); // Locks for writing the most recent data in the buffer, returns NULL if no data in buffer
    void Unlock(); // Unlocks buffer locked by last call to Lock()
    
  private:
    
  };
  
  class NodeHandle
  {
  public:
    //////////////////////////////
    // PubSub Interface

    // Registers a new publisher. If a publisher with that name already exists, this will overwrite the existing one.
    template<typename T>
    PubHandle<T> RegisterPublisher(const std::string& topic_name)
    {
      return PubHandle<T>(topic_name);
    }

    template<typename T>
    void Subscribe(const std::string& topic_name, const std::function<void(const T&)>& callback)
    {
      SubHandle<T>* sub = new SubHandle<T>(topic_name, callback);
      _subHandles.push_back(sub);
    }
    
    //////////////////////////////
    // Shared Memory Interface
    template<typename T>
    SharedTopic<T> OpenSharedTopic(const std::string& topic_name);
    
    //////////////////////////////
    // Runtime
    void Spin();
    void SpinOnce();
    
  private:
    std::vector<SubHandleBase*> _subHandles;
  };
}
