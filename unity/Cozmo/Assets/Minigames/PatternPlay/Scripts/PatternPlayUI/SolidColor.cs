using UnityEngine;
using System.Collections;

[ExecuteInEditMode]
public class SolidColor : MonoBehaviour {

  public Color ObjectColor {
    get {
      return lightMaterial_.color;
    }
    set {
      lightMaterial_.color = value;
    }
  }

  private Material lightMaterial_;

  private void Awake(){
    Renderer renderer = gameObject.GetComponent<Renderer>();
    lightMaterial_ = new Material(Shader.Find("Diffuse"));
    lightMaterial_.color = ObjectColor;

    // Let unity know we'll handle destroying this material
    lightMaterial_.hideFlags = HideFlags.HideAndDontSave;

    renderer.sharedMaterial = lightMaterial_;
  }

  private void OnDestroy(){
    DestroyImmediate(lightMaterial_);
  }
}
