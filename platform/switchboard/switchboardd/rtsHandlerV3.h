#pragma once

#include "switchboardd/IRtsHandler.h"

class RtsHandlerV3 : public IRtsHandler {
public:
  bool StartRts();
};