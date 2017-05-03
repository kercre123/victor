using UnityEngine;
using UnityEngine.UI;
using Anki.UI;

namespace Cozmo.Challenge.DroneMode {
  public class DroneModeCameraReticle : MonoBehaviour {

    [SerializeField]
    private RectTransform _ReticleContainer;

    [SerializeField]
    private AnkiTextLegacy _ReticleLabel;

    [SerializeField]
    private Image _ReticleStrokeImage;

    [SerializeField]
    private Image _ReticleStrokeFill;

    public string ReticleLabel {
      get { return _ReticleLabel.text; }
      set { _ReticleLabel.text = value; }
    }

    public void SetImageSize(float width, float height) {
      _ReticleContainer.sizeDelta = new Vector2(width, height);
      _ReticleContainer.localPosition = new Vector3(width * 0.5f, -height * 0.5f, 0f);
    }

    public void ShowReticleLabelText(bool showText) {
      _ReticleLabel.gameObject.SetActive(showText);
    }

    public void SetColor(Color fillColor, Color strokeColor) {
      _ReticleStrokeFill.color = fillColor;
      _ReticleStrokeImage.color = strokeColor;
    }
  }
}