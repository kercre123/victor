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

  // TODO: Replace this with CLAD generated enums to match engine events.
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

  // Initialize the CozmoEventManager
  public CozmoEventManager() {
    if (_Instance != null) {
      DAS.Error(this, "CozmoEventManager already initialized");
    }

  }

  // Fire a specified CozmoEvent's Action,
  public bool FireEvent(CozmoEvent toFire) {
    Action eventAction;
    if (CozmoEventDict.TryGetValue(toFire, out eventAction)) {
      if (eventAction != null) {
        eventAction.Invoke();
        return true;
      }
    }
    return false;
  }
}
