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
      _GlintMaterial = MaterialPool.GetMaterial(UIPrefabHolder.Instance.AnimatedGlintShader, _MaskImage.defaultMaterial.renderQueue);
      _MaskImage.material = _GlintMaterial;
    }

    private void Start() {
    }

    public void EnableGlint(bool enable) {
      string debugMaskColor = _MaskImage != null ? _MaskImage.color.ToString() : "(Image was null)";
      DAS.Info(this, string.Format("AnkiAnimateGlint.EnableGlint START! enable={0} maskColor={1}", enable, debugMaskColor));
      
      Color maskColor = _MaskImage.color;
      maskColor.a = enable ? 1 : 0;
      _MaskImage.color = maskColor;

      debugMaskColor = _MaskImage != null ? _MaskImage.color.ToString() : "(Image was null)";
      DAS.Info(this, string.Format("AnkiAnimateGlint.EnableGlint END! enable={0} maskColor={1}", enable, debugMaskColor));
    }

    private void OnDestroy() {
      DAS.Info(this, "AnkiAnimateGlint.OnDestroy START! Returning material: " + this.gameObject.name);
      MaterialPool.ReturnMaterial(_GlintMaterial);
      DAS.Info(this, "AnkiAnimateGlint.OnDestroy END! Finished returning material: " + this.gameObject.name);
    }
  }
}