using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

namespace Cozmo {
  namespace UI {

    public class RewardedActionListData {
      public List<RewardedActionData> RewardedActions = new List<RewardedActionData>();
    }

    public class RewardedActionData {

      public RewardedActionData() {
        Id = 0;
        Reward = new RewardData();
        RewardEvent = new SerializableGameEvents();
      }

      /// <summary>
      /// The Id of this action for use in the editor.
      /// </summary>
      public uint Id;
      public SerializableGameEvents RewardEvent;
      /// <summary>
      /// The Conditions for if this reward will be granted when its event is fired.
      /// </summary>
      public List<GoalCondition> EventConditions = new List<GoalCondition>();
      public RewardData Reward;
    }

    [System.Serializable]
    public class RewardData {
      public RewardData() {
        ItemID = "experience";
        Amount = 0;
        DescriptionKey = "reward.description.Missing";
      }

      [ItemId]
      public string ItemID;
      public int Amount;
      public string DescriptionKey;
    }

  }
}