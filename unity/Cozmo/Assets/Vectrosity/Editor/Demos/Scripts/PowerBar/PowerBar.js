// Makes an efficient randomly-animated 3/4 circle power bar using MakeArc and SetColor
#pragma strict
import Vectrosity;

var speed = 0.25;
var lineWidth = 25;
var radius = 60.0;
var segmentCount = 200;

private var bar : VectorLine;
private var position : Vector2;
private var currentPower : float;
private var targetPower : float;

function Start () {
	position = Vector2(radius+20, Screen.height - (radius+20));
	
	// Set up a white circle for the background of the power bar
	var circle = new VectorLine ("BarBackground", new Vector2[50], null, lineWidth, LineType.Continuous, Joins.Weld);
	circle.MakeCircle (position, radius);
	circle.Draw();
	
	// Make the power bar by drawing a 270° arc
	bar = new VectorLine ("TotalBar", new Vector2[segmentCount+1], null, lineWidth-4, LineType.Continuous, Joins.Weld);
	bar.color = Color.black;
	bar.MakeArc (position, radius, radius, 0.0, 270.0);
	bar.Draw();
	
	currentPower = Random.value;
	SetTargetPower();
	// Set the initial bar colors by coloring the segments from the beginning to the current power level
	bar.SetColor (Color.red, 0, Mathf.Lerp (0, segmentCount, currentPower));
}

function SetTargetPower () {
	targetPower = Random.value;
}

function Update () {
	var oldPower = currentPower;
	// Move current power up or down, and choose a new target power when the current power reaches the target
	if (targetPower < currentPower) {
		currentPower -= speed * Time.deltaTime;
		if (currentPower < targetPower) {
			SetTargetPower();
		}
		// When the bar decreases, use SetColor to "erase" the color from the current power to the old power
		bar.SetColor (Color.black, Mathf.Lerp (0, segmentCount, currentPower), Mathf.Lerp (0, segmentCount, oldPower));
	}
	else {
		currentPower += speed * Time.deltaTime;
		if (currentPower > targetPower) {
			SetTargetPower();
		}
		// When the bar increases, use SetColor to color the line segments from the old power to the current power
		bar.SetColor (Color.red, Mathf.Lerp (0, segmentCount, oldPower), Mathf.Lerp (0, segmentCount, currentPower));
	}
}

function OnGUI () {
	GUI.Label (Rect(Screen.width/2 - 40, Screen.height/2 - 15, 80, 30), "Power: " + (currentPower*100).ToString("f0") + "%");
}