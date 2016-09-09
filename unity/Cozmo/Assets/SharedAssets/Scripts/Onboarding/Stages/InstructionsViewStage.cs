using UnityEngine;

namespace Onboarding {

  public class InstructionsViewStage : ShowContinueStage {

    [SerializeField]
    private Transform _RedTableOutline;

    [SerializeField]
    private Anki.UI.AnkiTextLabel _TextfieldHeader;

    [SerializeField]
    private Anki.UI.AnkiTextLabel _TextfieldFooter;

    private int _Step = 0;

    public override void Start() {
      base.Start();
      _RedTableOutline.gameObject.SetActive(false);
    }

    protected override void HandleContinueClicked() {
      _Step++;
      if (_Step >= 2) {
        base.HandleContinueClicked();
      }
      else {
        _RedTableOutline.gameObject.SetActive(true);
        _TextfieldHeader.text = Localization.Get(LocalizationKeys.kOnboardingGreatPlaySpace2Header);
        _TextfieldFooter.text = Localization.Get(LocalizationKeys.kOnboardingGreatPlaySpace2Footer);
      }
    }

  }

}
