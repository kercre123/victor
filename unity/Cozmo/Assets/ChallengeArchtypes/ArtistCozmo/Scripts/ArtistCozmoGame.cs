using UnityEngine;
using System.Collections.Generic;
using UnityEngine.UI;
using ArtistCozmo;
using System;

public class ArtistCozmoGame : GameBase {

  public enum ArtStyle {
    Painting,
    Sketch,

    Count
  }

  public Gradient ColorGradient { get; private set; }
  public int ColorCount { get; private set; }
  public ArtStyle Style { get; private set; }

  [SerializeField]
  private GameObject _ImagePrefab;

  [SerializeField]
  private string _SaveFolder;

  public string SaveFolder { get { return _SaveFolder; } }

  private ImageReceiver _ImageReceiver;

  public ImageReceiver ImageReceiver { get { return _ImageReceiver; } }

  private ArtistCozmoGameConfig _Config;

  protected override void Initialize(MinigameConfigBase minigameConfigData) {

    _Config = minigameConfigData as ArtistCozmoGameConfig;

    _ImageReceiver = new ImageReceiver("ArtistCozmo");
    _StateMachine.SetNextState(new ArtistCozmoFindFaceState());
  }

  protected override void CleanUpOnDestroy() {
    _ImageReceiver.DestroyTexture();
    _ImageReceiver.Dispose();
    _ImageReceiver = null;
  }

  public void DisplayImage(Texture2D image, Action onSave, Action onRetry) {
    var imageMenu = SharedMinigameView.ShowNarrowGameStateSlide(_ImagePrefab, "ImageMenu").GetComponent<ArtistCozmoImageMenu>();
    imageMenu.Initialize(image);
    imageMenu.OnSaveClick += onSave;
    imageMenu.OnRetryClick += onRetry;
  }

  public void RandomizeFilter() {
    if (_Config != null) {
      if (_Config.ColorGradients.Length > 0) {
        ColorGradient = _Config.ColorGradients[UnityEngine.Random.Range(0, _Config.ColorGradients.Length)];
      }
      ColorCount = 1 << UnityEngine.Random.Range(_Config.MinColorBits, _Config.MaxColorBits + 1);
    }
    else {
      ColorCount = 8;
    }
    Style = (ArtStyle)UnityEngine.Random.Range(0, (int)ArtStyle.Count);
  }


}
