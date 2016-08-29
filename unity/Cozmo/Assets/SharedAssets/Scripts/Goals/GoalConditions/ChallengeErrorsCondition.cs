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
/// Goal condition that check the player errors in a game
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class ChallengeErrorsCondition : GoalCondition {

      public ComparisonType compareType;
      public int TargetErrors;

      // Returns true if the specified player's errors are equal to the target
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if ((cozEvent is MinigameGameEvent)) {
          GameBase miniGameInstance = HomeHub.Instance.MiniGameInstance;
          if (miniGameInstance == null) { return false; }
          isMet = CompareConditionValues(miniGameInstance.PlayerMistakeCount, TargetErrors, compareType);
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        compareType = (ComparisonType)EditorGUILayout.EnumPopup(compareType);
        TargetErrors = EditorGUILayout.IntField(new GUIContent("Target Errors", "Target errors to check"), TargetErrors);
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
