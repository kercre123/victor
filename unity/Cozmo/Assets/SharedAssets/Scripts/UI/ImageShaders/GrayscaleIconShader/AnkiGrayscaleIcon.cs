using UnityEngine;
using UnityEngine.UI;
using System.Collections;

namespace Cozmo.UI {
  [ExecuteInEditMode, RequireComponent(typeof(Image))]
  public class AnkiGrayscaleIcon : MonoBehaviour {

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
            _GrayscaleMaterialInstance = MaterialPool.GetMaterial(ShaderHolder.Instance.GrayscaleShader, _ImageToGrayscale.defaultMaterial.renderQueue);
            _ImageToGrayscale.material = _GrayscaleMaterialInstance;
          }
          else {
            MaterialPool.ReturnMaterial(_GrayscaleMaterialInstance);
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
      MaterialPool.ReturnMaterial(_GrayscaleMaterialInstance);
    }
  }
}