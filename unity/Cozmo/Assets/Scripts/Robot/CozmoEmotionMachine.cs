using UnityEngine;
using System.Collections;
using System;
using System.Collections.Generic;

[Serializable]
public struct CozmoAnimation {
  public string animName;
  public uint numLoops;
}

[Serializable]
public struct CozmoEmotionState {
  //public string stateName;
  public string stateName;
  public bool playAllAnims;
  public List<CozmoAnimation> emotionAnims;
  
}

/// <summary>
/// a specific matrix of emote states (animation/behaviors) that can populated within the inspector
///   and then be triggered by string identifier
/// </summary>
public class CozmoEmotionMachine : MonoBehaviour {

  public string defaultIdleAnimationName;
  public List<CozmoEmotionState> emotionStates;
  private Dictionary<string, List<CozmoAnimation>> typeAnims = new Dictionary<string, List<CozmoAnimation>>();
  private Dictionary<string, CozmoEmotionState> statesByName = new Dictionary<string, CozmoEmotionState>();

  public void Awake() {
    // populate our helper look up
    for (int i = 0; i < emotionStates.Count; i++) {
      if (string.IsNullOrEmpty(emotionStates[i].stateName)) {
        DAS.Error("CozmoEmotionMachine", "trying to add state with no name");
      }
      else {
        if (!statesByName.ContainsKey(emotionStates[i].stateName)) {
          typeAnims.Add(emotionStates[i].stateName, emotionStates[i].emotionAnims);
          statesByName.Add(emotionStates[i].stateName, emotionStates[i]);
        }
        else {
          DAS.Error("CozmoEmotionMachine", "trying to add " + emotionStates[i].stateName + " more than once");
        }
      }
    }

  }

  public void OnEnable() {
    StartMachine();
  }

  
  public void OnDisable() {
    CloseMachine();
  }

  // initailization
  public void StartMachine() {
    if (CozmoEmotionManager.instance != null) {
      CozmoEmotionManager.instance.RegisterMachine(this);
      CozmoEmotionManager.instance.SetIdleAnimation(defaultIdleAnimationName);
    }
    InitializeMachine();
  }

  public virtual void InitializeMachine() {
  }

  public virtual void UpdateMachine() {
  }
  
  // clean-up
  public void CloseMachine() {
    if (CozmoEmotionManager.instance != null) {
      CozmoEmotionManager.instance.UnregisterMachine();
    }
    CleanUp();
  }

  public virtual void CleanUp() {
  }

  public bool HasAnimForState(string state_name) {
    if (typeAnims.ContainsKey(state_name) && typeAnims[state_name].Count > 0)
      return true;
    return false;
  }

  public List<CozmoAnimation> GetAnimsForType(string state_name) {
    return typeAnims[state_name];
  }

  public CozmoEmotionState GetStateForName(string state_name) {
    return statesByName[state_name];
  }
  
}
