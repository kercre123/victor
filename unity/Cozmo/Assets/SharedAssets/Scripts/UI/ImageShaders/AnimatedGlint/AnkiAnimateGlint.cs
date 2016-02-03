using UnityEngine;
using UnityEngine.UI;
using System.Collections;

namespace Cozmo.UI {
  public class AnkiAnimateGlint : MonoBehaviour {

    [SerializeField]
    private Image _MaskImage;

    private Material _GlintMaterial;

    private void Awake() {
      _MaskImage.enabled = true;
      _GlintMaterial = MaterialPool.GetMaterial(UIPrefabHolder.Instance.AnimatedGlintShader);
      _MaskImage.material = _GlintMaterial;
    }

    private void Start() {
    }

    public void EnableGlint(bool enable) {
      Color maskColor = _MaskImage.color;
      maskColor.a = enable ? 1 : 0;
      _MaskImage.color = maskColor;
    }

    private void OnDestroy() {
      MaterialPool.ReturnMaterial(_GlintMaterial);
    }
  }
}