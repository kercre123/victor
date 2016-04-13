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

  public IRobot CurrentRobot {
    get {
      if (_CurrRobot == null) {
        _CurrRobot = RobotEngineManager.Instance.CurrentRobot;
      }
      return _CurrRobot;
    }
  }

  // Map Animation Group Names to Event Enums using the tool
  public Dictionary<GameEvent, string> AnimationGroupDict = new Dictionary<GameEvent, string>();
  // Map RobotCallbacks to GameEvents instead of AnimationGroups to separate game logic from Animation names
  private Dictionary<GameEvent, RobotCallback> AnimationCallbackDict = new Dictionary<GameEvent, RobotCallback>();

  #if UNITY_IOS && !UNITY_EDITOR
  public static string sEventMapDirectory { get { return  Path.Combine(Application.dataPath, "../cozmo_resources/assets/animationGroupMaps"); } }



#else
  public static string sEventMapDirectory { get { return Application.dataPath + "/../../../lib/anki/products-cozmo-assets/animationGroupMaps"; } }
  #endif
  /*
  public AnimationManager() {
  }*/

  public void Initialize() {
    GameEventManager.Instance.OnGameEvent += GameEventReceived;
    // Load all Event Map Configs (Can have multiple, so you can create different configs, game only uses one.)
    if (Directory.Exists(sEventMapDirectory)) {
      string[] _EventMapFiles = Directory.GetFiles(sEventMapDirectory);
      // TODO : Specify the event map file to use in a config
      if (_EventMapFiles.Length > 0) {
        LoadAnimationMap(_EventMapFiles[0]);
      }
      else {
        DAS.Warn(this, string.Format("No Animation Event Map to load in {0}"));
      }
    }
    else {
      DAS.Warn(this, string.Format("No Animation Event Map to load in {0}"));
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
    if (!AnimationGroupDict.ContainsKey(cozEvent)) {
      DAS.Error(this, string.Format("GameEvent {0} doesn't exist within AnimationGroupDict. Have you Initialized the AnimationManager?", cozEvent));
      return;
    }
    if (AnimationGroupDict.TryGetValue(cozEvent, out animGroup)) {
      RobotCallback newCallback = null;
      if (!string.IsNullOrEmpty(animGroup)) {
        AnimationCallbackDict.TryGetValue(cozEvent, out newCallback);
        Debug.LogWarning(string.Format("Received Event from EventManager, sending animgroup : {0}", animGroup));
        CurrentRobot.SendAnimationGroup(animGroup, newCallback);
      }
      else if (AnimationCallbackDict.TryGetValue(cozEvent, out newCallback)) {
        DAS.Warn(this, string.Format("GameEvent {0} has an animation callback, but no animation group", cozEvent));
        newCallback.Invoke(true);
      }
    }
  }

  /// <summary>
  /// Adds the specified callback to be fired at the end of the AnimationGroup that is called from
  /// the specified GameEvent.
  /// </summary>
  /// <param name="cozEvent">gameEvent</param>
  /// <param name="newCallback">RobotCallback</param>
  public void AddAnimationEndedCallback(GameEvent cozEvent, RobotCallback newCallback) {
    if (!AnimationCallbackDict.ContainsKey(cozEvent)) {
      AnimationCallbackDict.Add(cozEvent, newCallback);
    }
    else {
      AnimationCallbackDict[cozEvent] += newCallback;
    }
  }

  /// <summary>
  /// Removes the specified callback to be fired at the end of the AnimationGroup that is called from
  /// the specified GameEvent.
  /// </summary>
  /// <param name="cozEvent">gameEvent</param>
  /// <param name="newCallback">RobotCallback</param>
  public void RemoveAnimationEndedCallback(GameEvent cozEvent, RobotCallback toRemove) {
    if (AnimationCallbackDict.ContainsKey(cozEvent)) {
      AnimationCallbackDict[cozEvent] -= toRemove;
    }
  }


}
