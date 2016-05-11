using UnityEngine;
using UnityEngine.UI;
using Anki.UI;
using System.Collections;

public class ShowCozmoCubeSlide : MonoBehaviour {

  [SerializeField]
  private HorizontalOrVerticalLayoutGroup _CubeContainer;

  [SerializeField]
  private Image _CubePrefab;

  [SerializeField]
  private AnkiTextLabel _ShowCozmoCubesLabel;

  [SerializeField]
  private RectTransform _TransparentCubeContainer;

  private Image[] _CubeImages;

  private float _OutOfViewAlpha = 0.5f;

  public void Initialize(int numCubesToShow, Cozmo.CubePalette.CubeColor inViewColor) {
    CreateCubes(numCubesToShow, inViewColor.uiSprite);
    LightUpCubes(0);
    string locKeyToUse = (numCubesToShow > 1) ? LocalizationKeys.kMinigameLabelShowCubesPlural : LocalizationKeys.kMinigameLabelShowCubesSingular;
    _ShowCozmoCubesLabel.text = Localization.GetWithArgs(locKeyToUse,
      numCubesToShow);

    _TransparentCubeContainer.gameObject.SetActive(true);
  }

  public void LightUpCubes(int numberCubes) {
    for (int i = 0; i < _CubeImages.Length; i++) {
      _CubeImages[i].color = (i < numberCubes) ? Color.white : new Color(1, 1, 1, _OutOfViewAlpha);
    }
    _TransparentCubeContainer.gameObject.SetActive(numberCubes < _CubeImages.Length);
  }

  private void CreateCubes(int numCubesToShow, Sprite inViewSprite) {
    _CubeImages = new Image[numCubesToShow];
    for (int i = 0; i < _CubeImages.Length; i++) {
      _CubeImages[i] = UIManager.CreateUIElement(_CubePrefab, _CubeContainer.transform).GetComponent<Image>();
      _CubeImages[i].sprite = inViewSprite;
    }
  }
}
