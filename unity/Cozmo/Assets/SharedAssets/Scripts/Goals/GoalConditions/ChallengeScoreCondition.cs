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
/// Goal condition that check if a player/cozmo score value is high enough
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class ChallengeScoreCondition : GoalCondition {
     
      public bool IsPlayer;
      public int TargetScore;

      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if (cozEvent is MinigameGameEvent) {
          MinigameGameEvent miniGameEvent = (MinigameGameEvent)cozEvent;
          int toCheck = 0;
          if (IsPlayer) {
            toCheck = miniGameEvent.PlayerScore;
          }
          else {
            toCheck = miniGameEvent.CozmoScore;
          }
          if (toCheck >= TargetScore) {
            isMet = true;
          }
        }
        return isMet;
      }

      #if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        IsPlayer = EditorGUILayout.Toggle(new GUIContent("Check Player", "True if we are checking PlayerScore, False if we are checking CozmoScore"), IsPlayer);
        TargetScore = EditorGUILayout.IntField(new GUIContent("Target Score", "Score to check, returns true if current score is equal to or greater than this"), TargetScore);
        EditorGUILayout.EndHorizontal();
      }
      #endif
    }
  }
}
