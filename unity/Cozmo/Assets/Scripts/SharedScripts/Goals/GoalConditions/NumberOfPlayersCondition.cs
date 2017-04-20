using UnityEngine;
using Cozmo.HomeHub;



#if UNITY_EDITOR
using UnityEditor;
#endif
/// <summary>
/// Goal condition that check the number of players
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class NumberOfPlayersCondition : GoalCondition {

      public int NumPlayers;
      public ComparisonType compareType;

      // Returns true if the number of players match the condition.
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if ((cozEvent is MinigameGameEvent)) {
          GameBase miniGameInstance = HubWorldBase.Instance.GetMinigameInstance();
          if (miniGameInstance == null) {
            return false;
          }
          int toCheck = miniGameInstance.GetPlayerCount();
          isMet = CompareConditionValues(toCheck, NumPlayers, compareType);
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        compareType = (ComparisonType)EditorGUILayout.EnumPopup(compareType);
        NumPlayers = EditorGUILayout.IntField(new GUIContent("Number of Players", "Total Players"), NumPlayers);
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
