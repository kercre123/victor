using Cozmo.UI;

namespace Onboarding {

  public class OnboardingWaitDialogCloseStage : OnboardingBaseStage {
    public override void Start() {
      base.Start();
      BaseModal.BaseModalClosed += HandleModalClosed;
    }

    public override void OnDestroy() {
      base.OnDestroy();
      BaseModal.BaseModalClosed -= HandleModalClosed;

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Freeplay);
      Anki.Cozmo.Audio.GameAudioClient.SetFreeplayMoodMusicState(Anki.AudioMetaData.SwitchState.Freeplay_Mood.Neutral);
    }

    private void HandleModalClosed(BaseModal modal) {
      if (modal is Cozmo.Repair.UI.NeedsRepairModal) {
        OnboardingManager.Instance.GoToNextStage();
      }
      else if (modal is Cozmo.Energy.UI.NeedsEnergyModal) {
        OnboardingManager.Instance.GoToNextStage();
      }
    }

#if ANKI_DEV_CHEATS
    public override void SkipPressed() {
      // Debug moving around, since continue    
      BaseModal modal = FindObjectOfType<Cozmo.Repair.UI.NeedsRepairModal>();
      if (modal != null) {
        modal.CloseDialog();
        return;
      }
      modal = FindObjectOfType<Cozmo.Energy.UI.NeedsEnergyModal>();
      if (modal != null) {
        modal.CloseDialog();
        return;
      }
      // The modal closing will cause the GoToNextStage call if we found a modal
      base.SkipPressed();
    }
#endif
  }

}
