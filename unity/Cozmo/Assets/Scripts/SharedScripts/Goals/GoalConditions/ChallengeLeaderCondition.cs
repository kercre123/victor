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
/// Goal condition that check if the player has a higher score
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class ChallengeLeaderCondition : GoalCondition {

      public bool IsPlayerWinning;

      // Returns true if the specified player has more points
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if (cozEvent is ChallengeGameEvent) {
          ChallengeGameEvent challengeEvent = (ChallengeGameEvent)cozEvent;
          if (challengeEvent.PlayerScore > challengeEvent.CozmoScore) {
            isMet = IsPlayerWinning;
          }
          else {
            isMet = !IsPlayerWinning;
          }
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        IsPlayerWinning = EditorGUILayout.Toggle(new GUIContent("Player Winning", "True if the PlayerScore is greater than CozmoScore"), IsPlayerWinning);
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
