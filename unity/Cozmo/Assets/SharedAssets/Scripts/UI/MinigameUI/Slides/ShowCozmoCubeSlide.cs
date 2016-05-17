using UnityEngine;
using UnityEngine.UI;
using Anki.UI;
using System.Collections;
using Cozmo;
using Cozmo.UI;

public class ShowCozmoCubeSlide : MonoBehaviour {

  [SerializeField]
  private HorizontalOrVerticalLayoutGroup _CubeContainer;

  [SerializeField]
  private IconProxy _CubePrefab;

  [SerializeField]
  private AnkiTextLabel _ShowCozmoCubesLabel;

  [SerializeField]
  private RectTransform _TransparentCubeContainer;

  private IconProxy[] _CubeImages;

  private float _OutOfViewAlpha = 0.5f;

  private CubePalette.CubeColor _InViewColor;
  private CubePalette.CubeColor _OutViewColor;

  public void Initialize(int numCubesToShow, CubePalette.CubeColor inViewColor, CubePalette.CubeColor outViewColor) {
    _InViewColor = inViewColor;
    _OutViewColor = outViewColor;
    CreateCubes(numCubesToShow, inViewColor.uiSprite);
    LightUpCubes(0);
    string locKeyToUse = (numCubesToShow > 1) ? LocalizationKeys.kMinigameLabelShowCubesPlural : LocalizationKeys.kMinigameLabelShowCubesSingular;
    _ShowCozmoCubesLabel.text = Localization.GetWithArgs(locKeyToUse,
      numCubesToShow);

    _TransparentCubeContainer.gameObject.SetActive(true);
  }

  public void LightUpCubes(int numberCubes) {
    for (int i = 0; i < _CubeImages.Length; i++) {
      if (i < numberCubes) {
        _CubeImages[i].SetIcon(_InViewColor.uiSprite);
        _CubeImages[i].SetAlpha(1f);
      }
      else {
        _CubeImages[i].SetIcon(_OutViewColor.uiSprite);
        _CubeImages[i].SetAlpha(_OutOfViewAlpha);
      }
    }
    _TransparentCubeContainer.gameObject.SetActive(numberCubes < _CubeImages.Length);
  }

  private void CreateCubes(int numCubesToShow, Sprite inViewSprite) {
    _CubeImages = new IconProxy[numCubesToShow];
    for (int i = 0; i < _CubeImages.Length; i++) {
      _CubeImages[i] = UIManager.CreateUIElement(_CubePrefab, _CubeContainer.transform).GetComponent<Cozmo.UI.IconProxy>();
      _CubeImages[i].SetIcon(inViewSprite);
    }
  }
}
