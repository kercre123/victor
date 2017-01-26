using UnityEngine;

namespace FaceEnrollment {
  public class FaceEnrollmentInstructionsModal : Cozmo.UI.BaseModal {

    [SerializeField]
    private Anki.UI.AnkiTextLabel _NameForFaceLabel;

    public void SetNameForFace(string nameForFace) {
      _NameForFaceLabel.text = nameForFace + ":";
    }

    protected override void CleanUp() {
      base.CleanUp();
    }
  }
}