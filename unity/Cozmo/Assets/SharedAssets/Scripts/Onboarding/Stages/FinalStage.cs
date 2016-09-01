using Cozmo.UI;
using UnityEngine;
using System.Collections.Generic;

namespace Onboarding {

  public class FinalStage : OnboardingBaseStage {

    // Spotlights only certain buttons, but still show them is the design.
    // The first button we're highlighting needs to work, but clicking on anything else should just dismiss this dialog.
    [SerializeField]
    private List<UnityEngine.UI.Button> _ClickBlockers;
    public override void Start() {
      base.Start();
      BaseView.BaseViewOpened += HandleViewChanged;
      BaseView.BaseViewClosed += HandleViewChanged;

      for (int i = 0; i < _ClickBlockers.Count; ++i) {
        _ClickBlockers[i].onClick.AddListener(OnBlockerClicked);
      }
      // Trying to keep the Onboarding as isolated as possible, so rather than making several layers
      // of getters in homeview, just find it
      MonoBehaviour panel = (MonoBehaviour)GameObject.FindObjectOfType(typeof(Cozmo.HomeHub.ChallengeListPanel));
      if (panel != null) {
        OnboardingManager.Instance.SetOutlineRegion(panel.transform);
        OnboardingManager.Instance.ShowOutlineRegion(true);
      }
    }

    public override void OnDestroy() {
      base.OnDestroy();
      BaseView.BaseViewOpened -= HandleViewChanged;
      BaseView.BaseViewClosed -= HandleViewChanged;
    }

    public void OnBlockerClicked() {
      OnboardingManager.Instance.GoToNextStage();
    }

    public void HandleViewChanged(BaseView view) {
      OnboardingManager.Instance.GoToNextStage();
    }
  }

}
