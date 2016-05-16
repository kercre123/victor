using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using Anki.UI;
using System;

public class ArtistCozmoImageMenu : MonoBehaviour {

  [SerializeField]
  private RawImage _RawImage;

  [SerializeField]
  private Cozmo.UI.CozmoButton _RetryButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _SaveButton;

  public event Action OnSaveClick;

  public event Action OnRetryClick;

  private void Awake() {
    string viewControllerName = "artist_cozmo_image_menu";
    _SaveButton.Initialize(HandleSaveClick, "save_filtered_photo_button", viewControllerName);
    _RetryButton.Initialize(HandleRetryClick, "retry_taking_photo_button", viewControllerName);
  }

  private void HandleSaveClick() {
    if (OnSaveClick != null) {
      OnSaveClick();
    }
  }

  private void HandleRetryClick() {
    if (OnRetryClick != null) {
      OnRetryClick();
    }
  }


  public void Initialize(Texture texture) {
    _RawImage.texture = texture;
  }


}
