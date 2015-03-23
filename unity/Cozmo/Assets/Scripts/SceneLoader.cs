using UnityEngine;
using System.Collections;

public class SceneLoader : MonoBehaviour {

	[SerializeField] string scene = null;

	public void LoadScene () {
		if(string.IsNullOrEmpty(scene)) return;

		Application.LoadLevel(scene);
	}
}
