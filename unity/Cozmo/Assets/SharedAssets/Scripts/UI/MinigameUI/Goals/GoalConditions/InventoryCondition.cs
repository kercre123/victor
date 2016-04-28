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
/// Goal condition that specifies a day of the week that must be met.
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class InventoryCondition : GoalCondition {
      
      /// <summary>
      /// The Item type of the reward.
      /// </summary>
      public string ItemType;

      public int ItemCount;

      // Returns true if day of the week is the desired day of the week
      public override bool ConditionMet() {
        return (DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.GetItemAmount(ItemType) >= ItemCount);
      }

      #if UNITY_EDITOR
      public override void DrawControls() {
        ItemType = EditorGUILayout.TextField("Item Type", ItemType);
        ItemCount = EditorGUILayout.IntField("Count Required", ItemCount);
      }
      #endif
    }
  }
}
