using UnityEngine;
using System;
using System.Collections;

[ExecuteInEditMode]
public class CozmoScreenMultiSettingsDetector : ScreenMultiSettingsDetector {
	protected override void UpdateIndex() {

		float dpi = Screen.dpi;
		
		if(dpi == 0f) return;
		
		Vector2 screenSize = new Vector2(Screen.width, Screen.height);
		
		//		if(Application.isEditor) {
		//			screenSize = Handles.GetMainGameViewSize();
		//		}
		
		float aspect = 1f;
		if(screenSize.y > 0f) aspect = screenSize.x / screenSize.y;
		
		bool wideScreen = aspect > (16f / 10.5f);
		

		CurrentIndex = wideScreen ? 1 : 0;
	}

}
