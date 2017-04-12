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
/// Goal condition that specifies the Behavior type for the desired behavior object in a BehaviorSuccessGameEvent.
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class FreeplayBehaviorCondition : GoalCondition {

      public BehaviorObjective Behavior;

      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if (cozEvent is BehaviorSuccessGameEvent) {
          BehaviorSuccessGameEvent behaviorEvent = (BehaviorSuccessGameEvent)cozEvent;
          if (behaviorEvent.Behavior == Behavior) {
            isMet = true;
          }
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        Behavior = (BehaviorObjective)EditorGUILayout.EnumPopup(new GUIContent("Behavior", "The type of behavior that is succeeding as part of a Freeplay Behavior success event"), (Enum)Behavior);
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
