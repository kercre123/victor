#include "neonTests.h"

#include <stdio.h>
#include <stdlib.h>
#include <chrono>
#include <csignal>

namespace {

}

void Cleanup(int signum)
{  
  exit(signum);
}

int main(int argc, char* argv[])
{
  signal(SIGTERM, Cleanup);

  if(argc < 2)
  {
    printf("Not enough arguments\n");
    return -1;
  }

  printf("Starting Neon Tests\n");

  Neon::RunTest(argv[1]);

  printf("Finish Neon Tests\n");
}
