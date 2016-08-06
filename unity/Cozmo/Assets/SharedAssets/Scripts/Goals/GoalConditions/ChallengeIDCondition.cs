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
      private string[] _ChallengeIDs = null;

      public override void DrawControls() {
        _ChallengeIDs = GetAllChallengeIds();
        int currentOption = 0;
        string currentValue = ChallengeID;
        if (!string.IsNullOrEmpty(currentValue)) {
          for (int i = 0; i < _ChallengeIDs.Length; i++) {
            if (_ChallengeIDs[i] == currentValue) {
              currentOption = i;
              break;
            }
          }
        }

        int newOption = EditorGUI.Popup(EditorGUILayout.GetControlRect(), currentOption, _ChallengeIDs);
        ChallengeID = _ChallengeIDs[newOption];

      }

      private string[] GetAllChallengeIds() {
        ChallengeDataList challengeDataConfig = AssetDatabase.LoadAssetAtPath<ChallengeDataList>(kChallengeDataConfigLocation);
        List<string> allIds = new List<string>();
        allIds.AddRange(challengeDataConfig.EditorGetIds());
        return allIds.ToArray();
      }
#endif
    }
  }
}
