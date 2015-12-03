using UnityEngine;
using System;
using System.Collections;
using UnityEngine.UI;
using Anki.UI;

namespace Cozmo {
  namespace UI {
    public class SimpleAlertView : BaseView {

      [SerializeField]
      private AnkiTextLabel _AlertTitleText;

      [SerializeField]
      private AnkiTextLabel _AlertMessageText;

      [SerializeField]
      private AnkiButton _PrimaryButton;

      [SerializeField]
      private AnkiButton _SecondaryButton;

      [SerializeField]
      private AnkiButton _CloseButton;

      private string _TitleKey;
      private string _DescriptionKey;

      public string TitleLocKey {
        get { return _AlertTitleText.text; }
        set {
          if (_TitleKey != value) {
            _TitleKey = value;
            _AlertTitleText.text = Localization.Get(value);
          }
        }
      }

      public string DescriptionLocKey {
        get { return _AlertMessageText.text; }
        set { 
          if (_DescriptionKey != value) {
            _DescriptionKey = value;
            _AlertMessageText.text = Localization.Get(value);
          } 
        }
      }

      private void Awake() {
        _CloseButton.onClick.AddListener(CloseView);

        // Hide all buttons
        _PrimaryButton.gameObject.SetActive(false);
        _CloseButton.gameObject.SetActive(false);
        _SecondaryButton.gameObject.SetActive(false);
      }

      protected override void CleanUp() {
        ResetButton(_PrimaryButton);
        ResetButton(_CloseButton);
        ResetButton(_SecondaryButton);
      }

      private void ResetButton(AnkiButton button) {
        if (button.isActiveAndEnabled) {
          button.onClick.RemoveAllListeners();
        }
      }

      public void SetCloseButtonEnabled(bool enabled) {
        _CloseButton.gameObject.SetActive(enabled);
      }

      public void SetPrimaryButton(string titleKey, Action action = null) {
        SetupButton(_PrimaryButton, Localization.Get(titleKey), action);
      }

      public void SetSecondaryButton(string titleKey, Action action = null) {
        SetupButton(_SecondaryButton, Localization.Get(titleKey), action);
      }

      private void SetupButton(AnkiButton button, String title, Action action) {
        button.gameObject.SetActive(true);
        button.Text = title.ToUpper();
        button.onClick.RemoveAllListeners();
        button.onClick.AddListener(() => {
          if (action != null) {
            action();
          }
          CloseView();
        });
      }
    }
  }
}
