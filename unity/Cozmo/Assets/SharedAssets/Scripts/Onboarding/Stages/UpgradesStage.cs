using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class UpgradesStage : OnboardingBaseStage {

    [SerializeField]
    private GameObject _OverviewInstructions;

    private BaseView _UpgradeDetailsView = null;

    public override void Start() {
      base.Start();

      BaseView.BaseViewOpened += HandleViewOpened;
      GameEventManager.Instance.OnGameEvent += HandleGameEvent;
      UnlockablesManager.Instance.OnSparkComplete += HandleSparkComplete;
    }

    public override void OnDestroy() {
      base.OnDestroy();
      BaseView.BaseViewOpened -= HandleViewOpened;
      GameEventManager.Instance.OnGameEvent -= HandleGameEvent;
      UnlockablesManager.Instance.OnSparkComplete -= HandleSparkComplete;
    }

    private void HandleViewOpened(BaseView view) {
      if (view is CoreUpgradeDetailsDialog) {
        _UpgradeDetailsView = view;
        _OverviewInstructions.SetActive(false);
      }
    }

    public override void SkipPressed() {
      GoToNextState();
    }

    private void HandleGameEvent(GameEventWrapper gameEvent) {
      if (gameEvent.GameEventEnum == Anki.Cozmo.GameEvent.OnUnlockableEarned) {
        GoToNextState();
      }
    }
    private void HandleSparkComplete(CoreUpgradeDetailsDialog dialog) {
      GoToNextState();
    }

    private void GoToNextState() {
      // Since onboarding hides the close button really the only way to get out
      if (_UpgradeDetailsView) {
        _UpgradeDetailsView.CloseView();
      }
      OnboardingManager.Instance.GoToNextStage();
    }

  }

}
