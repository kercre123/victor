using Cozmo.UI;
using UnityEngine;

namespace Onboarding {

  public class RewardsIntroStage : ShowContinueStage {

    [SerializeField]
    private GameObject _ShowAfterWait;

    [SerializeField]
    private float _WaitTimeSec = 1.0f;

    protected override void Awake() {
      base.Awake();
      _ShowAfterWait.SetActive(false);
      Invoke("HandleWaitToShowComplete", _WaitTimeSec);

      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.ActivateHighLevelActivity(Anki.Cozmo.HighLevelActivity.Selection);
        robot.ExecuteBehaviorByID(Anki.Cozmo.BehaviorID.FindFaces_socialize);
      }
    }
    public override void Start() {
      base.Start();
    }
    public override void OnDestroy() {
      base.OnDestroy();
      CancelInvoke("HandleWaitToShowComplete");
    }

    private void HandleWaitToShowComplete() {
      _ShowAfterWait.SetActive(true);

      Cozmo.Needs.UI.NeedsHubView view = OnboardingManager.Instance.NeedsView;
      view.OnboardingBlockoutImage.gameObject.SetActive(true);
      UpdateButtonState(view.DiscoverButton);
      UpdateButtonState(view.RepairButton);
      UpdateButtonState(view.FeedButton);
      UpdateButtonState(view.PlayButton);
      view.NavBackgroundImage.LinkedComponentId = ThemeKeys.Cozmo.Image.kNavHubContainerBGDimmed;
      view.NavBackgroundImage.UpdateSkinnableElements();
    }
    private void UpdateButtonState(CozmoButton button) {
      button.Interactable = false;
      CozmoImage bg = button.GetComponentInChildren<CozmoImage>();
      if (bg != null) {
        bg.LinkedComponentId = ThemeKeys.Cozmo.Image.kNavHubButtonDimmed;
        bg.UpdateSkinnableElements();
      }
    }
  }

}
