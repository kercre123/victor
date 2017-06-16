using Cozmo.UI;
using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo.ExternalInterface;
using DataPersistence;

namespace Onboarding {

  // This screen is either initial
  public class NurtureIntroStage : ShowContinueStage {
    [SerializeField]
    private CozmoText _TextFieldInstance;

    protected override void Awake() {
      base.Awake();
      PlayerProfile profile = DataPersistenceManager.Instance.Data.DefaultProfile;
      Cozmo.ItemData itemData = Cozmo.ItemDataConfig.GetData(GenericRewardsConfig.Instance.SparkID);
      int giveSparksAmount = itemData.StartingAmount;
      // If the robot has a lot of sparks give them that value.
      int currentSparks = profile.Inventory.GetItemAmount(GenericRewardsConfig.Instance.SparkID);
      if (OnboardingManager.Instance.IsReturningUser()) {
        _TextFieldInstance.text = Localization.Get(LocalizationKeys.kOnboardingNeedsIntroReturning);
      }
      // If they have old "treats" saved on the app, which are now sparks, Give them those.
      int oldSparksAmount = profile.Inventory.GetItemAmount(GenericRewardsConfig.Instance.TreatID);
      // We just want to be generious so they either get:
      // 1. An old user from the old app, give them treat sparks
      // 2. A brand new user gets a default amount of start sparks
      // 3. A device connecting to a robot that has had some nurture on it already might have that value if it's large.
      //      Since new sparks are saved on the robot.
      giveSparksAmount = Mathf.Max(oldSparksAmount, giveSparksAmount, currentSparks);
      if (giveSparksAmount != currentSparks) {
        profile.Inventory.SetItemAmount(itemData.ID, giveSparksAmount);
      }
    }

  }

}
