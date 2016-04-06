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
    set {
      if (_Instance != null) {
        _Instance = value;
      }
    }
  }

  private IRobot _CurrRobot = null;

  // Map Animation Group Names to Event Enums
  public Dictionary<GameEvent, string> AnimationGroupDict = new Dictionary<GameEvent, string>();

  public static string sEventMapDirectory { get { return Application.dataPath + "/../../../lib/anki/products-cozmo-assets/animationGroupMaps"; } }

  public AnimationManager() {
    GameEventManager.Instance.OnGameEvent += GameEventReceived;
    _CurrRobot = RobotEngineManager.Instance.CurrentRobot;
    // Load all Event Map Configs (Can have multiple, so you can create different configs, game only uses one.)
    if (Directory.Exists(sEventMapDirectory)) {
      string[] _EventMapFiles = Directory.GetFiles(sEventMapDirectory);
      // TODO : Specify the event map file to use in a config
      LoadAnimationMap(_EventMapFiles[0]);
    }
    else {
      DAS.Warn(this, "No Animation Event Map to load in products-cozmo-assets/animationGroupMaps/");
    }
  }

  public void LoadAnimationMap(string path) {
    AnimationGroupDict.Clear();
    string json = File.ReadAllText(path);
    DAS.Event(this, string.Format("LoadAnimationMap {0}", Path.GetFileName(path)));
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

  public void GameEventReceived(GameEvent cozEvent) {
    string animGroup = "";
    if (AnimationGroupDict.TryGetValue(cozEvent, out animGroup)) {
      if (!string.IsNullOrEmpty(animGroup)) {
        _CurrRobot.SendAnimationGroup(animGroup);
      }
    }
  }

}
