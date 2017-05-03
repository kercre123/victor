using Cozmo.Challenge;
using UnityEngine;



#if UNITY_EDITOR
using UnityEditor;
#endif
/// <summary>
/// End of Challenge condition that returns -1 unless a new difficulty has been unlocked
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class ChallengeDifficultyUnlockedCondition : GoalCondition {
      [ChallengeId]
      public string ChallengeID;
      public int Difficulty;

      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if (cozEvent is DifficultyUnlockedGameEvent) {
          DifficultyUnlockedGameEvent challengeEvent = (DifficultyUnlockedGameEvent)cozEvent;
          if (challengeEvent.GameID == ChallengeID &&
              (challengeEvent.NewDifficulty > 0 && challengeEvent.NewDifficulty >= Difficulty)) {
            isMet = true;
          }
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        ChallengeID = ChallengeIDCondition.DrawChallengeID(ChallengeID);
        Difficulty = EditorGUILayout.IntField(new GUIContent("Difficulty", "Newly unlocked difficulty level from OnChallengeDifficultyUnlock"), Difficulty);
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
