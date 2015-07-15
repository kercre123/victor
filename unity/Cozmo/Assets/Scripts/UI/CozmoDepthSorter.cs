using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public enum CozmoCanvasDepth {
	
	Screen_Status,
	Screen_Controls,
	Screen_Vision,
	Screen_Game,
	
	Overlay_Status,
	Overlay_Game,
	Overlay_Building,
	Overlay_Controls,
	Overlay_Inventory,
	Overlay_BackButton,
	Overlay_Shell,
	Overlay_Debug,
	Overlay_Options,
}

/// <summary>
/// slap this component on any canvas to simplify maintaining its depth relative to all the other canvases in the game
///		simple drop down that when mutated causes the canvas's pertinent values to refresh
/// </summary>
[ExecuteInEditMode]
public class CozmoDepthSorter : MonoBehaviour {

	[SerializeField] CozmoCanvasDepth depth = CozmoCanvasDepth.Screen_Status; 

	CozmoCanvasDepth lastDepth = CozmoCanvasDepth.Screen_Status; 
	Canvas canvas;

	int SortingOrderForDepth(CozmoCanvasDepth depth) {
		switch(depth) {
			case CozmoCanvasDepth.Screen_Status: 		return 2;
			case CozmoCanvasDepth.Screen_Controls: 		return 5;
			case CozmoCanvasDepth.Screen_Vision: 		return 10;
			case CozmoCanvasDepth.Screen_Game: 			return 15;

			case CozmoCanvasDepth.Overlay_Status: 		return 2;
			case CozmoCanvasDepth.Overlay_Game: 		return 5;
			case CozmoCanvasDepth.Overlay_Building: 	return 10;
			case CozmoCanvasDepth.Overlay_Controls: 	return 15;
			case CozmoCanvasDepth.Overlay_Shell: 		return 20;
			case CozmoCanvasDepth.Overlay_Debug: 		return 25;
			case CozmoCanvasDepth.Overlay_BackButton: 	return 30;
			case CozmoCanvasDepth.Overlay_Inventory: 	return 35;
			case CozmoCanvasDepth.Overlay_Options: 		return 40;
		}

		return 0;
	}

	RenderMode RenderModeForDepth(CozmoCanvasDepth depth) {
		switch(depth) {
			case CozmoCanvasDepth.Screen_Status:
			case CozmoCanvasDepth.Screen_Controls:
			case CozmoCanvasDepth.Screen_Vision:
			case CozmoCanvasDepth.Screen_Game:
				return RenderMode.ScreenSpaceCamera;
				
			case CozmoCanvasDepth.Overlay_Status:
			case CozmoCanvasDepth.Overlay_Game:
			case CozmoCanvasDepth.Overlay_Building:
			case CozmoCanvasDepth.Overlay_Controls:
			case CozmoCanvasDepth.Overlay_Inventory:
			case CozmoCanvasDepth.Overlay_BackButton:
			case CozmoCanvasDepth.Overlay_Debug:
			case CozmoCanvasDepth.Overlay_Shell:
			case CozmoCanvasDepth.Overlay_Options:
				return RenderMode.ScreenSpaceOverlay;
		}

		return RenderMode.ScreenSpaceOverlay;
	}

	void OnEnable() {
		canvas = GetComponent<Canvas>();
		Refresh();
	}

	void Update() {
		if(Dirty())	Refresh();
	}

	bool Dirty() {
		return lastDepth != depth;
	}

	void Refresh() {
		lastDepth = depth;
		if(canvas == null) return;

		canvas.renderMode = RenderModeForDepth(depth);

		int sortOrder = SortingOrderForDepth(depth);
		canvas.sortingOrder = sortOrder;

		if(canvas.renderMode == RenderMode.ScreenSpaceCamera) {
			if(transform.parent != null) {
				Camera cam = canvas.worldCamera;
				if(cam == null) {
					cam = transform.parent.gameObject.GetComponentInChildren<Camera>();
					canvas.worldCamera = cam;
				}
				cam.depth = sortOrder;
			}
			else {
				canvas.worldCamera = Camera.main;
			}
		}
		else {
			canvas.worldCamera = null;
		}
	}
}
