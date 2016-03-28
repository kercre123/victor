using UnityEngine;
using Anki.Cozmo;
using System.Collections;
using System.Collections.Generic;

public class AnimationEventManager {
  
  public static AnimationEventManager Instance = null;

  // TODO: Replace this with CLAD generated file to match engine events.
  public enum AnimationEvent {
    SPEEDTAP_TAP,
    SPEEDTAP_WIN_HAND,
    SPEEDTAP_WIN_ROUND,
    SPEEDTAP_WIN_SESSION,
    SPEEDTAP_LOSE_HAND,
    SPEEDTAP_LOSE_ROUND,
    SPEEDTAP_LOSE_SESSION,
    SPEEDTAP_IDLE,
    SPEEDTAP_FAKE
  }

  // Dictionary that maps animation Events to specific animation groups.
  public Dictionary<AnimationEvent,string> AnimationEventDict = new Dictionary<AnimationEvent, string>();

  public void PlayAnimation(AnimationEvent animE, RobotCallback callback = null) {
    Robot currRob = (Robot)RobotEngineManager.Instance.CurrentRobot;
    string animGroup = "";
    if (currRob == null || AnimationEventDict.TryGetValue(animE, out animGroup) == false) {
      return;
    }
    currRob.SendAnimationGroup(animGroup, callback);
  }
}
