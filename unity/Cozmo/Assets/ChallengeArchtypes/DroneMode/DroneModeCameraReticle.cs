using UnityEngine;
using UnityEngine.UI;
using Anki.UI;

namespace Cozmo.Minigame.DroneMode {
  public class DroneModeCameraReticle : MonoBehaviour {

    [SerializeField]
    private RectTransform _ReticleContainer;

    [SerializeField]
    private AnkiTextLabel _ReticleLabel;

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
  }
}