#pragma once

#include "switchboardd/IRtsHandler.h"

class RtsHandlerV2 : public IRtsHandler {
public:
  bool StartRts();
};