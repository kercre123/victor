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
/// Goal condition that specifies the Current Game completed
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class ChallengeIDCondition : GoalCondition {
      
      [ChallengeId]
      public string ChallengeID;

      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if (cozEvent is MinigameGameEvent) {
          MinigameGameEvent miniGameEvent = (MinigameGameEvent)cozEvent;
          if (miniGameEvent.GameID == ChallengeID) {
            isMet = true;
          }
        }
        return isMet;
      }

      #if UNITY_EDITOR
      public override void DrawControls() {
        ChallengeID = EditorGUILayout.TextField("ChallengeID", ChallengeID);
      }
      #endif
    }
  }
}
