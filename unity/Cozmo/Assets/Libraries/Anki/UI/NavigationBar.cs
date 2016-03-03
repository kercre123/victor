using UnityEngine;
using UnityEngine.UI;
using System.Collections;


namespace Anki {
  namespace UI {
    public class NavigationBar : View {
      [SerializeField]
      private Text
        _TitleText;

      public AnkiButton BackButton;

      public virtual string Title {
        set { if (_TitleText !=null) { _TitleText.text = value;} }
        get { if (_TitleText !=null) { return _TitleText.text; } return null; }
      }

      public bool BackButtonHidden {
        get {
          if (BackButton == null) {
            return true;
          }
          else {
            return !BackButton.IsActive();
          }
        }
        set {
          if (BackButton != null) {
            BackButton.gameObject.SetActive(!value);
          }
        }
      }

    }
  }
}
