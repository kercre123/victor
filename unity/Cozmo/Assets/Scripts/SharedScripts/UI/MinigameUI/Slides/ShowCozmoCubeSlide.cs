using UnityEngine;
using UnityEngine.UI;
using Anki.UI;
using System.Collections.Generic;
using Cozmo;
using Cozmo.UI;
using DG.Tweening;

public class ShowCozmoCubeSlide : MonoBehaviour {

  [SerializeField]
  private HorizontalOrVerticalLayoutGroup _CubeContainer;

  [SerializeField]
  private IconProxy _CubePrefab;

  [SerializeField]
  private CozmoText _ShowCozmoCubesLabel;

  [SerializeField]
  private RectTransform _TransparentCubeContainer;

  [SerializeField]
  private RectTransform _CozmoImageTransform;

  private IconProxy[] _CubeImages;

  private Color _InViewColor;
  private Color _OutViewColor;

  private Tweener _Tween;

  private bool _ShowTransparentCube;

  public bool ShowTransparentCube {
    get { return _ShowTransparentCube; }
    set {
      _ShowTransparentCube = value;
      if (!_ShowTransparentCube) {
        _TransparentCubeContainer.gameObject.SetActive(false);
      }
    }
  }

  public void Initialize(int numCubesToShow, CubePalette.CubeColor inViewColor, CubePalette.CubeColor outViewColor, bool showTransparentCube = true) {
    _InViewColor = inViewColor.uiLightColor;
    _OutViewColor = outViewColor.uiLightColor;
    _ShowTransparentCube = showTransparentCube;
    CreateCubes(numCubesToShow);
    LightUpCubes(0);
    string locKeyToUse = (numCubesToShow > 1) ? LocalizationKeys.kMinigameLabelShowCubesPlural : LocalizationKeys.kMinigameLabelShowCubesSingular;
    _ShowCozmoCubesLabel.text = Localization.GetWithArgs(locKeyToUse,
          numCubesToShow);

    _TransparentCubeContainer.gameObject.SetActive(_ShowTransparentCube);
    _Tween = null;
  }

  public void OnDestroy() {
    DestroyTween();
  }

  public void RotateCozmoImageTo(float degrees, float duration) {
    DestroyTween();
    if (_CozmoImageTransform != null) {
      _Tween = _CozmoImageTransform.DORotate(new Vector3(0, 0, degrees), duration);
    }
  }

  private void DestroyTween() {
    if (_Tween != null) {
      _Tween.Kill(false);
      _Tween = null;
    }
  }

  //It's expected the called has already done a loc with args replacement
  public void SetLabelText(string txt) {
    _ShowCozmoCubesLabel.text = txt;
  }
  public void SetCubeSpacing(float space) {
    VerticalLayoutGroup group = _CubeContainer.GetComponent<VerticalLayoutGroup>();
    if (group) {
      group.spacing = space;
    }
  }

  public void LightUpCubes(int numberCubes) {
    for (int i = 0; i < _CubeImages.Length; i++) {
      if (i < numberCubes) {
        _CubeImages[i].IconImage.color = _InViewColor;
      }
      else {
        _CubeImages[i].IconImage.color = _OutViewColor;
      }
      _CubeImages[i].IconImage.enabled = _CubeImages[i].IconImage.sprite != null;
    }
    if (_ShowTransparentCube) {
      _TransparentCubeContainer.gameObject.SetActive(numberCubes < _CubeImages.Length);
    }
  }

  public void LightUpCubes(List<int> cubeIndices) {
    for (int i = 0; i < _CubeImages.Length; i++) {
      if (cubeIndices.Contains(i)) {
        _CubeImages[i].IconImage.color = _InViewColor;
        _CubeImages[i].SetAlpha(1f);
      }
      else {
        _CubeImages[i].IconImage.color = _OutViewColor;
      }
      _CubeImages[i].IconImage.enabled = _CubeImages[i].IconImage.sprite != null;
    }
    if (_ShowTransparentCube) {
      _TransparentCubeContainer.gameObject.SetActive(cubeIndices.Count == 0);
    }
  }

  private void CreateCubes(int numCubesToShow) {
    _CubeImages = new IconProxy[numCubesToShow];
    for (int i = 0; i < _CubeImages.Length; i++) {
      _CubeImages[i] = UIManager.CreateUIElement(_CubePrefab, _CubeContainer.transform).GetComponent<Cozmo.UI.IconProxy>();
      _CubeImages[i].IconImage.color = _InViewColor;
    }
  }
}
