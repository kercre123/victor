using System;
using UnityEngine;
using Anki.Cozmo;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json;

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

  // TODO: Replace this with CLAD generated enums to match engine message
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

  // Dictionary that maps CLAD Events to Actions
  // Other Tools add their specific actions to the CozmoEvent
  // For example, The AnimationTool would add SendAnimationGroup calls to CozmoEvents
  public Dictionary<CozmoEvent,Action> CozmoEventDict = new Dictionary<CozmoEvent, Action>();

  public CozmoEventManager() {
    // TODO : Constructor?
    // RobotEngineManager.Instance.WhateverTheEventMessageThingyIs += CozmoEventReceived;
  }

  // TODO : Ok so the CozmoEvent Action has been fired, how does that action send the proper animation?
  // AnimationTool has a CozmoEvent to String mapping for GroupNames?
  // Fire the Action for the specific CozmoEvent enum
  public bool CozmoEventRecieved(CozmoEvent cozEvent) {
    Action cozAction;
    if (CozmoEventDict.TryGetValue(cozEvent, out cozAction)) {
      if (cozAction != null) {
        cozAction.Invoke();
        return true;
      }
    }
    return false;
  }
}
