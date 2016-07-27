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

    // Enum for GoalCondition
    public enum UnlockableState {
      Locked,
      Available,
      Unlocked
    }

    [System.Serializable]
    public class CurrentUnlockCondition : GoalCondition {

      public UnlockId Unlocked;
      public bool IsMatching = true;
      public UnlockableState State;

      // Returns true if the UnlockID's unlock status matches the isUnlocked flag
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
        bool isMet = false;
        switch (State) {
        case UnlockableState.Locked:
          isMet = (UnlockablesManager.Instance.IsUnlockableAvailable(Unlocked) == false);
          break;
        case UnlockableState.Available:
          isMet = (((UnlockablesManager.Instance.IsUnlockableAvailable(Unlocked) == true) && (UnlockablesManager.Instance.IsUnlocked(Unlocked) == false)));
          break;
        case UnlockableState.Unlocked:
          isMet = (UnlockablesManager.Instance.IsUnlocked(Unlocked) == true);
          break;
        }
        return (IsMatching == isMet);
      }

      #if UNITY_EDITOR
      public override void DrawControls() {
        Unlocked = (UnlockId)EditorGUILayout.EnumPopup("Unlock", (Enum)Unlocked);
        State = (UnlockableState)EditorGUILayout.EnumPopup(new GUIContent("State", "Locked if not available or unlocked. Available if available but not unlocked. Unlocked if unlocked."), (Enum)State);
        IsMatching = EditorGUILayout.Toggle(new GUIContent("IsMatching", "Are we checking if the unlock is in this state or NOT in this state"), IsMatching);
      }
      #endif
    }
  }
}
