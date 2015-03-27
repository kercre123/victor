#pragma strict
import Vectrosity;

var gridPixels = 50;
private var gridLine : VectorLine;

function Start () {
	gridLine = new VectorLine("Grid", new Vector2[0], null, 1.0);
	// Align 1-pixel lines on the pixel grid, so they don't potentially get messed up by anti-aliasing
	gridLine.rectTransform.anchoredPosition = Vector2(.5, .5);
	MakeGrid();
}

function OnGUI () {
	GUI.Label (Rect(10, 10, 30, 20), gridPixels.ToString());
	gridPixels = GUI.HorizontalSlider (Rect(40, 15, 590, 20), gridPixels, 5, 200);
	if (GUI.changed) {
		MakeGrid();
	}
}

function MakeGrid () {
	var numberOfGridPoints = ((Screen.width/gridPixels + 1) + (Screen.height/gridPixels + 1)) * 2;
	gridLine.Resize (numberOfGridPoints);
	
	var index = 0;
	for (var x = 0; x < Screen.width; x += gridPixels) {
		gridLine.points2[index++] = Vector2(x, 0);
		gridLine.points2[index++] = Vector2(x, Screen.height-1);
	}
	for (var y = 0; y < Screen.height; y += gridPixels) {
		gridLine.points2[index++] = Vector2(0, y);
		gridLine.points2[index++] = Vector2(Screen.width-1, y);
	}
		
	gridLine.Draw();
}