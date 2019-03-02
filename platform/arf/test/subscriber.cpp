#include <stdio.h>
#include "arf/arf.h"

int main(int argc, char* argv[])
{
  ARF::Init(argc, argv, "test_subscriber");
  ARF::NodeHandle n;
  
  n.Subscribe<std::string>("test_publisher/pub", [=](const std::string& msg){
    printf("Incoming message: %s\n", msg.c_str());
  });

  n.Spin();
  
  return 0;
}
