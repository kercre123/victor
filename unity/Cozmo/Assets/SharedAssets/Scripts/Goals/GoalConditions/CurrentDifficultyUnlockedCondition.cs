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
/// Checks the current difficulty of the desired Challenge
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class CurrentDifficultyUnlockedCondition : GoalCondition {
      [ChallengeId]
      public string ChallengeID;
      public bool UseMinDiff;
      public int MinDiff;
      public bool UseMaxDiff;
      public int MaxDiff;

      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        int diff = 0;
        ChallengeData data = ChallengeDataList.Instance.GetChallengeDataById(ChallengeID);
        if (data && UnlockablesManager.Instance.IsUnlocked(data.UnlockId.Value)) {
          // if not found, then we're at difficulty 0
          DataPersistenceManager.Instance.Data.DefaultProfile.GameDifficulty.TryGetValue(ChallengeID, out diff);
          isMet = !UseMinDiff || diff >= MinDiff;
          if (isMet) {
            isMet = !UseMaxDiff || diff <= MaxDiff;
          }
        }
        return isMet;
      }

#if UNITY_EDITOR
      public override void DrawControls() {

        ChallengeID = ChallengeIDCondition.DrawChallengeID(ChallengeID);
        EditorGUILayout.BeginHorizontal();
        UseMinDiff = EditorGUILayout.Toggle("Use Min", UseMinDiff);
        if (UseMinDiff) {
          MinDiff = EditorGUILayout.IntField("Min Diffs", MinDiff);
        }
        EditorGUILayout.EndHorizontal();
        EditorGUILayout.BeginHorizontal();
        UseMaxDiff = EditorGUILayout.Toggle("Use Max", UseMaxDiff);
        if (UseMaxDiff) {
          MaxDiff = EditorGUILayout.IntField("Max Diffs", MaxDiff);
        }
        EditorGUILayout.EndHorizontal();
      }
#endif
    }
  }
}
