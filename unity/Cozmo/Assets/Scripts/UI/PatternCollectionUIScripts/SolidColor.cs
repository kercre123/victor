using UnityEngine;
using System.Collections;

[ExecuteInEditMode]
public class SolidColor : MonoBehaviour {

  public Color ObjectColor;

  private Color currentColor;

	// Update is called once per frame
	void Update () {
    if (ObjectColor != currentColor) {
      gameObject.GetComponent<Renderer>().material.color = currentColor = ObjectColor;
    }
	}
}
