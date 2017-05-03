using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using Cozmo;
using DataPersistence;
using Cozmo.HomeHub;



#if UNITY_EDITOR
using UnityEditor;
#endif
/// <summary>
/// Goal condition that check the time played in a game
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class ChallengeTimePlayedCondition : GoalCondition {

      public float TargetTime;
      public ComparisonType compareType;

      // Returns true if the specified player's time played in seconds matches target
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if ((cozEvent is ChallengeGameEvent)) {
          GameBase challengeInstance = HubWorldBase.Instance.GetChallengeInstance();
          if (challengeInstance == null) { return false; }
          isMet = CompareConditionValues(challengeInstance.GetGameTimeElapsedInSeconds(), TargetTime, compareType);
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        compareType = (ComparisonType)EditorGUILayout.EnumPopup(compareType);
        TargetTime = EditorGUILayout.FloatField(new GUIContent("Target Time (secs)", "Target Time played since start of the minigame to check"), TargetTime);
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
