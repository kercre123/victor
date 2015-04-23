using UnityEngine;
using System.Collections;

public class SceneLoader : MonoBehaviour {

	[SerializeField] string scene = null;

	public void LoadScene () {
		if(string.IsNullOrEmpty(scene)) return;

		Debug.Log("SceneLoader Application.LoadLevel("+scene+")");
		Application.LoadLevel(scene);
	}
}
