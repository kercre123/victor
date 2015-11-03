using UnityEngine;
using System.Collections;

public class DebugMenuDialog : MonoBehaviour {

	public void OnDebugMenuCloseTap(){
    GameObject.Destroy(gameObject);
  }
}
