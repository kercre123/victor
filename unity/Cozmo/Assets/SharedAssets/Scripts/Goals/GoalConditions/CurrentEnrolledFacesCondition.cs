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
/// Goal condition that checks the amount of enrolled faces saved to robot
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class CurrentEnrolledFacesCondition : GoalCondition {

      public int TargetFaces;
      public ComparisonType compareType;

      // Returns true if the specified player's time played in seconds matches target
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        int faceCount = 0;
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          faceCount = RobotEngineManager.Instance.CurrentRobot.EnrolledFaces.Count;
        }
        isMet = CompareConditionValues(faceCount, TargetFaces, compareType);
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        compareType = (ComparisonType)EditorGUILayout.EnumPopup(compareType);
        TargetFaces = EditorGUILayout.IntField(new GUIContent("Target Faces", "Target Number of Enrolled Faces to check for"), TargetFaces);
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
