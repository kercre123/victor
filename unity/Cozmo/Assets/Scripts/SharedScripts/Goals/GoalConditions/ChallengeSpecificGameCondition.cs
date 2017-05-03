using UnityEngine;

#if UNITY_EDITOR
using UnityEditor;
#endif
/// <summary>
/// Goal condition if the game specific dictionary has that value that matches compare type
/// Useful so we don't keep adding to base class. 
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class ChallengeSpecificGameCondition : GoalCondition {

      public float TargetValue;
      public string ConditionKey;
      public ComparisonType compareType;

      // Returns true if the game specific dictionary has that value, and is met
      // For example checking the presence of "lives" in memory match.
      // Can probably move "Accuracy" to this state since that is only for speedtap and it's in gamebase now.
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if ((cozEvent is ChallengeGameEvent)) {
          ChallengeGameEvent challengeEvent = (ChallengeGameEvent)cozEvent;
          if (challengeEvent.GameSpecificValues != null && challengeEvent.GameSpecificValues.ContainsKey(ConditionKey)) {
            isMet = CompareConditionValues(challengeEvent.GameSpecificValues[ConditionKey], TargetValue, compareType);
          }
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        compareType = (ComparisonType)EditorGUILayout.EnumPopup(compareType);
        ConditionKey = EditorGUILayout.TextField(new GUIContent("ConditionKey", "Condition Key"), ConditionKey);
        TargetValue = EditorGUILayout.FloatField(new GUIContent("TargetValue", "Value To Compare"), TargetValue);
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
