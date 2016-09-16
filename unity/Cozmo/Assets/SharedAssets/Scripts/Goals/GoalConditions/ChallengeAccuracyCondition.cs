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
/// Goal condition that check the % score out of total score in a game
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class ChallengeAccuracyCondition : GoalCondition {

      public float TargetAcc;
      public ComparisonType compareType;

      // Returns true if the specified player's % points earned out of total matches the target
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if ((cozEvent is MinigameGameEvent)) {
          GameBase miniGameInstance = HomeHub.Instance.MiniGameInstance;
          if (miniGameInstance == null) { return false; }
          int toCheck = miniGameInstance.PlayerScoreTotal;
          int mistakes = miniGameInstance.PlayerMistakeCount;
          float acc = ((float)toCheck / (float)(toCheck + mistakes));
          isMet = CompareConditionValues(acc, TargetAcc, compareType);
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        compareType = (ComparisonType)EditorGUILayout.EnumPopup(compareType);
        TargetAcc = EditorGUILayout.Slider(TargetAcc, 0.0f, 1.0f);
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
