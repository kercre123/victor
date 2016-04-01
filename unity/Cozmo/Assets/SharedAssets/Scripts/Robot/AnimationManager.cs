using System;
using UnityEngine;
using Anki.Cozmo;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json;
using System.IO;

/// <summary>
/// Manager for calling AnimationGroups that are mapped to game events
/// </summary>
public class AnimationManager {

  AnimationEventMap _CurrentEventMap;

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

  public void LoadAnimationMap(string path) {
    AnimationGroupDict.Clear();
    string json = File.ReadAllText(path);
    _CurrentEventMap = JsonConvert.DeserializeObject<AnimationEventMap>(json, GlobalSerializerSettings.JsonSettings);
    for (int i = 0; i < _CurrentEventMap.Pairs.Count; i++) {
      AnimationEventMap.CladAnimPair currPair = _CurrentEventMap.Pairs[i];
      if (!string.IsNullOrEmpty(currPair.AnimName)) {
        if (!AnimationGroupDict.ContainsKey(currPair.CladEvent)) {
          AnimationGroupDict.Add(currPair.CladEvent, currPair.AnimName);
        }
      }
    }
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
