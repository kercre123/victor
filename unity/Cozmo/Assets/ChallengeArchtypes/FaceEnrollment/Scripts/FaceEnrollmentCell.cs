using UnityEngine;
using System.Collections;

public class FaceEnrollmentCell : MonoBehaviour {

  public System.Action<int, string> OnEditNameRequested;
  public System.Action<int> OnDeleteNameRequested;

  private string _FaceName;
  private int _FaceID;

  [SerializeField]
  Anki.UI.AnkiTextLabel _NameLabel;

  public void Initialize(int faceID, string faceName) {
    _FaceName = faceName;
    _FaceID = faceID;
    _NameLabel.text = _FaceName;
  }

  private void HandleEditNameClicked() {
    if (OnEditNameRequested != null) {
      OnEditNameRequested(_FaceID, _FaceName);
    }
  }
}
