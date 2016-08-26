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

      public bool IsPlayer;
      public float TargetAcc;
      public ComparisonType compareType;

      // Returns true if the specified player's % points earned out of total matches the target
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if ((cozEvent is MinigameGameEvent)) {
          GameBase miniGameInstance = HomeHub.Instance.MiniGameInstance;
          if (miniGameInstance == null) { return false; }
          int toCheck = 0;
          if (IsPlayer) {
            toCheck = miniGameInstance.PlayerScoreTotal;
          }
          else {
            toCheck = miniGameInstance.CozmoScoreTotal;
          }
          float acc = ((float)toCheck / (float)(miniGameInstance.PlayerScoreTotal + miniGameInstance.CozmoScoreTotal));
          isMet = CompareConditionValues(acc, TargetAcc, compareType);
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        IsPlayer = EditorGUILayout.Toggle(new GUIContent("Check Player", "True if we are checking PlayerScore, False if we are checking CozmoScore"), IsPlayer);
        compareType = (ComparisonType)EditorGUILayout.EnumPopup(compareType);
        TargetAcc = EditorGUILayout.Slider(TargetAcc, 0.0f, 1.0f);
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
