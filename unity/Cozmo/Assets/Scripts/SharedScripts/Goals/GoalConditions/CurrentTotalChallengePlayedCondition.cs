using Cozmo.Challenge;
using DataPersistence;



#if UNITY_EDITOR
using UnityEditor;
#endif
/// <summary>
/// Condition that is met if you have completed the specified Challenge the specified amount of times
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class CurrentTotalChallengePlayedCondition : GoalCondition {
      [ChallengeId]
      public string ChallengeID;
      public bool UseMinGames;
      public int MinGames;
      public bool UseMaxGames;
      public int MaxGames;

      // Returns true if you have played the requested challenge enough times
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        int played = 0;
        PlayerProfile pProf = DataPersistenceManager.Instance.Data.DefaultProfile;
        if (pProf != null && pProf.TotalGamesPlayed.ContainsKey(ChallengeID)) {
          played = pProf.TotalGamesPlayed[ChallengeID];
        }
        isMet = !UseMinGames || played >= MinGames;
        if (isMet) {
          isMet = !UseMaxGames || played <= MaxGames;
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        ChallengeID = ChallengeIDCondition.DrawChallengeID(ChallengeID);
        EditorGUILayout.BeginHorizontal();
        UseMinGames = EditorGUILayout.Toggle("Use Min", UseMinGames);
        if (UseMinGames) {
          MinGames = EditorGUILayout.IntField("Min Games", MinGames);
        }
        EditorGUILayout.EndHorizontal();
        EditorGUILayout.BeginHorizontal();
        UseMaxGames = EditorGUILayout.Toggle("Use Max", UseMaxGames);
        if (UseMaxGames) {
          MaxGames = EditorGUILayout.IntField("Max Games", MaxGames);
        }
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
