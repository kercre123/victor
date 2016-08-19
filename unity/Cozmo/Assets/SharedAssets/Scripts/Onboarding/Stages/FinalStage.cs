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
      BaseView.BaseViewOpened += HandleViewOpened;

      for (int i = 0; i < _ClickBlockers.Count; ++i) {
        _ClickBlockers[i].onClick.AddListener(OnBlockerClicked);
      }
    }

    public override void OnDestroy() {
      base.OnDestroy();
      BaseView.BaseViewOpened -= HandleViewOpened;
    }

    public void OnBlockerClicked() {
      OnboardingManager.Instance.GoToNextStage();
    }

    public void HandleViewOpened(BaseView view) {
      OnboardingManager.Instance.GoToNextStage();
    }
  }

}
