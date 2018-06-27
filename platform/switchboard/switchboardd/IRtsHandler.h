#pragma once

enum CommsState : uint8_t {
  Raw,
  Clad,
  SecureClad
};

class IRtsHandler {
public:
  virtual ~IRtsHandler() {
  }
  
  virtual bool StartRts() = 0;

protected:
  inline bool AssertState(CommsState state) {
    return state == _commsState;
  }

  CommsState _commsState = CommsState::Raw;
};