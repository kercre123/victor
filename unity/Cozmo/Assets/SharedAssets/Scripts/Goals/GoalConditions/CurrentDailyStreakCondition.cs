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
/// Condition that is met if the current streak is in place
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class CurrentDailyStreakCondition : GoalCondition {

      public bool UseMinSession;
      public int MinSession;
      public bool UseMaxSession;
      public int MaxSession;

      // Returns true if the desired number of sessions has been played
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        isMet = !UseMinSession || DataPersistenceManager.Instance.CurrentStreak >= MinSession;
        if (isMet) {
          isMet = !UseMaxSession || DataPersistenceManager.Instance.CurrentStreak <= MaxSession;
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        UseMinSession = EditorGUILayout.Toggle(new GUIContent("Use Min", "If we care about a minimum streak"), UseMinSession);
        if (UseMinSession) {
          MinSession = EditorGUILayout.IntField(new GUIContent("Min Sessions", "The Lowest streak this is true for, inclusive"), MinSession);
        }
        EditorGUILayout.EndHorizontal();
        EditorGUILayout.BeginHorizontal();
        UseMaxSession = EditorGUILayout.Toggle(new GUIContent("Use Max", "If we care about a maximum streak"), UseMaxSession);
        if (UseMaxSession) {
          MaxSession = EditorGUILayout.IntField(new GUIContent("Max Sessions", "The Highest streak this is true for, inclusive"), MaxSession);
        }
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
