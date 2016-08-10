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
/// End of Challenge condition that returns -1 unless a new difficulty has been unlocked
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class ChallengeDifficultyUnlockedCondition : GoalCondition {
      [ChallengeId]
      public string ChallengeID;
      public int Difficulty;

      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if (cozEvent is DifficultyUnlockedGameEvent) {
          DifficultyUnlockedGameEvent miniGameEvent = (DifficultyUnlockedGameEvent)cozEvent;
          if (miniGameEvent.GameID == ChallengeID &&
              (miniGameEvent.NewDifficulty > 0 && miniGameEvent.NewDifficulty >= Difficulty)) {
            isMet = true;
          }
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        ChallengeID = ChallengeIDCondition.DrawChallengeID(ChallengeID);
        Difficulty = EditorGUILayout.IntField(new GUIContent("Difficulty", "Newly unlocked difficulty level from OnChallengeDifficultyUnlock"), Difficulty);
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
