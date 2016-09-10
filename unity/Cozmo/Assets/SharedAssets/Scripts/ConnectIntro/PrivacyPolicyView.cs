using UnityEngine;
using System.Collections;

public class PrivacyPolicyView : Cozmo.UI.BaseView {

  [SerializeField]
  AnkiInfiniteScrollView _AnkiInfiniteScrollView;

  private void Start() {
    _AnkiInfiniteScrollView.SetString(Localization.Get(LocalizationKeys.kPrivacyPolicyText));
  }

  protected override void CleanUp() {
    base.CleanUp();
  }
}
