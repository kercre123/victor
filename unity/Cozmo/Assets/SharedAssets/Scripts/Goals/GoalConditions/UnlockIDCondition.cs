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
/// Goal condition that specifies the UnlockID of the desired unlock.
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class UnlockIDCondition : GoalCondition {
     
      public UnlockId Unlocked;

      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        if (cozEvent is UnlockableUnlockedGameEvent) {
          UnlockableUnlockedGameEvent unlockEvent = (UnlockableUnlockedGameEvent)cozEvent;
          if (unlockEvent.Unlock == Unlocked) {
            isMet = true;
          }
        }
        return isMet;
      }

      #if UNITY_EDITOR
      public override void DrawControls() {
        EditorGUILayout.BeginHorizontal();
        Unlocked = (UnlockId)EditorGUILayout.EnumPopup(new GUIContent("Unlock", "The ID of the Unlock being unlocked as part of thise UnlockableUnlocked Game Event"), (Enum)Unlocked);
        EditorGUILayout.EndHorizontal();
      }
      #endif
    }
  }
}
