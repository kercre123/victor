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
    public class ChallengeDifficultyCondition : GoalCondition {

      public int Difficulty;

      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if (cozEvent is ChallengeGameEvent) {
          ChallengeGameEvent challengeEvent = (ChallengeGameEvent)cozEvent;
          if (challengeEvent.Difficulty == Difficulty) {
            isMet = true;
          }
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        Difficulty = EditorGUILayout.IntField(new GUIContent("Difficulty", "Current difficulty level of the challenge"), Difficulty);
        EditorGUILayout.EndHorizontal();
      }

#endif
    }
  }
}
