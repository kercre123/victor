using UnityEngine;
using System.Collections;

#if FACTORY_TEST

public class SpinIcon : MonoBehaviour {

  // Use this for initialization
  void Start() {
	
  }
	
  // Update is called once per frame
  void Update() {
    transform.RotateAround(transform.position, Vector3.forward, -250.0f * Time.deltaTime);
  }
}

#endif
