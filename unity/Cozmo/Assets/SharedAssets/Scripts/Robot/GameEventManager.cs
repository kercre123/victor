using System;
using UnityEngine;
using Anki.Cozmo;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json;


public class GameEventManager {
  
  private static GameEventManager _Instance = null;

  public static GameEventManager Instance { 
    get { 
      if (_Instance == null) { 
        _Instance = new GameEventManager();
      }
      return _Instance; 
    } 
  }

  // Event that fires and notifes all listeners of a CozmoEvent being recieved,
  // other managers listen to this and use that to map enum based events to things
  public event Action<GameEvents> OnReceiveGameEvent;

  // List of All Event Enums, used by tools for visualization
  public readonly List<GameEvents> CozmoEventList;

  public GameEventManager() {
    CozmoEventList = new List<GameEvents>();
    for (int i = 0; i < (int)GameEvents.Count; i++) {
      CozmoEventList.Add((GameEvents)i);
    }
    // Listen to whatever message is giving us the CozmoEvent message.
    // RobotEngineManager.Instance.WhateverTheEventMessageThingyIs += HandleGameEvent;
  }

  // Fire the Action without sending a message base to engine, as we received the message from engine
  public void HandleGameEvent(GameEvents cozEvent) {
    if (OnReceiveGameEvent != null) {
      OnReceiveGameEvent(cozEvent);
    }
  }

  public void SendGameEventToEngine(GameEvents cozEvent) {
    // TODO : Send Message down to engine before firing the game event
    HandleGameEvent(cozEvent);
  }
}
