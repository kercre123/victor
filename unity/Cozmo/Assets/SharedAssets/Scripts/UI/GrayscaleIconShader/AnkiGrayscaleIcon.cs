using UnityEngine;
using UnityEngine.UI;
using System.Collections;

namespace Cozmo.UI {
  [ExecuteInEditMode, RequireComponent(typeof(Image))]
  public class AnkiGrayscaleIcon : MonoBehaviour {

    private static SimpleObjectPool<Material> _GrayscaleMaterialPool;

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
            if (_GrayscaleMaterialPool == null) {
              _GrayscaleMaterialPool = new SimpleObjectPool<Material>(CreateGrayscaleMaterial, 1);
            }
            _GrayscaleMaterialInstance = _GrayscaleMaterialPool.GetObjectFromPool();
            _GrayscaleMaterialInstance.name = "Custom Grayscale Material";
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

    private Material CreateGrayscaleMaterial() {
      return new Material(UIPrefabHolder.Instance.GrayscaleMaterial){ hideFlags = HideFlags.HideAndDontSave };
    }
  }
}