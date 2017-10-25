using UnityEngine;
using UnityEngine.UI;
using System.Collections;

namespace Cozmo.UI {
  public class AnkiAnimateGlint : MonoBehaviour {

    [SerializeField]
    private Image _MaskImage;

    [SerializeField, Range(0.01f, 10.0f)]
    private float _GlintRateModifier = 8.0f;

    [SerializeField, Range(0.01f, 20.0f)]
    private float _GlintSpeedModifier = 5.0f;

    private Material _GlintMaterial;

    private void Awake() {
      if (_MaskImage != null) {
        _MaskImage.enabled = true;
#if UNITY_EDITOR
        // There is a bug with Unity Editor and asset bundles where it can't find the shader just by instance, so here
        // we have to explicitly call Shader.Find(). This is not the most performant way to do things so we should only
        // do it for the editor.
        _GlintMaterial = MaterialPool.GetMaterial(Shader.Find(ShaderHolder.Instance.AnimatedGlintShader.name), _MaskImage.defaultMaterial.renderQueue);
#else
        _GlintMaterial = MaterialPool.GetMaterial(ShaderHolder.Instance.AnimatedGlintShader, _MaskImage.defaultMaterial.renderQueue);
#endif

        // Property names defined in AnimatedGlintShader 
        _GlintMaterial.SetFloat("_GlintPeriod", _GlintRateModifier);
        _GlintMaterial.SetFloat("_GlintSpeed", _GlintSpeedModifier);

        _MaskImage.material = _GlintMaterial;
      }
    }

    public void EnableGlint(bool enable) {
      if (_MaskImage != null) {
        Color maskColor = _MaskImage.color;
        maskColor.a = enable ? 1 : 0;
        _MaskImage.color = maskColor;
      }
    }

    private void OnDestroy() {
      MaterialPool.ReturnMaterial(_GlintMaterial);
    }
  }
}