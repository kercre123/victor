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
  public event Action<GameEventWrapper> OnGameEvent;

  // List of All Event Enums, used by tools for visualization
  public readonly List<GameEvent> GameEventList;

  public GameEventManager() {
    GameEventList = new List<GameEvent>();
    for (int i = 0; i < (int)GameEvent.Count; i++) {
      GameEventList.Add((GameEvent)i);
    }
    GameEventWrapperFactory.Init();
  }

  public void FireGameEvent(GameEvent cozEvent) {
    FireGameEvent(GameEventWrapperFactory.Create(cozEvent));
  }

  // Fire the Action without sending a message base to engine
  // Use this for when we receive a game event from engine
  public void FireGameEvent(GameEventWrapper cozEvent) {
    // Don't fire game events while in SDK mode since we don't want metagame features enablec
    if (DataPersistence.DataPersistenceManager.Instance.IsSDKEnabled) {
      return;
    }
    if (OnGameEvent != null) {
      OnGameEvent(cozEvent);
    }
  }
}
