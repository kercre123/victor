using UnityEngine;

#if UNITY_EDITOR
using UnityEditor;
#endif
/// <summary>
/// Goal condition that check the time played in a game
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class TimedIntervalCondition : GoalCondition {

      public float TargetTime;
      public ComparisonType compareType;

      // Returns true if the specified player's time played in seconds matches target
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if ((cozEvent is TimedIntervalGameEvent)) {
          isMet = CompareConditionValues(((TimedIntervalGameEvent)cozEvent).TargetTime_Sec, TargetTime, compareType);
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        compareType = (ComparisonType)EditorGUILayout.EnumPopup(compareType);
        TargetTime = EditorGUILayout.FloatField(new GUIContent("Target Time (secs)", "Target TOTAL Time to check when the Interval Event fires"), TargetTime);
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
