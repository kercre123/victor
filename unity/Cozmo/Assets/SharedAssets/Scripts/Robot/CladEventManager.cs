using System;
using UnityEngine;
using Anki.Cozmo;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json;

// TODO: Move all of this into clad
public enum CladGameEvent : byte {
  SPEEDTAP_TAP,
  SPEEDTAP_WIN_HAND,
  SPEEDTAP_WIN_ROUND,
  SPEEDTAP_WIN_SESSION,
  SPEEDTAP_LOSE_HAND,
  SPEEDTAP_LOSE_ROUND,
  SPEEDTAP_LOSE_SESSION,
  SPEEDTAP_IDLE,
  SPEEDTAP_FAKE,
  MORE_BULLSHIT,
  EVEN_MORE_BULLSHIT,
  A_LOT_MORE_BULLSHIT,
  EXCESSIVE_BULLSHIT,
  WAY_TOO_MUCH_BULLSHIT,
  WELL_BEYOND_ACCEPTABLE_BULLSHIT_LEVELS,
  TOO_LAZY_FOR_THIS_JOKE_1,
  TOO_LAZY_FOR_THIS_JOKE_2,
  TOO_LAZY_FOR_THIS_JOKE_3,
  TOO_LAZY_FOR_THIS_JOKE_4,
  TOO_LAZY_FOR_THIS_JOKE_5,
  TOO_LAZY_FOR_THIS_JOKE_6,
  TOO_LAZY_FOR_THIS_JOKE_7,
  TOO_LAZY_FOR_THIS_JOKE_8,
  TOO_LAZY_FOR_THIS_JOKE_9,
  TOO_LAZY_FOR_THIS_JOKE_10,
  TOO_LAZY_FOR_THIS_JOKE_11,
  TOO_LAZY_FOR_THIS_JOKE_12,
  TOO_LAZY_FOR_THIS_JOKE_13,
  TOO_LAZY_FOR_THIS_JOKE_14,
  TOO_LAZY_FOR_THIS_JOKE_15,
  TOO_LAZY_FOR_THIS_JOKE_16,
  TOO_LAZY_FOR_THIS_JOKE_17,
  TOO_LAZY_FOR_THIS_JOKE_18,
  TOO_LAZY_FOR_THIS_JOKE_19,
  TOO_LAZY_FOR_THIS_JOKE_20,
  TOO_LAZY_FOR_THIS_JOKE_21,
  TOO_LAZY_FOR_THIS_JOKE_22,
  TOO_LAZY_FOR_THIS_JOKE_23,
  TOO_LAZY_FOR_THIS_JOKE_24,
  TOO_LAZY_FOR_THIS_JOKE_25,
  TOO_LAZY_FOR_THIS_JOKE_26,
  TOO_LAZY_FOR_THIS_JOKE_27,
  TOO_LAZY_FOR_THIS_JOKE_28,
  TOO_LAZY_FOR_THIS_JOKE_29,
  TOO_LAZY_FOR_THIS_JOKE_30,
  COUNT
}

public class CladEventManager {
  
  private static CladEventManager _Instance = null;

  public static CladEventManager Instance { 
    get { 
      if (_Instance == null) { 
        _Instance = new CladEventManager();
      }
      return _Instance; 
    } 
  }

  // Event that fires and notifes all listeners of a CozmoEvent being recieved,
  // other managers listen to this and use that to map enum based events to things
  public event Action<CladGameEvent> CozmoEventReceived;

  // List of All Event Enums, used by tools for visualization
  public readonly List<CladGameEvent> CozmoEventList;

  public CladEventManager() {
    CozmoEventList = new List<CladGameEvent>();
    for (int i = 0; i < (int)CladGameEvent.COUNT; i++) {
      CozmoEventList.Add((CladGameEvent)i);
    }
    // Listen to whatever message is giving us the CozmoEvent message.
    // RobotEngineManager.Instance.WhateverTheEventMessageThingyIs += RaiseCozmoEvent;
  }

  public void RaiseCozmoEvent(CladGameEvent cozEvent) {
    if (CozmoEventReceived != null) {
      CozmoEventReceived(cozEvent);
    }
  }
}
