using UnityEngine;
using UnityEngine.UI;
using System.Collections;

[ExecuteInEditMode]
public class ImageSheen : MonoBehaviour {
  [SerializeField] Vector2 uv = new Vector2(0f, 0f);
  [SerializeField] Color color = Color.white;
  [SerializeField] float amount = 0f;

  [SerializeField] string textureName = "_FlashMaskTex";
  [SerializeField] string colorName = "_FlashColor";
  [SerializeField] string amountName = "_FlashAmount";

  [SerializeField] Material sourceMaterial;

  Image image;
  Material matCopy;

  void OnEnable() {
    image = GetComponent<Image>();
    if (sourceMaterial == null)
      return;
    matCopy = new Material(sourceMaterial);
    image.material = matCopy;
  }

  void LateUpdate() {
    if (sourceMaterial == null)
      return;
    if (matCopy == null) {
      matCopy = new Material(sourceMaterial);
      image.material = matCopy;
    }

    matCopy.SetTextureOffset(textureName, uv);
    matCopy.SetFloat(amountName, amount);
    matCopy.SetColor(colorName, color);
  }

  void OnDisable() {
    if (matCopy != null) {
      if (Application.isPlaying) {
        Destroy(matCopy);
      }
      else {
        DestroyImmediate(matCopy);
      }
    }
  }
}