using UnityEngine;
using UnityEngine.UI;
using Anki.UI;
using System.Collections;

public class ShowCozmoCubeSlide : MonoBehaviour {

  [SerializeField]
  private VerticalLayoutGroup _CubeContainer;

  [SerializeField]
  private Image _CubePrefab;

  [SerializeField]
  private AnkiTextLabel _ShowCozmoCubesLabel;

  private Image[] _CubeImages;

  private Sprite _OutOfViewColor;
  private Sprite _InViewColor;

  public void Initialize(int numCubesToShow, Cozmo.CubePalette.CubeColor outOfViewColor, Cozmo.CubePalette.CubeColor inViewColor) {
    _InViewColor = inViewColor.uiSprite;
    _OutOfViewColor = outOfViewColor.uiSprite;

    CreateCubes(numCubesToShow);
    LightUpCubes(0);
    _ShowCozmoCubesLabel.text = Localization.GetWithArgs(LocalizationKeys.kMinigameLabelShowCubes,
      numCubesToShow);
  }

  public void LightUpCubes(int numberCubes) {
    for (int i = 0; i < _CubeImages.Length; i++) {
      _CubeImages[i].sprite = (i < numberCubes) ? _InViewColor : _OutOfViewColor;
    }
  }

  private void CreateCubes(int numCubesToShow) {
    _CubeImages = new Image[numCubesToShow];
    for (int i = 0; i < _CubeImages.Length; i++) {
      _CubeImages[i] = UIManager.CreateUIElement(_CubePrefab, _CubeContainer.transform).GetComponent<Image>();
    }
  }
}
