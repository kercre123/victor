using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using Cozmo;
using Cozmo.UI;

[RequireComponent(typeof(Image))]
public class BlurFilter : MonoBehaviour {

  private Material _Material;

  private void Awake() {
    _Material = MaterialPool.GetMaterial(UIPrefabHolder.Instance.BlurShader);
    var image = GetComponent<Image>();
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
