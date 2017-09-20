using Anki.Assets;
using Cozmo.UI;
using UnityEngine;

namespace Cozmo.WhatsNew {
  public class WhatsNewModal : BaseModal {
    public delegate void OptOutButtonPressedHandler();
    public event OptOutButtonPressedHandler OnOptOutButtonPressed;

    [SerializeField]
    private CozmoButton _OkayButton;

    [SerializeField]
    private CozmoButton _CodeLabButton;

    [SerializeField]
    private CozmoText _NewContentTitleLabel;

    [SerializeField]
    private CozmoText _NewContentDescLabel;

    [SerializeField]
    private CozmoImage _LeftGradientCapImage;

    [SerializeField]
    private CozmoImage _RightGradientCapImage;

    [SerializeField]
    private CozmoImage _GradientImage;
    private Material _GradientMaterial;
    private const string _LeftColorProperty = "_LeftColor";
    private const string _RightColorProperty = "_RightColor";

    [SerializeField]
    private CozmoImage _IconImage;
    private string _IconAssetBundle;
    private string _IconAssetName;

    public void InitializeWhatsNewModal(WhatsNewData data) {
      _NewContentTitleLabel.text = Localization.Get(data.TitleKey);
      _NewContentDescLabel.text = Localization.Get(data.DescriptionKey);

      _GradientMaterial = new Material(_GradientImage.material);
      _GradientImage.material = _GradientMaterial;

      SetColor(_LeftGradientCapImage, WhatsNewData.GetColor(data.LeftTintColor), 0.5f, _LeftColorProperty);
      SetColor(_RightGradientCapImage, WhatsNewData.GetColor(data.RightTintColor), 0.5f, _RightColorProperty);

      _IconAssetBundle = AssetBundleManager.Instance.RemapVariantName(data.IconAssetBundleName + ".uhd");
      _IconAssetName = data.IconAssetName;
      AssetBundleManager.Instance.LoadAssetBundleAsync(_IconAssetBundle, HandleIconAssetBundleLoaded);

      _OkayButton.Initialize(HandleOkayButtonClicked, "ignore_button", this.DASEventDialogName);
      _CodeLabButton.gameObject.SetActive(false);

      this.ModalClosedWithCloseButtonOrOutside += HandleModalClosedWithCloseButtonOrOutside;
    }

    private void OnDestroy() {
      this.ModalClosedWithCloseButtonOrOutside -= HandleModalClosedWithCloseButtonOrOutside;
    }

    private void Update() {
      if (IsAnimating) {
        // Match alpha of material with AlphaController value to avoid awkward animation
        float targetAlpha = _AlphaController.alpha * _AlphaController.alpha;
        SetColor(_LeftGradientCapImage, _LeftGradientCapImage.color, targetAlpha, _LeftColorProperty);
        SetColor(_RightGradientCapImage, _RightGradientCapImage.color, targetAlpha, _RightColorProperty);
      }
      else if (!_LeftGradientCapImage.color.a.IsNear(1f, float.Epsilon)) {
        SetColor(_LeftGradientCapImage, _LeftGradientCapImage.color, 1f, _LeftColorProperty);
        SetColor(_RightGradientCapImage, _RightGradientCapImage.color, 1f, _RightColorProperty);
      }
    }

    private void SetColor(CozmoImage targetImage, Color targetColor, float targetAlpha, string materialProperty) {
      targetColor.a = targetAlpha;
      targetImage.color = targetColor;
      _GradientImage.color = new Color(1, 1, 1, targetAlpha);
      _GradientMaterial.SetColor(materialProperty, targetColor);
    }

    private void HandleOkayButtonClicked() {
      RaiseOptOutPressed();
      CloseDialog();
    }

    private void HandleModalClosedWithCloseButtonOrOutside() {
      RaiseOptOutPressed();
    }

    private void RaiseOptOutPressed() {
      if (OnOptOutButtonPressed != null) {
        OnOptOutButtonPressed();
      }
    }

    private void HandleIconAssetBundleLoaded(bool wasLoaded) {
      if (wasLoaded) {
        AssetBundleManager.Instance.LoadAssetAsync<Sprite>(_IconAssetBundle,
                                                           _IconAssetName,
                                                           HandleIconLoaded);
      }
    }

    private void HandleIconLoaded(Sprite icon) {
      if (icon != null) {
        _IconImage.sprite = icon;
      }
    }
  }
}