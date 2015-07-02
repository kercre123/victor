using UnityEngine;
using System.Collections;

public class SceneLoader : MonoBehaviour {

	[SerializeField] string scene = null;
	[SerializeField] bool onEnable = false;
	[SerializeField] int onUpdateFrame = -1;

	int frames = 0;

	void OnEnable() {
		if(onEnable) LoadScene();
	}

	void Update() {
		frames++;
		if(onUpdateFrame == frames) LoadScene();
	}

	public void LoadScene() {
		if(string.IsNullOrEmpty(scene)) return;

		Debug.Log("SceneLoader Application.LoadLevel("+scene+")");
		Application.LoadLevel(scene);
	}
}
