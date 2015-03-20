// This script animates a partial line segment in a spline
#pragma strict
import Vectrosity;

var lineMaterial : Material;
var segments = 60;
var visibleLineSegments = 20;
var speed = 60.0;
private var startIndex : float;
private var endIndex : float;
private var line : VectorLine;

function Start () {
	startIndex = -visibleLineSegments;
	endIndex = 0;
	
	// Make Vector2 array where the size is the number of segments plus one, since we'll use a continuous line
	var linePoints = new Vector2[segments+1];
	// Make a VectorLine object using the above points and a material as defined in the inspector, with a width of 30 pixels
	line = new VectorLine("Spline", linePoints, lineMaterial, 30.0, LineType.Continuous, Joins.Weld);
	var sw = Screen.width / 5;
	var sh = Screen.height / 3;
	line.MakeSpline ([Vector2(sw, sh), Vector2(sw*2, sh*2), Vector2(sw*3, sh*2), Vector2(sw*4, sh)]);
}

function Update () {
	// Change startIndex and endIndex over time, wrapping around as necessary
	startIndex += Time.deltaTime * speed;
	endIndex += Time.deltaTime * speed;
	if (startIndex >= segments+1) {
		startIndex = -visibleLineSegments;
		endIndex = 0;
	}
	else if (startIndex < -visibleLineSegments) {
		startIndex = segments;
		endIndex = segments + visibleLineSegments;
	}
	line.drawStart = startIndex;
	line.drawEnd = endIndex;
	line.Draw();
}