using System;
using UnityEngine;
using Anki.Cozmo;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json;

[Serializable]
public class SerializableGameEvents : SerializableEnum<Anki.Cozmo.GameEvent> {

}

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
  public event Action<GameEvent> OnGameEvent;

  // List of All Event Enums, used by tools for visualization
  public readonly List<GameEvent> GameEventList;

  public GameEventManager() {
    GameEventList = new List<GameEvent>();
    for (int i = 0; i < (int)GameEvent.Count; i++) {
      GameEventList.Add((GameEvent)i);
    }
    // Listen to whatever message is giving us the CozmoEvent message.
    // TODO : Add RobotEngineManager Event Handling Here once engine work is done
  }

  // Fire the Action without sending a message base to engine
  // Use this for
  public void HandleGameEvent(GameEvent cozEvent) {
    if (OnGameEvent != null) {
      OnGameEvent(cozEvent);
    }
  }

  // Potentially use this for cases where the event is expected to be sent from unity to engine
  public void SendGameEventToEngine(GameEvent cozEvent) {
    // TODO : Send Message down to engine before firing the game event
    HandleGameEvent(cozEvent);
  }
}
