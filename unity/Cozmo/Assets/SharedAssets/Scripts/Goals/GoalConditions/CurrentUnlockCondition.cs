using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;


#if UNITY_EDITOR
using UnityEditor;
#endif
/// <summary>
/// Goal condition that checks to see if an UnlockID is unlocked or locked still
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class CurrentUnlockCondition : GoalCondition {

      public UnlockId Unlocked;
      public bool IsUnlocked;

      // Returns true if the UnlockID's unlock status matches the isUnlocked flag
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        isMet = (UnlockablesManager.Instance.IsUnlocked(Unlocked) == IsUnlocked);
        return isMet;
      }

      #if UNITY_EDITOR
      public override void DrawControls() {
        Unlocked = (UnlockId)EditorGUILayout.EnumPopup("Unlock", (Enum)Unlocked);
        IsUnlocked = EditorGUILayout.Toggle(new GUIContent("IsUnlocked", "Condition is true if unlock ID state matches this flag"), IsUnlocked);
      }
      #endif
    }
  }
}
