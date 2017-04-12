using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using Newtonsoft.Json;

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
        Tag = "";
      }

      /// <summary>
      /// The Id of this action for use in the editor.
      /// </summary>
      public uint Id;
      public SerializableGameEvents RewardEvent;
      /// <summary>
      /// The tag group of this reward.
      /// </summary>
      public string Tag;
      /// <summary>
      /// The Conditions for if this reward will be granted when its event is fired.
      /// </summary>
      public List<GoalCondition> EventConditions = new List<GoalCondition>();
      public RewardData Reward;

      /// <summary>
      /// Creates a copy of this instance and returns it with an Id of 0
      /// </summary>
      public RewardedActionData Copy() {
        RewardedActionData newReward;
        string json = "";
        json = JsonConvert.SerializeObject(this, Formatting.Indented, new JsonSerializerSettings {
          TypeNameHandling = TypeNameHandling.Auto
        });
        newReward = JsonConvert.DeserializeObject<RewardedActionData>(json, new JsonSerializerSettings {
          TypeNameHandling = TypeNameHandling.Auto
        });
        newReward.Id = 0;
        return newReward;
      }
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