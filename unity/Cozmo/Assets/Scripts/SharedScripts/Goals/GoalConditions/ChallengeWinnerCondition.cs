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
/// Goal condition that specified who won
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class ChallengeWinnerCondition : GoalCondition {
     
      public bool PlayerWin;

      // Returns true if the game event went in the favor of the specified player.
      // Playerwin == true if the player just scored/won round/won game
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if (cozEvent is MinigameGameEvent) {
          MinigameGameEvent miniGameEvent = (MinigameGameEvent)cozEvent;
          if (miniGameEvent.PlayerWin == PlayerWin) {
            isMet = true;
          }
        }
        return isMet;
      }

      #if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        PlayerWin = EditorGUILayout.Toggle(new GUIContent("PlayerWin", "PlayerWin is true if the player has 'won' this event. Includes scoring a point, winning a full round, and winning a full game"), PlayerWin);
        EditorGUILayout.EndHorizontal();
      }
      #endif
    }
  }
}
