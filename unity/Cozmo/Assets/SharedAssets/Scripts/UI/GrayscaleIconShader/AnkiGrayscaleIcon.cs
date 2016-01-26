using UnityEngine;
using UnityEngine.UI;
using System.Collections;

namespace Cozmo.UI {
  [ExecuteInEditMode, RequireComponent(typeof(Image))]
  public class AnkiGrayscaleIcon : MonoBehaviour {

    private static SimpleObjectPool<Material> _GrayscaleMaterialPool;

    private static SimpleObjectPool<Material> GetMaterialPool() {
      if (_GrayscaleMaterialPool == null) {
        _GrayscaleMaterialPool = new SimpleObjectPool<Material>(CreateGrayscaleMaterial, 1);
      }
      return _GrayscaleMaterialPool;
    }

    private static Material CreateGrayscaleMaterial() {
      Material grayScaleMaterial = new Material(UIPrefabHolder.Instance.GrayscaleShader){ hideFlags = HideFlags.HideAndDontSave };
      grayScaleMaterial.name = "Grayscale Material (Generated)";
      return grayScaleMaterial;
    }

    [SerializeField]
    private Image _ImageToGrayscale;

    [SerializeField]
    private bool _StartGrayscale = false;
    
    private bool _IsGrayscale = false;
    private Material _GrayscaleMaterialInstance = null;

    public bool IsGrayscale {
      get {
        return _IsGrayscale;
      }

      set {
        if (value != _IsGrayscale) {
          _IsGrayscale = value;
          if (_IsGrayscale) {
            _GrayscaleMaterialInstance = GetMaterialPool().GetObjectFromPool();
            _ImageToGrayscale.material = _GrayscaleMaterialInstance;
          }
          else {
            _GrayscaleMaterialPool.ReturnToPool(_GrayscaleMaterialInstance);
            _GrayscaleMaterialInstance = null;
            _ImageToGrayscale.material = null;
          }
        }
      }
    }

    private void OnEnable() {
      if (_StartGrayscale) {
        IsGrayscale = _StartGrayscale;
      }
    }

    #if UNITY_EDITOR
    private void Update() {
      if (_StartGrayscale != IsGrayscale) {
        IsGrayscale = _StartGrayscale;
      }
    }
    #endif

    private void OnDestroy() {
      if (_GrayscaleMaterialPool != null && _GrayscaleMaterialInstance != null) {
        _GrayscaleMaterialPool.ReturnToPool(_GrayscaleMaterialInstance);
      }
    }
  }
}