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
      public ComparisonType compareType;

      // Returns true if the specified player's score is equal to or greater than the target
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if (cozEvent is ChallengeGameEvent) {
          ChallengeGameEvent challengeEvent = (ChallengeGameEvent)cozEvent;
          int toCheck = 0;
          if (IsPlayer) {
            toCheck = challengeEvent.PlayerScore;
          }
          else {
            toCheck = challengeEvent.CozmoScore;
          }
          isMet = CompareConditionValues(toCheck, TargetScore, compareType);
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        IsPlayer = EditorGUILayout.Toggle(new GUIContent("Check Player", "True if we are checking PlayerScore, False if we are checking CozmoScore"), IsPlayer);
        compareType = (ComparisonType)EditorGUILayout.EnumPopup(compareType);
        TargetScore = EditorGUILayout.IntField(new GUIContent("Target Score", "Target points to check"), TargetScore);
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
