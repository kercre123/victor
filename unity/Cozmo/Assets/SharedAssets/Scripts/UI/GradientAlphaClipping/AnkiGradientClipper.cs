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

      private static SimpleObjectPool<Material> _ClippingMaterialPool;

      [SerializeField]
      private Image _BackgroundImage;

      [SerializeField]
      private Image _MaskingFrame;

      [SerializeField]
      private Sprite _Graphic;

      [SerializeField]
      [Range(0, 1)]
      private float _TopClipping;

      [SerializeField]
      [Range(0, 1)]
      private float _BottomClipping;

      [SerializeField]
      [Range(0, 1)]
      private float _LeftClipping;

      [SerializeField]
      [Range(0, 1)]
      private float _RightClipping;

      private Material _ClippingMaterial;
      private float _XMinNormalizedUV;
      private float _XMaxNormalizedUV;
      private float _YMinNormalizedUV;
      private float _YMaxNormalizedUV;

      private void Awake() {
        if (_ClippingMaterialPool == null) {
          _ClippingMaterialPool = new SimpleObjectPool<Material>(CreateClippingMaterial, 1);
        }

        _MaskingFrame.sprite = _Graphic;
        _BackgroundImage.sprite = _Graphic;

        _ClippingMaterial = _ClippingMaterialPool.GetObjectFromPool();
        _ClippingMaterial.name = "Custom Clipping Material";
        _MaskingFrame.material = _ClippingMaterial;

        float textureAtlasPixelWidth = _Graphic.texture.width;
        _XMinNormalizedUV = _Graphic.textureRect.xMin / textureAtlasPixelWidth;
        _XMaxNormalizedUV = _Graphic.textureRect.xMax / textureAtlasPixelWidth;

        float textureAtlasPixelHeight = _Graphic.texture.height;
        _YMinNormalizedUV = _Graphic.textureRect.yMin / textureAtlasPixelHeight;
        _YMaxNormalizedUV = _Graphic.textureRect.yMax / textureAtlasPixelHeight;
      }

      private void OnEnable() {
        _ClippingMaterial.SetVector("_Clipping", new Vector4(_TopClipping, _LeftClipping, 
          _BottomClipping, _RightClipping));

        _ClippingMaterial.SetVector("_AtlasUV", new Vector4(_XMinNormalizedUV,
          _YMinNormalizedUV,
          _XMaxNormalizedUV - _XMinNormalizedUV,
          _YMaxNormalizedUV - _YMinNormalizedUV));
      }

      private void OnDestroy() {
        if (_ClippingMaterialPool != null) {
          _ClippingMaterialPool.ReturnToPool(_ClippingMaterial);
        }
      }

      private Material CreateClippingMaterial() {
        return new Material(UIPrefabHolder.Instance.SoftClippingMaterial){ hideFlags = HideFlags.HideAndDontSave };
      }
    }
  }
}