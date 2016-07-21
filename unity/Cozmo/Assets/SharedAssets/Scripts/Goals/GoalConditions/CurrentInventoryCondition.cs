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
/// Inventory condition, specifies an item type and amount needed
/// </summary>
namespace Anki {
  namespace Cozmo {
    [System.Serializable]
    public class CurrentInventoryCondition : GoalCondition {
      
      /// <summary>
      /// The Item type of the reward.
      /// </summary>
      [ItemId]
      public string ItemType;

      public int ItemCount;

      // Returns true if day of the week is the desired day of the week
      public override bool ConditionMet(GameEventWrapper cozEvent = null) {
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
