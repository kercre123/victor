using UnityEngine;
using System.Collections;

public class FaceEnrollmentShelfContent : MonoBehaviour {

  public System.Action GameDoneButtonPressed;
  public System.Action HowToPlayButtonPressed;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _ShelfText;

  [SerializeField]
  private Cozmo.UI.CozmoButton _DoneButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _HowToPlayButton;

  private void Awake() {
    _DoneButton.Initialize(() => {
      if (GameDoneButtonPressed != null) {
        GameDoneButtonPressed();
      }
    }, "game_done_button", "face_enrollment_shelf_content");
    _HowToPlayButton.Initialize(() => {
      if (HowToPlayButtonPressed != null) {
        HowToPlayButtonPressed();
      }
    }, "how_to_play_button", "face_enrollment_shelf_content");
  }

  public void SetShelfTextKey(string shelfTextKey) {
    _ShelfText.text = Localization.Get(shelfTextKey);
  }

  public void ShowDoneButton(bool showDone) {
    _DoneButton.gameObject.SetActive(showDone);
  }

}
