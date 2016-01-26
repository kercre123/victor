using UnityEngine;
using UnityEngine.UI;
using System.Collections;

namespace Cozmo.UI {
  public class AnkiAnimateGlint : MonoBehaviour {
    private static SimpleObjectPool<Material> _GlintMaterialPool;

    private static SimpleObjectPool<Material> GetMaterialPool() {
      if (_GlintMaterialPool == null) {
        _GlintMaterialPool = new SimpleObjectPool<Material>(CreateGlintMaterial, 1);
      }
      return _GlintMaterialPool;
    }

    private static Material CreateGlintMaterial() {
      Material glintMaterial = new Material(UIPrefabHolder.Instance.AnimatedGlintShader){ hideFlags = HideFlags.HideAndDontSave };
      glintMaterial.name = "Animated Glint Material (Generated)";
      return glintMaterial;
    }

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

    private Material _GlintMaterial;

    private void Awake() {
      _AnimateGlint = false;
      _GlintMaterial = GetMaterialPool().GetObjectFromPool();
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

    private void Update() {
      UpdateMaterial();
    }

    private void OnDestroy() {
      if (_GlintMaterialPool != null && _GlintMaterial != null) {
        _GlintMaterialPool.ReturnToPool(_GlintMaterial);
      }
    }

    private void UpdateMaterial() {
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