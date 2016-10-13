using Anki.Cozmo.ExternalInterface;
using Anki.UI;
using Cozmo.UI;
using UnityEngine;

namespace Cozmo.Settings {
  public class SettingsAcknowledgementsPanel : MonoBehaviour {
    [SerializeField]
    private CozmoButton _AcknowledgementsLinkButton;

    private ScrollingTextView _AcknowledgementsDialogInstance;

    [SerializeField]
    private string _AcknowledgementsTextFileName;

    [SerializeField]
    private CozmoButton _PrivacyPolicyLinkButton;

    private ScrollingTextView _PrivacyPolicyDialogInstance;

    [SerializeField]
    private string _PrivacyPolicyTextFileName;

    [SerializeField]
    private CozmoButton _TermsOfUseButton;

    private ScrollingTextView _TermsOfUseDialogInstance;

    [SerializeField]
    private string _TermsOfUseTextFileName;

    private void Awake() {
      string dasEventViewName = "settings_acknowledgements_panel";
      _AcknowledgementsLinkButton.Initialize(HandleAcknowledgementsLinkButtonTapped, "acknowledgements_link", dasEventViewName);
      _PrivacyPolicyLinkButton.Initialize(HandlePrivacyPolicyLinkButtonTapped, "privacyPolicy_link", dasEventViewName);
      _TermsOfUseButton.Initialize(HandleTermsOfUseLinkButtonTapped, "termsOfUse_link", dasEventViewName);
    }

    private void OnDestroy() {
      if (_AcknowledgementsDialogInstance != null) {
        _AcknowledgementsDialogInstance.CloseViewImmediately();
      }
      if (_PrivacyPolicyDialogInstance != null) {
        _PrivacyPolicyDialogInstance.CloseViewImmediately();
      }
      if (_TermsOfUseDialogInstance != null) {
        _TermsOfUseDialogInstance.CloseViewImmediately();
      }
    }


    private void HandleAcknowledgementsLinkButtonTapped() {
      if (_AcknowledgementsDialogInstance == null) {
        _AcknowledgementsDialogInstance = UIManager.OpenView(AlertViewLoader.Instance.ScrollingTextViewPrefab,
                                                             (ScrollingTextView view) => { view.DASEventViewName = "acknowledgements_view"; });
        _AcknowledgementsDialogInstance.Initialize(Localization.Get(LocalizationKeys.kSettingsVersionPanelAcknowledgementsModalTitle),
                                                   Localization.ReadLocalizedTextFromFile(_AcknowledgementsTextFileName));
      }
    }

    private void HandlePrivacyPolicyLinkButtonTapped() {
      if (_PrivacyPolicyDialogInstance == null) {
        _PrivacyPolicyDialogInstance = UIManager.OpenView(AlertViewLoader.Instance.ScrollingTextViewPrefab,
                                                           (ScrollingTextView view) => { view.DASEventViewName = "privacyPolicy_view"; });
        _PrivacyPolicyDialogInstance.Initialize(Localization.Get(LocalizationKeys.kPrivacyPolicyTitle),
                                                Localization.ReadLocalizedTextFromFile(_PrivacyPolicyTextFileName));
      }
    }

    private void HandleTermsOfUseLinkButtonTapped() {
      if (_TermsOfUseDialogInstance == null) {
        _TermsOfUseDialogInstance = UIManager.OpenView(AlertViewLoader.Instance.ScrollingTextViewPrefab,
                                                       (ScrollingTextView view) => { view.DASEventViewName = "termsOfUse_view"; });
        _TermsOfUseDialogInstance.Initialize(Localization.Get(LocalizationKeys.kLabelTermsOfUse),
                                             Localization.ReadLocalizedTextFromFile(_TermsOfUseTextFileName));
      }
    }
  }
}
