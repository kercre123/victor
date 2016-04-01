using System;
using UnityEngine;
using Anki.Cozmo;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json;

/// <summary>
/// Manager for calling AnimationGroups that are mapped to game events
/// </summary>
public class AnimationManager {
  
  private static AnimationManager _Instance = null;

  public static AnimationManager Instance { 
    get { 
      if (_Instance == null) { 
        _Instance = new AnimationManager();
      }
      return _Instance; 
    } 
  }

  private IRobot _CurrRobot = null;

  // Map Animation Group Names to Event Enums
  public Dictionary<GameEvents, string> AnimationGroupDict = new Dictionary<GameEvents, string>();


  public AnimationManager() {
    GameEventManager.Instance.OnReceiveGameEvent += CozmoEventRecieved;
    _CurrRobot = RobotEngineManager.Instance.CurrentRobot;
  }

  public void CozmoEventRecieved(GameEvents cozEvent) {
    string animGroup = "";
    if (AnimationGroupDict.TryGetValue(cozEvent, out animGroup)) {
      if (!string.IsNullOrEmpty(animGroup)) {
        _CurrRobot.SendAnimationGroup(animGroup);
      }
    }
  }

}
