using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;

[Serializable]
public class RectTransformSettings {
	public Vector2 anchoredPosition = new Vector2( 0f, 0f );
	public Vector3 anchoredPosition3D = new Vector3( 0f, 0f, 0f );
	public Vector2 anchorMax = new Vector2( 0.5f, 0.5f );
	public Vector2 anchorMin = new Vector2( 0.5f, 0.5f );
	public Vector2 offsetMax = new Vector2( 0f, 0f );
	public Vector2 offsetMin = new Vector2( 0f, 0f );
	public Vector2 pivot = new Vector2( 0.5f, 0.5f );
	public Vector2 sizeDelta = new Vector2( 100f, 100f );
}

[ExecuteInEditMode]
public class RectTransformMultiSettings : MonoBehaviour {
	
	[SerializeField] List<RectTransformSettings> screenSettings = new List<RectTransformSettings>();

	bool smallScreen = false;
	RectTransform _rTrans;
	RectTransform rTrans {
		get {
			if(_rTrans == null) _rTrans = transform as RectTransform;
			return _rTrans;
		}
	}

	void OnEnable() {
		RefreshToScreen();
		ScreenMultiSettingsDetector.ShareSettings += RefreshSettingsToScreen;
	}

	void OnDisable() {
		ScreenMultiSettingsDetector.ShareSettings -= RefreshSettingsToScreen;
	}

	void RefreshToScreen() {
		float dpi = Screen.dpi;
		
		if(dpi == 0f) return;
		
		Vector2 screenSize = new Vector2(Screen.width, Screen.height);

//		if(Application.isEditor) {
//			screenSize = Handles.GetMainGameViewSize();
//		}

		float aspect = 1f;
		if(screenSize.y > 0f) aspect = screenSize.x / screenSize.y;

		smallScreen = aspect > (16f / 10.5f);

		//float screenHeightInches = (float)Screen.height / (float)dpi;
		//smallScreen = screenHeightInches < CozmoUtil.SMALL_SCREEN_MAX_HEIGHT;


		Debug.Log(gameObject.name + " RefreshToScreen smallScreen("+smallScreen+") width("+Screen.width+") height("+Screen.height+") aspect("+aspect+")");
		RefreshSettingsToScreen();
	}

	void RefreshSettingsToScreen() {
		
		int index = ScreenMultiSettingsDetector.CurrentIndex;
		while(screenSettings.Count <= index) {
			RectTransformSettings newSettings = new RectTransformSettings();
			SaveScreenSettings(newSettings);
			screenSettings.Add(newSettings);
		}

		LoadSettings(screenSettings[index]);
	}

	void LoadSettings(RectTransformSettings settings) {
		rTrans.anchoredPosition = settings.anchoredPosition;
		rTrans.anchoredPosition3D = settings.anchoredPosition3D;
		rTrans.anchorMax = settings.anchorMax;
		rTrans.anchorMin = settings.anchorMin;
		rTrans.offsetMax = settings.offsetMax;
		rTrans.offsetMin = settings.offsetMin;
		rTrans.pivot = settings.pivot;
		rTrans.sizeDelta = settings.sizeDelta;
	}
	
	void SaveScreenSettings(RectTransformSettings settings) {
		settings.anchoredPosition = rTrans.anchoredPosition;
		settings.anchoredPosition3D = rTrans.anchoredPosition3D;
		settings.anchorMax = rTrans.anchorMax;
		settings.anchorMin = rTrans.anchorMin;
		settings.offsetMax = rTrans.offsetMax;
		settings.offsetMin = rTrans.offsetMin;
		settings.pivot = rTrans.pivot;
		settings.sizeDelta = rTrans.sizeDelta;
	}

//	public void ToggleMode() {
//		smallScreen = !smallScreen;
//		LoadSettings();
//	}

	public void SaveCurrentSettings() {
		
		int index = ScreenMultiSettingsDetector.CurrentIndex;
		while(screenSettings.Count <= index) {
			RectTransformSettings newSettings = new RectTransformSettings();
			SaveScreenSettings(newSettings);
			screenSettings.Add(newSettings);
		}
		
		SaveScreenSettings(screenSettings[index]);
	}

}