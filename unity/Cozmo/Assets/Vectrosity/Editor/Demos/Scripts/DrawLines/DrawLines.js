#pragma strict
import Vectrosity;
import UnityEngine.GUILayout;

var lineMaterial : Material;
var rotateSpeed = 90.0;
var maxPoints = 500;
private var line : VectorLine;
private var endReached : boolean;
private var lineColors : List.<Color>;
private var continuous = true;
private var fillJoins = false;
private var weldJoins = false;
private var oldFillJoins = false;
private var oldWeldJoins = false;
private var thickLine = false;
private var canClick = true;

function Start () {
	SetLine();
}

function SetLine () {
	VectorLine.Destroy (line);
	
	if (!continuous) {
		fillJoins = false;
	}
	var lineType = (continuous? LineType.Continuous : LineType.Discrete);
	var joins = (fillJoins? Joins.Fill : Joins.None);
	var lineWidth = (thickLine? 24 : 2);
	var arrayLength = continuous? 1 : 0;
	lineColors = new List.<Color>();
	
	line = new VectorLine("Line", new Vector2[arrayLength], lineMaterial, lineWidth, lineType, joins);
	line.drawTransform = transform;
		
	endReached = false;
}

function Update () {
	// Since we can rotate the transform, get the local space for the current point, so the mouse position won't be rotated with the line
	var mousePos = transform.InverseTransformPoint (Input.mousePosition);
	// Add a line point when the mouse is clicked
	if (Input.GetMouseButtonDown (0) && canClick && !endReached) {
		line.points2.Add (mousePos);
		// Add a new color entry for each line segment...discrete lines have 1 segment for every two points so only add when the count is even
		if (continuous || (!continuous && line.points2.Count%2 == 0)) {
			lineColors.Add (Color.white);
		}
		// Start off with 2 points for discrete lines
		if (!continuous && line.points2.Count == 1) {
			line.points2.Add (Vector2.zero);
			lineColors.Add (Color.white);
		}
		if (line.points2.Count == maxPoints) {
			endReached = true;
		}
	}
	
	// The last line point should always be where the mouse is
	if (continuous || (!continuous && line.points2.Count >= 2)) {	// Only draw when continuous, or when discrete && there are at least 2 points
		line.points2[line.points2.Count-1] = mousePos;
		line.Draw();
	}
	
	// Rotate around midpoint of screen
	transform.RotateAround (Vector2(Screen.width/2, Screen.height/2), Vector3.forward, Time.deltaTime * rotateSpeed * Input.GetAxis ("Horizontal"));
}

function OnGUI () {
	var rect = Rect(20, 20, 265, 280);
	canClick = (rect.Contains (Event.current.mousePosition)? false : true);
	BeginArea (rect);
	GUI.contentColor = Color.black;
	Label("Click to add points to the line\nRotate with the right/left arrow keys");
	Space (5);
	Label("This option takes effect when line is reset:");
	continuous = Toggle(continuous, "Continuous line");
	Space (5);
	GUI.contentColor = Color.white;
	if (Button("Reset line", Width(150))) {
		SetLine();
	}
	Space (15);
	GUI.contentColor = Color.black;
	thickLine = Toggle (thickLine, "Thick line");
	line.lineWidth = (thickLine? 24 : 2);
	fillJoins = Toggle (fillJoins, "Fill joins (only works with continuous line)");
	if (!line.continuous) {
		fillJoins = false;
	}
	weldJoins = Toggle (weldJoins, "Weld joins");
	if (oldFillJoins != fillJoins) {
		if (fillJoins) {
			weldJoins = false;
		}
		oldFillJoins = fillJoins;
	}
	else if (oldWeldJoins != weldJoins) {
		if (weldJoins) {
			fillJoins = false;
		}
		oldWeldJoins = weldJoins;
	}
	if (fillJoins) {
		line.joins = Joins.Fill;
	}
	else if (weldJoins) {
		line.joins = Joins.Weld;
	}
	else {
		line.joins = Joins.None;
	}
	Space (10);
	GUI.contentColor = Color.white;
	if (Button ("Randomize Color", Width(150))) {
		RandomizeColor();
	}
	if (Button ("Randomize All Colors", Width(150))) {
		RandomizeAllColors();
	}
	
	if (endReached) {
		GUI.contentColor = Color.black;
		Label ("No more points available. You must be bored!");
	}
	EndArea();
}

function RandomizeColor () {
	line.color = Color(Random.value, Random.value, Random.value);
}

function RandomizeAllColors () {
	for (var i = 0; i < lineColors.Count; i++) {
		lineColors[i] = Color(Random.value, Random.value, Random.value);
	}
	line.SetColors (lineColors);
}