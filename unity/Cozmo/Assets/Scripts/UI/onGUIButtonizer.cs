using UnityEngine;
using System.Collections;

public class onGUIButtonizer : MonoBehaviour {

	SceneLoader sceneLoader;

	void Awake() {
		sceneLoader = GetComponent<SceneLoader>();
	}

	void OnGUI () {
		if(GUI.Button(new Rect(0,0,200, 200), "LoadLevel")) {
			sceneLoader.LoadScene();
		}
	}

//	void OnLevelWasLoaded() {
//		
//		GameObject[] objects = Resources.FindObjectsOfTypeAll<GameObject>();
//		foreach(GameObject obj in objects) obj.hideFlags = HideFlags.None;
//	}
}
