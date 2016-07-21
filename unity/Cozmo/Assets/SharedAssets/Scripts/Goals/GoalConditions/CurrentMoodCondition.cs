using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;


#if UNITY_EDITOR
using UnityEditor;
#endif
/// <summary>
/// Goal condition that checks to see if a mood is above or below a certain threshold
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class CurrentMoodCondition : GoalCondition {

      public SimpleMoodType Mood;

      public bool UseMinMood;
      [Range(0.0f, 1.0f)]
      public float MinMood;
      public bool UseMaxMood;
      [Range(0.0f, 1.0f)]
      public float MaxMood;

      // Returns true if the Mood is within the ideal range
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          float eVal = RobotEngineManager.Instance.CurrentRobot.EmotionValues[(int)Mood];
          isMet = !UseMinMood || eVal >= MinMood;
          if (isMet) {
            isMet = !UseMaxMood || eVal >= MaxMood;
          }
        }
        return isMet;
      }

      #if UNITY_EDITOR
      public override void DrawControls() {
        Mood = (Anki.Cozmo.SimpleMoodType)EditorGUILayout.EnumPopup(new GUIContent("Mood", "The Mood value to check"), Mood);
        EditorGUILayout.BeginHorizontal();
        UseMinMood = EditorGUILayout.Toggle(new GUIContent("Use Min", "If we are checking to make sure Mood is above a min value (inclusive)"), UseMinMood);
        if (UseMinMood) {
          MinMood = EditorGUILayout.Slider(new GUIContent("Min Mood", "Min % Mood that will return true"), MinMood, 0.0f, 1.0f);
        }
        EditorGUILayout.EndHorizontal();
        EditorGUILayout.BeginHorizontal();
        UseMaxMood = EditorGUILayout.Toggle(new GUIContent("Use Max", "If we are checking to make sure Mood is below a max value (inclusive)"), UseMaxMood);
        if (UseMaxMood) {
          MaxMood = EditorGUILayout.Slider(new GUIContent("Max Mood", "Max % Mood that will return true"), MaxMood, 0.0f, 1.0f);
        }
        EditorGUILayout.EndHorizontal();

      }
      #endif
    }
  }
}
