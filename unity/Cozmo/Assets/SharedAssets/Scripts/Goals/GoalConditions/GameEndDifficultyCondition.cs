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
/// Goal condition that specifies Difficulty beat
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class GameEndDifficultyCondition : GoalCondition {
     
      public int Difficulty;

      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if (cozEvent is MinigameCompletedGameEvent) {
          MinigameCompletedGameEvent miniGameEvent = (MinigameCompletedGameEvent)cozEvent;
          if (miniGameEvent.Difficulty <= Difficulty) {
            isMet = true;          
          }
        }
        return isMet;
      }

      #if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        Difficulty = EditorGUILayout.IntField(new GUIContent("Difficulty", "Current Difficulty level of the Game we just played"), Difficulty);
        EditorGUILayout.EndHorizontal();
      }
      #endif
    }
  }
}
