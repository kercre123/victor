using UnityEngine;
using UnityEngine.UI;
using System.Collections;

namespace Cozmo.UI {
  public class AnkiAnimateGlint : MonoBehaviour {

    [SerializeField]
    private Image _MaskImage;

    [SerializeField]
    private Texture _GlintTexture;

    [SerializeField]
    private float _LoopSpeed = 5;

    [SerializeField]
    private float _SecondsBetweenLoops = 2;

    private Vector2 _UVOffset;
    private float _SecondsSinceLastLoop = -1f;
    private bool _AnimateGlint = true;
    private float _GlintWidthToMaskWidthRatio;
    private bool _IsEnabled = true;

    private Material _GlintMaterial;

    private void Awake() {
      _MaskImage.enabled = true;
      _AnimateGlint = false;
      _GlintMaterial = MaterialPool.GetMaterial(UIPrefabHolder.Instance.AnimatedGlintShader);
      _MaskImage.material = _GlintMaterial;
      _MaskImage.material.SetTexture("_GlintTex", _GlintTexture);
      _MaskImage.material.SetVector("_AtlasUV", UIPrefabHolder.GetAtlasUVs(_MaskImage.sprite));
    }

    private void Start() {
      RectTransform rectTransform = this.transform as RectTransform;
      _GlintWidthToMaskWidthRatio = rectTransform.rect.width / _GlintTexture.width;
      _MaskImage.material.SetFloat("_GlintWidthToMaskWidthRatio", _GlintWidthToMaskWidthRatio);
      _UVOffset = new Vector2(Random.Range(-_GlintWidthToMaskWidthRatio, _GlintWidthToMaskWidthRatio), 0);
      _AnimateGlint = true;
    }

    public void EnableGlint(bool enable) {
      _IsEnabled = enable;
      Color maskColor = _MaskImage.color;
      maskColor.a = enable ? 1 : 0;
      _MaskImage.color = maskColor;
    }

    private void Update() {
      UpdateMaterial();
    }

    private void OnDestroy() {
      MaterialPool.ReturnMaterial(_GlintMaterial);
    }

    private void UpdateMaterial() {
      if (!_IsEnabled) {
        return;
      }

      if (_AnimateGlint) {
        _UVOffset.x -= _LoopSpeed * Time.deltaTime;
        _UVOffset.y = 0;

        if (_UVOffset.x < -_GlintWidthToMaskWidthRatio) {
          _UVOffset.x += 2 * _GlintWidthToMaskWidthRatio;
          _SecondsSinceLastLoop = 0f;
          _AnimateGlint = false;
        }

        _MaskImage.material.SetVector("_UVOffset", new Vector4(_UVOffset.x,
          _UVOffset.y, 0, 0));
      }
      else {
        _SecondsSinceLastLoop += Time.deltaTime;
        if (_SecondsSinceLastLoop >= _SecondsBetweenLoops) {
          _SecondsSinceLastLoop = -1f;
          _AnimateGlint = true;
        }
      }
    }
  }
}