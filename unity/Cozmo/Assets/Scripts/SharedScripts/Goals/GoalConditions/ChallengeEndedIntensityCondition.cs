using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using Cozmo;
using DataPersistence;



#if UNITY_EDITOR
using UnityEditor;
#endif
/// <summary>
/// End of Challenge condition that check if the challenge was a high intensity match
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class ChallengeEndedIntensityCondition : GoalCondition {
     
      public bool IsHighIntensity;

      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if (cozEvent is MinigameGameEvent) {
          MinigameGameEvent miniGameEvent = (MinigameGameEvent)cozEvent;
          if (miniGameEvent.HighIntensity == IsHighIntensity) {
            isMet = true;
          }
        }
        return isMet;
      }

      #if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        IsHighIntensity = EditorGUILayout.Toggle(new GUIContent("HighIntensity", "Condition is true if results match this flag"), IsHighIntensity);
        EditorGUILayout.EndHorizontal();
      }
      #endif
    }
  }
}
