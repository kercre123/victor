using UnityEngine;
using System.Collections;

public class GeodomeForeground : MonoBehaviour {

  public float DegreesPerSecond;
	
	// Update is called once per frame
	void Update () {
    float delta = Mathf.Lerp(0f, DegreesPerSecond, Time.deltaTime);
    transform.Rotate(Vector3.forward * delta);
	}
}
