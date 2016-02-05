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
    [ExecuteInEditMode]
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
        _MaskingFrame.sprite = _Graphic;
        _BackgroundImage.sprite = _Graphic;

        CreateMaterial();
        _MaskingFrame.material = _ClippingMaterial;

        _ClippingMaterial.SetVector("_AtlasUV", UIPrefabHolder.GetAtlasUVs(_Graphic));
      }

      private void OnEnable() {
        UpdateMaterial();
      }

      #if UNITY_EDITOR
      private void Update() {
        if (_ScreenSpace) {
          if ((_SpecifyEndClipping && _ClippingMaterial.shader != UIPrefabHolder.Instance.GradiantComplexClippingScreenspaceShader)
              || (!_SpecifyEndClipping && _ClippingMaterial.shader != UIPrefabHolder.Instance.GradiantSimpleClippingScreenspaceShader)) {
            if (_SpecifyEndClipping) {
              _ClippingMaterial.shader = UIPrefabHolder.Instance.GradiantComplexClippingScreenspaceShader;
            }
            else {
              _ClippingMaterial.shader = UIPrefabHolder.Instance.GradiantSimpleClippingScreenspaceShader;
            }
          }
        }
        else {
          if ((_SpecifyEndClipping && _ClippingMaterial.shader != UIPrefabHolder.Instance.GradiantComplexClippingShader)
              || (!_SpecifyEndClipping && _ClippingMaterial.shader != UIPrefabHolder.Instance.GradiantSimpleClippingShader)) {
            if (_SpecifyEndClipping) {
              _ClippingMaterial.shader = UIPrefabHolder.Instance.GradiantComplexClippingShader;
            }
            else {
              _ClippingMaterial.shader = UIPrefabHolder.Instance.GradiantSimpleClippingShader;
            }
          }
        }
        UpdateMaterial();
      }
      #endif

      private void CreateMaterial() {
        if (_ScreenSpace) {
          if (_SpecifyEndClipping) {
            _ClippingMaterial = MaterialPool.GetMaterial(UIPrefabHolder.Instance.GradiantComplexClippingScreenspaceShader);
          }
          else {
            _ClippingMaterial = MaterialPool.GetMaterial(UIPrefabHolder.Instance.GradiantSimpleClippingScreenspaceShader);
          }
        }
        else {
          if (_SpecifyEndClipping) {
            _ClippingMaterial = MaterialPool.GetMaterial(UIPrefabHolder.Instance.GradiantComplexClippingShader);
          }
          else {
            _ClippingMaterial = MaterialPool.GetMaterial(UIPrefabHolder.Instance.GradiantSimpleClippingShader);
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