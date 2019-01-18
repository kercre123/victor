#include "switchboardd/daemon.h"

#include "switchboardd/opentracing.h"

int main(int argc, char* argv[]) {
  initializeTracing();

  SwitchboardMain();
  return 0;
}