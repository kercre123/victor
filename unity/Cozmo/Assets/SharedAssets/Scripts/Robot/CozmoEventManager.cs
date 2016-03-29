using System;
using UnityEngine;
using Anki.Cozmo;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json;

// TODO: Move all of this into clad
public enum CozmoEvent : byte {
  SPEEDTAP_TAP,
  SPEEDTAP_WIN_HAND,
  SPEEDTAP_WIN_ROUND,
  SPEEDTAP_WIN_SESSION,
  SPEEDTAP_LOSE_HAND,
  SPEEDTAP_LOSE_ROUND,
  SPEEDTAP_LOSE_SESSION,
  SPEEDTAP_IDLE,
  SPEEDTAP_FAKE,
  COUNT
}

public class CozmoEventManager {
  
  private static CozmoEventManager _Instance = null;

  public static CozmoEventManager Instance { 
    get { 
      if (_Instance == null) { 
        _Instance = new CozmoEventManager();
      }
      return _Instance; 
    } 
  }

  // Event that fires and notifes all listeners of a CozmoEvent being recieved,
  // other managers listen to this and use that to map enum based events to things
  public event Action<CozmoEvent> CozmoEventReceived;

  public CozmoEventManager() {
    // TODO : Constructor?
    // Listen to whatever message is giving us the CozmoEvent message.
    // RobotEngineManager.Instance.WhateverTheEventMessageThingyIs += RaiseCozmoEvent;
  }

  public void RaiseCozmoEvent(CozmoEvent cozEvent) {
    if (CozmoEventReceived != null) {
      CozmoEventReceived(cozEvent);
    }
  }
}
