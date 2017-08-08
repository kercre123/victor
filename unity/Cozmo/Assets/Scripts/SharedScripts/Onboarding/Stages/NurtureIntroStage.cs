using Cozmo.UI;
using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo.ExternalInterface;

namespace Onboarding {

  // This screen is either initial
  public class NurtureIntroStage : ShowContinueStage {
    [SerializeField]
    private CozmoText _TextFieldInstance;

    [SerializeField]
    private RuntimeAnimatorController _NurtureIntroAnimController;
    private Animator _AnimatorInst;

    private const string _kMechanimEventShowOnboardingText = "ShowOnboardingText";
    private static readonly int _kMechanimStateDefault = Animator.StringToHash("NeedsDefault");

    protected override void Awake() {
      base.Awake();

      if (OnboardingManager.Instance.IsReturningUser()) {
        _TextFieldInstance.text = Localization.Get(LocalizationKeys.kOnboardingNeedsIntroReturning);
      }
      _TextFieldInstance.gameObject.SetActive(false);
      _ContinueButtonInstance.gameObject.SetActive(false);
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Meter_Appear);

      // set up for the upcoming repair bit
      ForceSetDamagedParts msg = new ForceSetDamagedParts();
      msg.partIsDamaged = new bool[] { false, true, false };
      RobotEngineManager.Instance.Message.ForceSetDamagedParts = msg;
      RobotEngineManager.Instance.SendMessage();

      OnboardingManager.Instance.OnOnboardingAnimEvent += HandleOnboardingAnimEvent;

      Cozmo.Needs.UI.NeedsHubView needsHubView = OnboardingManager.Instance.NeedsView;
      _AnimatorInst = needsHubView.gameObject.GetComponent<Animator>();
      if (_AnimatorInst == null) {
        _AnimatorInst = needsHubView.gameObject.AddComponent<Animator>();
        _AnimatorInst.runtimeAnimatorController = _NurtureIntroAnimController;
      }
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Nurture_Background_Appear_Onboarding);

      DimNav(false);
    }

    public override void Start() {
      base.Start();
      // Usually not having completed onboarding prevents freeplay.
      // this stage is the exception, and we still dont want freeplay music, etc.
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        if (robot.Faces.Count > 0) {
          robot.TurnTowardsLastFacePose(Mathf.PI);
        }
        else {
          robot.ActivateHighLevelActivity(Anki.Cozmo.HighLevelActivity.Selection);
          robot.ExecuteBehaviorByID(Anki.Cozmo.BehaviorID.FindFaces_socialize);
        }
      }
    }

    protected override void HandleContinueClicked() {
      base.HandleContinueClicked();
    }
    public override void SkipPressed() {
      if (_ContinueButtonInstance.isActiveAndEnabled) {
        base.SkipPressed();
      }
      else {
        if (_AnimatorInst != null) {
          _AnimatorInst.Play(_kMechanimStateDefault);
        }
        HandleOnboardingAnimEvent(_kMechanimEventShowOnboardingText);
      }
    }

    private void HandleOnboardingAnimEvent(string param) {
      if (param == _kMechanimEventShowOnboardingText) {
        // make sure onboarding is in front after attaching since it started on awake.
        transform.SetAsLastSibling();
        _TextFieldInstance.gameObject.SetActive(true);
        _ContinueButtonInstance.gameObject.SetActive(true);
        DimNav(true);
      }
    }

    private void DimNav(bool wantsNormalDim) {
      // This happens after the animation is completed.
      Cozmo.Needs.UI.NeedsHubView needsHubView = OnboardingManager.Instance.NeedsView;
      needsHubView.OnboardingBlockoutImage.gameObject.SetActive(wantsNormalDim);
      needsHubView.NavBackgroundImage.LinkedComponentId = wantsNormalDim ?
                                                          ThemeKeys.Cozmo.Image.kNavHubContainerBGDimmed :
                                                          ThemeKeys.Cozmo.Image.kNavHubButtonNormal;
      needsHubView.NavBackgroundImage.UpdateSkinnableElements();
      if (wantsNormalDim) {
        DimButton(needsHubView.DiscoverButton);
        DimButton(needsHubView.RepairButton);
        DimButton(needsHubView.FeedButton);
        DimButton(needsHubView.PlayButton);
      }
    }

    private void DimButton(CozmoButton button) {
      CozmoImage bg = button.GetComponentInChildren<CozmoImage>();
      if (bg != null) {
        bg.LinkedComponentId = ThemeKeys.Cozmo.Image.kNavHubButtonDimmed;
        button.interactable = false;
        bg.UpdateSkinnableElements();
      }
    }

    public override void OnDestroy() {
      base.OnDestroy();
      OnboardingManager.Instance.OnOnboardingAnimEvent -= HandleOnboardingAnimEvent;
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.ExecuteBehaviorByID(Anki.Cozmo.BehaviorID.Wait);
      }
      if (_AnimatorInst != null) {
        // Possible if debug skipping around.
        if (_AnimatorInst.GetCurrentAnimatorStateInfo(0).shortNameHash != _kMechanimStateDefault) {
          _AnimatorInst.Play(_kMechanimStateDefault);
        }
      }
    }

  }

}
