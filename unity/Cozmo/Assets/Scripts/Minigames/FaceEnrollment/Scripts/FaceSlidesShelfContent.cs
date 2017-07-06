using UnityEngine;
using Cozmo.UI;

namespace FaceEnrollment {
  public class FaceSlidesShelfContent : MonoBehaviour {

    public System.Action AddNewPersonPressed;
    public System.Action GameDoneButtonPressed;

    [SerializeField]
    private CozmoText _ShelfText;

    [SerializeField]
    private CozmoButton _AddNewPersonButton;

    [SerializeField]
    private CozmoButton _DoneButton;

    private void Awake() {
      _AddNewPersonButton.Initialize(() => {
        if (AddNewPersonPressed != null) {
          AddNewPersonPressed();
        }
      }, "add_new_person_button", "face_enrollment_shelf_content");

      _DoneButton.Initialize(() => {
        if (GameDoneButtonPressed != null) {
          GameDoneButtonPressed();
        }
      }, "game_done_button", "face_enrollment_shelf_content");
    }

    public void SetShelfTextKey(string shelfTextKey) {
      _ShelfText.text = Localization.Get(shelfTextKey);
    }

    public void ShowDoneButton(bool showDone) {
      _DoneButton.gameObject.SetActive(showDone);
    }

    public void ShowAddNewPersonButton(bool showAddPersonButton) {
      _AddNewPersonButton.gameObject.SetActive(showAddPersonButton);
    }

  }
}