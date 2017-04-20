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
/// Goal condition that check the total score in a game
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class ChallengeTotalScoreCondition : GoalCondition {

      public bool IsPlayer;
      public int TargetScore;
      public ComparisonType compareType;

      // Returns true if the specified player's score matches the target range
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if ((cozEvent is MinigameGameEvent)) {
          GameBase miniGameInstance = HubWorldBase.Instance.GetMinigameInstance();
          if (miniGameInstance == null) { return false; }
          int toCheck = 0;
          if (IsPlayer) {
            toCheck = miniGameInstance.HumanScoreTotal;
          }
          else {
            toCheck = miniGameInstance.CozmoScoreTotal;
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
