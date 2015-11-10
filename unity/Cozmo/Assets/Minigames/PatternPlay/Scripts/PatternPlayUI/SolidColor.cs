using UnityEngine;
using System.Collections;

[ExecuteInEditMode]
public class SolidColor : MonoBehaviour {

  public Color ObjectColor {
    get {
      return _LightMaterial.color;
    }
    set {
      _LightMaterial.color = value;
    }
  }

  private Material _LightMaterial;

  private void Awake() {
    Renderer renderer = gameObject.GetComponent<Renderer>();
    _LightMaterial = new Material(Shader.Find("Diffuse"));
    _LightMaterial.color = ObjectColor;

    // Let unity know we'll handle destroying this material
    _LightMaterial.hideFlags = HideFlags.HideAndDontSave;

    renderer.sharedMaterial = _LightMaterial;
  }

  private void OnDestroy() {
    DestroyImmediate(_LightMaterial);
  }
}
