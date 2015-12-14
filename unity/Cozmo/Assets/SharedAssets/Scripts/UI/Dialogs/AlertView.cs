using UnityEngine;
using System;
using System.Collections;
using UnityEngine.UI;
using Anki.UI;
using DG.Tweening;

namespace Cozmo {
  namespace UI {
    public class AlertView : BaseView {

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
        get { return _AlertTitleText != null ? _AlertTitleText.text : null; }
        set {
          if (_TitleKey != value && _AlertTitleText != null) {
            _TitleKey = value;
            _AlertTitleText.text = Localization.Get(value);
          }
        }
      }

      public string DescriptionLocKey {
        get { return _AlertMessageText != null ? _AlertMessageText.text : null; }
        set { 
          if (_DescriptionKey != value && _AlertMessageText != null) {
            _DescriptionKey = value;
            _AlertMessageText.text = Localization.Get(value);
          } 
        }
      }

      private void Awake() {
        if (_CloseButton != null) {
          _CloseButton.onClick.AddListener(CloseView);
          _CloseButton.gameObject.SetActive(false);
        }

        // Hide all buttons
        if (_PrimaryButton != null) {
          _PrimaryButton.gameObject.SetActive(false);
        }

        if (_SecondaryButton != null) {
          _SecondaryButton.gameObject.SetActive(false);
        }
      }

      protected override void CleanUp() {
        ResetButton(_PrimaryButton);
        ResetButton(_CloseButton);
        ResetButton(_SecondaryButton);
      }

      private void ResetButton(AnkiButton button) {
        if (button != null && button.isActiveAndEnabled) {
          button.onClick.RemoveAllListeners();
        }
      }

      public void SetCloseButtonEnabled(bool enabled) {
        if (_CloseButton != null) {
          _CloseButton.gameObject.SetActive(enabled);
        }
        else {
          DAS.Warn(this, "Tried to set up a button that doesn't exist in this AlertView! " + gameObject.name);
        }
      }

      public void SetPrimaryButton(string titleKey, Action action = null) {
        SetupButton(_PrimaryButton, Localization.Get(titleKey), action);
      }

      public void SetSecondaryButton(string titleKey, Action action = null) {
        SetupButton(_SecondaryButton, Localization.Get(titleKey), action);
      }

      private void SetupButton(AnkiButton button, String title, Action action) {
        if (button != null) {
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
        else {
          DAS.Warn(this, "Tried to set up a button that doesn't exist in this AlertView! " + gameObject.name);
        }
      }

      protected override void ConstructOpenAnimation(Sequence openAnimation) {
      }

      protected override void ConstructCloseAnimation(Sequence closeAnimation) {
      }
    }
  }
}
