using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using Cozmo;
using Cozmo.UI;

[RequireComponent(typeof(Image))]
public class BlurFilter : MonoBehaviour {

  private Material _Material;

  private void Awake() {
    var image = GetComponent<Image>();
    _Material = MaterialPool.GetMaterial(ShaderHolder.Instance.BlurShader, image.defaultMaterial.renderQueue);
    image.material = _Material;
    image.enabled = true;
    OnRectTransformDimensionsChange();
  }

  private void OnDestroy() {
    MaterialPool.ReturnMaterial(_Material);
    _Material = null;
  }

  private void OnRectTransformDimensionsChange() {
    if (_Material != null) {
      var size = ((RectTransform)transform).rect.size;
      _Material.SetVector("_Spread", new Vector4(2 / size.x, 2 / size.y, 0, 0));
    }
  }
}
