using UnityEngine;
using System.Collections;

public class FaceEnrollmentInstructionsView : Cozmo.UI.BaseView {

  [SerializeField]
  private Anki.UI.AnkiTextLabel _FaceNameLabel;

  public void SetFaceName(string faceName) {
    _FaceNameLabel.text = faceName + ":";
  }

  protected override void CleanUp() {
    base.CleanUp();
  }
}
