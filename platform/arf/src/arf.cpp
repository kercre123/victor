#include "arf/arf.h"
#include <stdio.h>
#include <unistd.h>
#include <unordered_map>

#include <nng/protocol/pubsub0/pub.h>
#include <nng/protocol/pubsub0/sub.h>

namespace ARF
{
  std::string gARFName;
  
  std::string GetURL(const std::string& node_name, const std::string& topic_name)
  {
    std::string folder = "/tmp/arf/" + node_name;
    int ret = system(("mkdir -p '" + folder + "'").c_str());
    if (ret != 0) {
      return "";
    } 
    return "ipc://" + folder + "/" + topic_name;
  }
  
  std::string GetTCPURL(const std::string& node_name, const std::string& topic_name)
  {
    const std::unordered_map<std::string, int> portMap = {
      {"royale_depth_node/depth", 11000},
      {"kobuki_arf/kobuki_pose", 11001},
      {"kobuki_arf/kobuki_pose_update", 11002},
      {"kobuki_arf/scan", 11003},
      {"kobuki_arf/map", 11004},
      {"kobuki_arf/slam_pose", 11005},
      {"vector/rgb", 999}
    };

    std::string full_name = node_name.empty() ? topic_name : (node_name + "/" + topic_name);
    auto it = portMap.find(full_name);
    if(it != portMap.end()) {
      static char buf[64];
      sprintf(buf, "tcp://192.168.34.128:%i", it->second);
      return buf;
    }
    
    return "tcp://192.168.34.128:40000";
  }
  
  void Init(int /*argc*/, char** /*argv*/, const char* name)
  {
    printf("[%s] Initializing ARF...\n", name);
    gARFName = name;
  }

  PubHandleBase::PubHandleBase(const std::string& topic_name)
  {
    int rv;
    
    if ((rv = nng_pub0_open(&sock)) != 0) {
     fatal("nng_pub0_open", rv);
    }
    
    std::string url = GetURL(gARFName, topic_name);
    
    if ((rv = nng_listen(sock, url.c_str(), NULL, 0)) < 0) {
     fatal("nng_listen", rv);
    }
    
    // Temp TCP socket
    if ((rv = nng_pub0_open(&tcp_sock)) != 0) {
     fatal("nng_pub0_open", rv);
    }
    
    std::string tcp_url = GetTCPURL(gARFName, topic_name);
    printf("Listening on %s\n", tcp_url.c_str());
    if ((rv = nng_listen(tcp_sock, tcp_url.c_str(), NULL, 0)) < 0) {
     fatal("nng_listen", rv);
    }
  }

  void PubHandleBase::_publish(const std::string& data) const
  {
    int rv;
    if ((rv = nng_send(sock, (void*)data.c_str(), data.size(), 0)) != 0) {
     fatal("nng_send", rv);
    }
    
    // Temp TCP
    if ((rv = nng_send(tcp_sock, (void*)data.c_str(), data.size(), 0)) != 0) {
     fatal("nng_send", rv);
    }
  }
  
  void PubHandleBase::_publish(void* bytes, size_t size) const
  {
    int rv;
    if ((rv = nng_send(sock, bytes, size, 0)) != 0) {
      fatal("nng_send", rv);
    }
    
    // Temp TCP
    if ((rv = nng_send(tcp_sock, bytes, size, 0)) != 0) {
      fatal("nng_send", rv);
    }
  }
  
  SubHandleBase::SubHandleBase(const std::string& topic_name)
  {
    int rv;
    if ((rv = nng_sub0_open(&sock)) != 0) {
      fatal("nng_sub0_open", rv);
    }

    // subscribe to everything (empty means all topics)
    nng_setopt_size(sock, NNG_OPT_RECVMAXSZ, 1024 * 1024 * 5);
    if ((rv = nng_setopt(sock, NNG_OPT_SUB_SUBSCRIBE, "", 0)) != 0) {
      fatal("nng_setopt", rv);
    }
    
    std::string url = GetURL("", topic_name);
    
    if ((rv = nng_dial(sock, url.c_str(), NULL, 0)) != 0) {
      url = GetTCPURL("", topic_name);
      printf("Trying TCP conn to %s\n", url.c_str());
      if ((rv = nng_dial(sock, url.c_str(), NULL, 0)) != 0) {
        fatal("nng_dial", rv);
      }
    }
  }
  
  void SubHandleBase::Tick()
  {
    while(true) {
      int rv;
      char *buf = NULL;
      size_t sz;
      if ((rv = nng_recv(sock, &buf, &sz, NNG_FLAG_ALLOC | NNG_FLAG_NONBLOCK)) != 0) {
        if(rv == NNG_EAGAIN) {
          return;
        }
        fatal("nng_recv", rv);
      }

      if(buf && sz > 0) {
        OnMsg(buf, sz);
        nng_free(buf, sz);
      }
    }
  }
  
  void NodeHandle::Spin()
  {
    while(true) {
      SpinOnce();
      usleep(100000);
    }
  }
  
  void NodeHandle::SpinOnce()
  {
    for(SubHandleBase* sub : _subHandles) {
      sub->Tick();
    }
  }

}
