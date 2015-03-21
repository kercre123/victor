#pragma strict
import Vectrosity;

var lineThickness = 10.0;
var extraThickness = 2;
var numberOfLines = 2;
private var lines : VectorLine[];
private var wasSelected : boolean[];
private var index = 0;

function Start () {
	lines = new VectorLine[numberOfLines];
	wasSelected = new boolean[numberOfLines];
	for (var i = 0; i < numberOfLines; i++) {
		lines[i] = new VectorLine("SelectLine", new Vector2[5], null, lineThickness, LineType.Continuous, Joins.Fill);
		SetPoints (i);
	}
}

function SetPoints (i : int) {
	for (var j = 0; j < lines[i].points2.Count; j++) {
		lines[i].points2[j] = Vector2(Random.Range(0, Screen.width), Random.Range(0, Screen.height-20));
	}
	lines[i].Draw();	
}

function Update () {
	for (var i = 0; i < numberOfLines; i++) {
		if (lines[i].Selected (Input.mousePosition, extraThickness, index)) {
			if (!wasSelected[i]) {	// We use wasSelected to update the line color only when needed, instead of every frame
				lines[i].SetColor (Color.green);
				wasSelected[i] = true;
			}
			if (Input.GetMouseButtonDown(0)) {
				SetPoints (i);
			}
		}
		else {
			if (wasSelected[i]) {
				wasSelected[i] = false;
				lines[i].SetColor (Color.white);
			}
		}
	}
}

function OnGUI () {
	GUI.Label (Rect(10, 10, 800, 30), "Click a line to make a new line");
}