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
/// Goal condition that checks the number of rounds won
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class ChallengeRoundsWonCondition : GoalCondition {

      public bool IsPlayer;
      public int RoundsWon;
      public bool LessThan;

      // Returns true if the specified player's score is equal to or greater than the target
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if (cozEvent is MinigameGameEvent) {
          GameBase miniGameInstance = HomeHub.Instance.MiniGameInstance;
          if (miniGameInstance == null) { return false; }
          int toCheck = 0;
          if (IsPlayer) {
            toCheck = miniGameInstance.PlayerRoundsWon;
          }
          else {
            toCheck = miniGameInstance.CozmoRoundsWon;
          }
          if (toCheck >= RoundsWon) {
            isMet = !LessThan;
          }
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        IsPlayer = EditorGUILayout.Toggle(new GUIContent("Check Player", "True if we are checking PlayerScore, False if we are checking CozmoScore"), IsPlayer);
        LessThan = EditorGUILayout.Toggle(new GUIContent("Is Less Than", "If we are checking less than rather than equal to or greater than"), LessThan);
        RoundsWon = EditorGUILayout.IntField(new GUIContent("Target Score", "Target points to check"), RoundsWon);
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
