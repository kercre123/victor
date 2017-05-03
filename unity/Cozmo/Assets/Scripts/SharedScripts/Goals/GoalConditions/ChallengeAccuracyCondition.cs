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
    // If this were implemented fully in the new system it'd be a ChallengeSpecificGameCondition
    // we don't really want to change the rendering though, so just leave Editor facing stuff as is.
    [System.Serializable]
    public class ChallengeAccuracyCondition : GoalCondition {

      public float TargetAcc;
      public ComparisonType compareType;
      public const string kConditionKey = "PlayerAccuracy";

      // Returns true if the specified player's % points earned out of total matches the target
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if ((cozEvent is ChallengeGameEvent)) {
          GameBase challengeInstance = HubWorldBase.Instance.GetChallengeInstance();
          if (challengeInstance == null) { return false; }
          ChallengeGameEvent challengeEvent = (ChallengeGameEvent)cozEvent;
          if (challengeEvent.GameSpecificValues != null && challengeEvent.GameSpecificValues.ContainsKey(kConditionKey)) {
            isMet = CompareConditionValues(challengeEvent.GameSpecificValues[kConditionKey], TargetAcc, compareType);
          }
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
