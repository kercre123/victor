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
/// Goal condition that specifies a day of the week that must be met.
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class TotalSessionsCondition : GoalCondition {
      

      public bool UseMinSession;
      public int MinSession;
      public bool UseMaxSession;
      public int MaxSession;

      // Returns true if day of the week is the desired day of the week
      public override bool ConditionMet() {
        bool isMet = false;
        isMet = !UseMinSession || DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.Count >= MinSession;
        if (isMet) {
          isMet = !UseMaxSession || DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.Count <= MaxSession;
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
