using Anki.Cozmo.ExternalInterface;
using Anki.UI;
using Cozmo.UI;
using UnityEngine;

namespace Cozmo.Settings {
  public class SettingsAcknowledgementsPanel : MonoBehaviour {
    [SerializeField]
    private CozmoButton _AcknowledgementsLinkButton;

    private ScrollingTextModal _AcknowledgementsModalInstance;

    [SerializeField]
    private string _AcknowledgementsTextFileName;

    [SerializeField]
    private CozmoButton _PrivacyPolicyLinkButton;

    private ScrollingTextModal _PrivacyPolicyModalInstance;

    [SerializeField]
    private string _PrivacyPolicyTextFileName;

    [SerializeField]
    private CozmoButton _TermsOfUseButton;

    private ScrollingTextModal _TermsOfUseModalInstance;

    [SerializeField]
    private string _TermsOfUseTextFileName;

    private void Awake() {
      string dasEventViewName = "settings_acknowledgements_panel";
      _AcknowledgementsLinkButton.Initialize(HandleAcknowledgementsLinkButtonTapped, "acknowledgements_link", dasEventViewName);
      _PrivacyPolicyLinkButton.Initialize(HandlePrivacyPolicyLinkButtonTapped, "privacyPolicy_link", dasEventViewName);
      _TermsOfUseButton.Initialize(HandleTermsOfUseLinkButtonTapped, "termsOfUse_link", dasEventViewName);
    }

    private void OnDestroy() {
      if (_AcknowledgementsModalInstance != null) {
        _AcknowledgementsModalInstance.CloseDialogImmediately();
      }
      if (_PrivacyPolicyModalInstance != null) {
        _PrivacyPolicyModalInstance.CloseDialogImmediately();
      }
      if (_TermsOfUseModalInstance != null) {
        _TermsOfUseModalInstance.CloseDialogImmediately();
      }
    }


    private void HandleAcknowledgementsLinkButtonTapped() {
      if (_AcknowledgementsModalInstance == null) {
        System.Action<BaseModal> acknowledgementsModalCreated = (modal) => {
          _AcknowledgementsModalInstance = (ScrollingTextModal)modal;
          _AcknowledgementsModalInstance.DASEventDialogName = "acknowledgements_view";
          // now in unity resources folder, strip extension for Resources not our loc assets
          _AcknowledgementsTextFileName = _AcknowledgementsTextFileName.Replace(".txt", "");
          TextAsset acknowledgementsText = Resources.Load<TextAsset>(_AcknowledgementsTextFileName);
          _AcknowledgementsModalInstance.Initialize(Localization.Get(LocalizationKeys.kSettingsVersionPanelAcknowledgementsModalTitle),
                                                    acknowledgementsText.text);
        };

        UIManager.OpenModal(AlertModalLoader.Instance.ScrollingTextModalPrefab,
                            SettingsModal.SettingsSubModalPriorityData(), acknowledgementsModalCreated);
      }
    }

    private void HandlePrivacyPolicyLinkButtonTapped() {
      if (_PrivacyPolicyModalInstance == null) {
        System.Action<BaseModal> privacyPolicyCreated = (modal) => {
          _PrivacyPolicyModalInstance = (ScrollingTextModal)modal;
          _PrivacyPolicyModalInstance.DASEventDialogName = "privacyPolicy_view";
          _PrivacyPolicyModalInstance.Initialize(Localization.Get(LocalizationKeys.kPrivacyPolicyTitle),
                                                 Localization.ReadLocalizedTextFromFile(_PrivacyPolicyTextFileName));
        };

        UIManager.OpenModal(AlertModalLoader.Instance.ScrollingTextModalPrefab,
                            SettingsModal.SettingsSubModalPriorityData(), privacyPolicyCreated);
      }
    }

    private void HandleTermsOfUseLinkButtonTapped() {
      if (_TermsOfUseModalInstance == null) {
        System.Action<BaseModal> termsOfUseCreated = (modal) => {
          _TermsOfUseModalInstance = (ScrollingTextModal)modal;
          _TermsOfUseModalInstance.DASEventDialogName = "termsOfUse_view";
          _TermsOfUseModalInstance.Initialize(Localization.Get(LocalizationKeys.kLabelTermsOfUse),
                                              Localization.ReadLocalizedTextFromFile(_TermsOfUseTextFileName));
        };
        UIManager.OpenModal(AlertModalLoader.Instance.ScrollingTextModalPrefab,
                            SettingsModal.SettingsSubModalPriorityData(), termsOfUseCreated);
      }
    }
  }
}
