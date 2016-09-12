using Cozmo.UI;
using UnityEngine;
using System.Collections.Generic;

namespace Onboarding {

  public class FinalStage : ShowContinueStage {

    public override void Start() {
      base.Start();
      // Trying to keep the Onboarding as isolated as possible, so rather than making several layers
      // of getters in homeview, just find it
      MonoBehaviour panel = (MonoBehaviour)GameObject.FindObjectOfType(typeof(Cozmo.HomeHub.ChallengeListPanel));
      if (panel != null) {
        OnboardingManager.Instance.SetOutlineRegion(panel.transform);
        OnboardingManager.Instance.ShowOutlineRegion(true);
      }
    }

    protected override void HandleContinueClicked() {
      base.HandleContinueClicked();
      OnboardingManager.Instance.ShowOutlineRegion(false);
    }

  }

}
