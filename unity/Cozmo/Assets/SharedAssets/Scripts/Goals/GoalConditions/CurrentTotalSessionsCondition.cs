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
/// Condition that is met if the specified number of sessions have been played
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class CurrentTotalSessionsCondition : GoalCondition {

      public bool UseMinSession;
      public int MinSession;
      public bool UseMaxSession;
      public int MaxSession;

      // Returns true if the desired number of sessions has been played
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        isMet = !UseMinSession || DataPersistenceManager.Instance.Data.DefaultProfile.TotalSessions >= MinSession;
        if (isMet) {
          isMet = !UseMaxSession || DataPersistenceManager.Instance.Data.DefaultProfile.TotalSessions <= MaxSession;
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        UseMinSession = EditorGUILayout.Toggle("Use Min", UseMinSession);
        if (UseMinSession) {
          MinSession = EditorGUILayout.IntField("Min Sessions", MinSession);
        }
        EditorGUILayout.EndHorizontal();
        EditorGUILayout.BeginHorizontal();
        UseMaxSession = EditorGUILayout.Toggle("Use Max", UseMaxSession);
        if (UseMaxSession) {
          MaxSession = EditorGUILayout.IntField("Max Sessions", MaxSession);
        }
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
