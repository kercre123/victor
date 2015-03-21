// The DrawLinesTouch script adapted to work with mouse input, with the option for 3D or 2D lines
#pragma strict
import Vectrosity;
import System.Collections.Generic;

var lineMaterial : Material;
var maxPoints = 5000;
var lineWidth = 4.0;
var minPixelMove = 5;	// Must move at least this many pixels per sample for a new segment to be recorded
var useEndCap = false;
var capTex : Texture2D;
var capMaterial : Material;
// If line3D is true, the line is drawn in the scene rather than as an overlay. Note that in this demo, the line will look the same
// in the game view either way, but you can see the difference in the scene view.
var line3D = false;
var distanceFromCamera = 1.0;

private var line : VectorLine;
private var previousPosition : Vector2;
private var sqrMinPixelMove : int;
private var canDraw = false;

function Start () {
	if (useEndCap) {
		VectorLine.SetEndCap ("cap", EndCap.Mirror, capMaterial, capTex);
		lineMaterial = capMaterial;
	}
	
	if (line3D) {
		line = new VectorLine("DrawnLine", new List.<Vector3>(), lineMaterial, lineWidth, LineType.Continuous, Joins.Weld);
	}
	else {
		line = new VectorLine("DrawnLine", new List.<Vector2>(), lineMaterial, lineWidth, LineType.Continuous, Joins.Weld);		
	}
	line.endPointsUpdate = 1;	// Optimization for updating only the last point of the line, and the rest is not re-computed
	if (useEndCap) {
		line.endCap = "cap";
	}
	// Used for .sqrMagnitude, which is faster than .magnitude
	sqrMinPixelMove = minPixelMove*minPixelMove;
}

function Update () {
	var newPoint = GetMousePos();
	// Mouse button clicked, so start a new line
	if (Input.GetMouseButtonDown(0)) {
		if (line3D) {
			line.points3.Clear();
		}
		else {
			line.points2.Clear();
		}
		line.Draw();
		previousPosition = Input.mousePosition;
		if (line3D) {
			line.points3.Add (newPoint);
		}
		else {
			line.points2.Add (newPoint);
		}
		canDraw = true;
	}
	// Mouse button held down and mouse has moved far enough to make a new point
	else if (Input.GetMouseButton(0) && (Input.mousePosition - previousPosition).sqrMagnitude > sqrMinPixelMove && canDraw) {
		previousPosition = Input.mousePosition;
		if (line3D) {
			line.points3.Add (newPoint);
			var pointCount = line.points3.Count;
			line.Draw3D();
		}
		else {
			line.points2.Add (newPoint);
			pointCount = line.points2.Count;
			line.Draw();
		}
		if (pointCount >= maxPoints) {
			canDraw = false;
		}
	}
}

function GetMousePos () : Vector3 {
	var p = Input.mousePosition;
	if (line3D) {
		p.z = distanceFromCamera;
		return Camera.main.ScreenToWorldPoint (p);
	}
	return p;
}