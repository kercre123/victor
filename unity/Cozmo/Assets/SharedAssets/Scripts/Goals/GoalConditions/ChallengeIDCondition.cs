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
/// Goal condition that specifies the Current Game completed
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class ChallengeIDCondition : GoalCondition {

      [ChallengeId]
      public string ChallengeID;

      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if (cozEvent is MinigameGameEvent) {
          MinigameGameEvent miniGameEvent = (MinigameGameEvent)cozEvent;
          if (miniGameEvent.GameID == ChallengeID) {
            isMet = true;
          }
        }
        return isMet;
      }

#if UNITY_EDITOR

      private const string kChallengeDataConfigLocation = "Assets/AssetBundles/GameMetadata-Bundle/ChallengeData/ChallengeList.asset";
      private static string[] _ChallengeIDs = null;
      // Forgive Me. -R.A.
      public static string DrawChallengeID(string toDraw) {
        ChallengeDataList challengeDataConfig = AssetDatabase.LoadAssetAtPath<ChallengeDataList>(kChallengeDataConfigLocation);
        List<string> allIds = new List<string>();
        List<string> displayIds = new List<string>();
        allIds.AddRange(challengeDataConfig.EditorGetIds());
        _ChallengeIDs = allIds.ToArray();
        int currentOption = 0;
        string currentValue = toDraw;
        if (!string.IsNullOrEmpty(currentValue)) {
          for (int i = 0; i < _ChallengeIDs.Length; i++) {
            if (_ChallengeIDs[i] == currentValue) {
              currentOption = i;
              break;
            }
          }
        }
        for (int i = 0; i < challengeDataConfig.ChallengeData.Length; i++) {
          displayIds.Add((challengeDataConfig.ChallengeData[i].UnlockId).Value.ToString());
        }
        string[] _DrawIDs = displayIds.ToArray();
        int newOption = EditorGUI.Popup(EditorGUILayout.GetControlRect(), currentOption, _DrawIDs);
        toDraw = _ChallengeIDs[newOption];
        return toDraw;
      }

      public override void DrawControls() {
        ChallengeID = DrawChallengeID(ChallengeID);
      }

#endif
    }
  }
}
