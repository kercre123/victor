using UnityEngine;
using System;
using System.Collections;

[ExecuteInEditMode]
public class ScreenMultiSettingsDetector : MonoBehaviour {

	float width;
	float height;
	ScreenOrientation orientation;

	public static Action ShareSettings = null;
	public static int CurrentIndex = 0;

	public static ScreenMultiSettingsDetector instance;

	void Awake() {
		if(instance != null && instance != this) {
			if(Application.isPlaying) {
				GameObject.Destroy(gameObject);
				return;
			}

			gameObject.SetActive(false);
			return;
		}

		DontDestroyOnLoad(gameObject);

		instance = this;
	}

	void OnEnable () {
		Refresh();
	}

	void Update () {
		if(Dirty()) {
			//Debug.Log("ScreenChangeDetector ChangeDetected!");
			Refresh();
		}	
	}

	void SaveSettings() {
		width = Screen.width;
		height = Screen.height;
		orientation = Screen.orientation;
	}

	bool Dirty() {
		if(width != Screen.width) return true;
		if(height != Screen.height) return true;
		if(orientation != Screen.orientation) return true;
		return false;
	}

	void Refresh() {
		SaveSettings();
		UpdateIndex();
		if(ShareSettings != null) ShareSettings();
	}

	protected virtual void UpdateIndex() {
		CurrentIndex = 0;
	}

}
