using System;
using UnityEngine;
using Anki.Cozmo;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json;

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
  public Dictionary<CozmoEvent, string> AnimationGroupDict = new Dictionary<CozmoEvent, string>();


  public AnimationManager() {
    CozmoEventManager.Instance.CozmoEventReceived += CozmoEventRecieved;
    _CurrRobot = RobotEngineManager.Instance.CurrentRobot;
  }

  public void CozmoEventRecieved(CozmoEvent cozEvent) {
    if (AnimationGroupDict.ContainsKey(cozEvent)) {
      _CurrRobot.SendAnimationGroup(AnimationGroupDict[cozEvent]);
    }
  }

  public bool SetAnimationForEvent(CozmoEvent cozEvent, string animGroupName) {
    if (AnimationGroupDict.ContainsKey(cozEvent)) {
      DAS.Error(this, "CozmoEvent already has an Animation Assigned, only one Animation Group per Event");
      return false;
    }
    AnimationGroupDict.Add(cozEvent, animGroupName);
    return true;
  }

  public bool RemoveAnimationForEvent(CozmoEvent cozEvent, string animGroupName) {
    if (AnimationGroupDict.ContainsKey(cozEvent)) {
      AnimationGroupDict.Remove(cozEvent);
      return true;
    }
    DAS.Error(this, "CozmoEvent doesn't have an Animation Assigned, can't remove what doesn't exist");
    return false;
  }

}
