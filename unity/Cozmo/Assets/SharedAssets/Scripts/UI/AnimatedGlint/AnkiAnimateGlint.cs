using UnityEngine;
using UnityEngine.UI;
using System.Collections;

namespace Cozmo.UI {
  public class AnkiAnimateGlint : MonoBehaviour {

    [SerializeField]
    private Image _Image;

    [SerializeField]
    private float _LoopSpeed = 5;

    [SerializeField]
    private float _SecondsBetweenLoops = 2;

    private Vector2 _UVOffset;
    private float _SecondsSinceLastLoop = -1f;
    private bool _AnimateGlint = true;
    private float _WidthToHeightRatio;

    private void Awake() {
      _AnimateGlint = false;
      _Image.material.SetVector("_AtlasUV", UIPrefabHolder.GetAtlasUVs(_Image.sprite));
    }

    private void Start() {
      RectTransform rectTransform = this.transform as RectTransform;
      Texture glintTexture = _Image.material.GetTexture("_MaskTex");
      _WidthToHeightRatio = rectTransform.rect.width / glintTexture.width;
      _Image.material.SetFloat("_WidthToHeightRatio", _WidthToHeightRatio);
      _UVOffset = new Vector2(Random.Range(-_WidthToHeightRatio, _WidthToHeightRatio), 0);
      _AnimateGlint = true;
    }

    private void Update() {
      UpdateMaterial();
    }

    private void UpdateMaterial() {
      if (_AnimateGlint) {
        _UVOffset.x -= _LoopSpeed * Time.deltaTime;
        _UVOffset.y = 0;

        if (_UVOffset.x < -_WidthToHeightRatio) {
          _UVOffset.x += 2 * _WidthToHeightRatio;
          _SecondsSinceLastLoop = 0f;
          _AnimateGlint = false;
        }

        _Image.material.SetVector("_UVOffset", new Vector4(_UVOffset.x,
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