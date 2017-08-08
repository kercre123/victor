using Cozmo.UI;
using UnityEngine;
using System.Collections.Generic;

namespace Onboarding {

  public class TokensStage : ShowContinueStage {

    [SerializeField]
    private GameObject _ShowAfterWait;

    [SerializeField]
    private float _WaitTimeSec = 1.0f;

    protected override void Awake() {
      base.Awake();
      _ShowAfterWait.SetActive(false);
      Invoke("HandleWaitToShowComplete", _WaitTimeSec);
      // this is the one time we want the highlight to be behind the stars
      Cozmo.Needs.UI.NeedsHubView view = OnboardingManager.Instance.NeedsView;
      view.OnboardingBlockoutImage.transform.SetSiblingIndex(view.OnboardingBlockoutImage.transform.GetSiblingIndex() - 1);

    }
    public override void OnDestroy() {
      base.OnDestroy();
      CancelInvoke("HandleWaitToShowComplete");
    }

    private void HandleWaitToShowComplete() {
      _ShowAfterWait.SetActive(true);

      Cozmo.Needs.UI.NeedsHubView view = OnboardingManager.Instance.NeedsView;
      view.NavBackgroundImage.LinkedComponentId = ThemeKeys.Cozmo.Image.kNavHubContainerBGDimmed;
      view.NavBackgroundImage.UpdateSkinnableElements();
      view.OnboardingBlockoutImage.gameObject.SetActive(true);
      // Usually this is done in onboarding manager, but since we want them to see the animation, undim
      if (view.MetersWidget != null) {
        List<Anki.Cozmo.NeedId> dimmedMeters = new List<Anki.Cozmo.NeedId>() {
                Anki.Cozmo.NeedId.Energy, Anki.Cozmo.NeedId.Play , Anki.Cozmo.NeedId.Repair };
        view.MetersWidget.DimNeedMeters(dimmedMeters);
      }
      UpdateButtonState(view.DiscoverButton);
      UpdateButtonState(view.RepairButton);
      UpdateButtonState(view.FeedButton);
      UpdateButtonState(view.PlayButton);
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
