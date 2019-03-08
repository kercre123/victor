#include <stdio.h>
#include <unistd.h>

#include "arf/arf.h"

int main(int argc, char* argv[])
{
  ARF::Init(argc, argv, "test_publisher");
  ARF::NodeHandle n;
  
  auto pubHandle = n.RegisterPublisher<std::string>("pub");
  
  for(int i = 0; i < 10; i++) {
    printf("publishing...\n");
    pubHandle.publish("Test string.");
    sleep(1);
  }
  
  return 0;
}
