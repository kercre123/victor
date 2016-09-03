using UnityEngine;
using UnityEngine.UI;
using System.Collections;

namespace Cozmo {
  namespace UI {
    /// <summary>
    /// Emulates a soft gradient effect using two Images, one drawn fully in 
    /// the background and one drawn with blending as a "frame" on top of the
    /// <see cref="ScrollRect"/>'s contents. Use in conjunction with a
    /// <see cref="RectMask2D"/> to handle backgrounds with a fully transparent 
    /// border.  
    /// </summary>
    public class AnkiGradientClipper : MonoBehaviour {

      [SerializeField]
      private Image _BackgroundImage;

      [SerializeField]
      private Image _MaskingFrame;

      [SerializeField]
      private Sprite _Graphic;

      [SerializeField]
      [Range(-0.05f, 2f)]
      private float _TopClippingStart = 1;

      [SerializeField]
      [Range(-0.05f, 2f)]
      private float _BottomClippingStart = 1;

      [SerializeField]
      [Range(-0.05f, 2f)]
      private float _LeftClippingStart = 1;

      [SerializeField]
      [Range(-0.05f, 2f)]
      private float _RightClippingStart = 1;

      [SerializeField]
      private bool _SpecifyEndClipping = false;

      [SerializeField]
      private bool _ScreenSpace = false;

      [SerializeField]
      [Range(-0.05f, 2)]
      private float _TopClippingEnd = 0;

      [SerializeField]
      [Range(-0.05f, 2)]
      private float _BottomClippingEnd = 0;

      [SerializeField]
      [Range(-0.05f, 2)]
      private float _LeftClippingEnd = 0;

      [SerializeField]
      [Range(-0.05f, 2)]
      private float _RightClippingEnd = 0;

      private Material _ClippingMaterial;

      private void Awake() {
        if (_Graphic) {
          _MaskingFrame.sprite = _Graphic;
          _BackgroundImage.sprite = _Graphic;
        }
        CreateMaterial();
        _MaskingFrame.material = _ClippingMaterial;
        if (_Graphic) {
          _ClippingMaterial.SetVector("_AtlasUV", UIManager.GetAtlasUVs(_Graphic));
        }
      }

      private void OnEnable() {
        UpdateMaterial();
      }
#if UNITY_EDITOR
      private void Update() {
        if (_ClippingMaterial == null || ShaderHolder.Instance == null) {
          return;
        }
        if (_ScreenSpace) {
          if ((_SpecifyEndClipping && _ClippingMaterial.shader != ShaderHolder.Instance.GradiantComplexClippingScreenspaceShader)) {
            _ClippingMaterial.shader = ShaderHolder.Instance.GradiantComplexClippingScreenspaceShader;
          }
        }
        else {
          if ((_Graphic != null && _ClippingMaterial.shader != ShaderHolder.Instance.GradiantTextureClippingShader)
              || (_Graphic == null && _ClippingMaterial.shader != ShaderHolder.Instance.GradiantAlphaClippingShader)) {
            if (_Graphic != null) {
              _ClippingMaterial.shader = ShaderHolder.Instance.GradiantTextureClippingShader;
            }
            else {
              _ClippingMaterial.shader = ShaderHolder.Instance.GradiantAlphaClippingShader;
            }
          }
        }
        UpdateMaterial();
      }
#endif

      private void CreateMaterial() {
        if (_ScreenSpace) {
          _ClippingMaterial = MaterialPool.GetMaterial(ShaderHolder.Instance.GradiantComplexClippingScreenspaceShader, _MaskingFrame.defaultMaterial.renderQueue);
        }
        else {
          if (_Graphic != null) {
            _ClippingMaterial = MaterialPool.GetMaterial(ShaderHolder.Instance.GradiantTextureClippingShader, _MaskingFrame.defaultMaterial.renderQueue);
          }
          else {
            _ClippingMaterial = MaterialPool.GetMaterial(ShaderHolder.Instance.GradiantAlphaClippingShader, _MaskingFrame.defaultMaterial.renderQueue);
          }
        }
      }

      private void UpdateMaterial() {
        if (_SpecifyEndClipping) {
          _ClippingMaterial.SetVector("_ClippingEnd", new Vector4(_RightClippingEnd,
            _TopClippingEnd, _LeftClippingEnd, _BottomClippingEnd));

          _ClippingMaterial.SetVector("_ClippingSize", new Vector4(
            Mathf.Clamp(_RightClippingStart - _RightClippingEnd, 0.05f, 1f),
            Mathf.Clamp(_TopClippingStart - _TopClippingEnd, 0.05f, 1f),
            Mathf.Clamp(_LeftClippingStart - _LeftClippingEnd, 0.05f, 1f),
            Mathf.Clamp(_BottomClippingStart - _BottomClippingEnd, 0.05f, 1f)));
        }
        else {
          _ClippingMaterial.SetVector("_Clipping", new Vector4(_RightClippingStart,
            _TopClippingStart, _LeftClippingStart, _BottomClippingStart));
        }
        _ClippingMaterial.color = _MaskingFrame.color;
      }

      private void OnDestroy() {
        MaterialPool.ReturnMaterial(_ClippingMaterial);
      }
    }
  }
}