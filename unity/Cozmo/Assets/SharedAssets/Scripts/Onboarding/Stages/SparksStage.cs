using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class SparksStage : OnboardingBaseStage {


    public override void Start() {
      base.Start();

      UnlockablesManager.Instance.OnSparkComplete += HandleSparkComplete;
    }

    public void OnDestroy() {
      UnlockablesManager.Instance.OnSparkComplete -= HandleSparkComplete;
    }

    public void HandleSparkComplete(CoreUpgradeDetailsDialog dialog) {
      dialog.CloseView();
      OnboardingManager.Instance.GoToNextStage();
    }
  }

}
