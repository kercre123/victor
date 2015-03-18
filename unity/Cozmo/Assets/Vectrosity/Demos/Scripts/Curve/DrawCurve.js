#pragma strict
import Vectrosity;
import System.Collections.Generic;

var lineMaterial : Material;
var dottedLineMaterial : Material;
var segments = 60;

var anchorPoint : GameObject;
var controlPoint : GameObject;

private var numberOfCurves = 1;

private var line : VectorLine;
private var controlLine : VectorLine;

private var pointIndex = 0;
static var use : DrawCurve;
static var cam : Camera;
private var anchorObject : GameObject;
private var oldWidth : int;
private var useDottedLine = false;
private var oldDottedLineSetting = false;
private var oldSegments : int;
private var listPoints = false;

function Start () {
	use = this;	// Reference to this script, so FindObjectOfType etc. are not needed
	cam = Camera.main;
	oldWidth = Screen.width;
	oldSegments = segments;

	// Set up initial curve points (also used for drawing the green lines that connect control points to anchor points)
	var curvePoints = new Vector2[4];
	curvePoints[0] = Vector2(Screen.width*.25, Screen.height*.25);
	curvePoints[1] = Vector2(Screen.width*.125, Screen.height*.5);
	curvePoints[2] = Vector2(Screen.width-Screen.width*.25, Screen.height-Screen.height*.25);
	curvePoints[3] = Vector2(Screen.width-Screen.width*.125, Screen.height*.5);
	
	// Make the control lines
	controlLine = new VectorLine("Control Line", curvePoints, lineMaterial, 2.0);
	controlLine.color = Color(0.0, .75, .1, .6);
	controlLine.Draw();
	
	// Make the line object for the curve
	line = new VectorLine("Curve", new Vector2[segments+1], lineMaterial, 5.0, LineType.Continuous, Joins.Weld);
	
	// Create a curve in the VectorLine object
	line.MakeCurve (curvePoints[0], curvePoints[1], curvePoints[2], curvePoints[3], segments);
	line.Draw();
	
	// Make the GUITexture objects for anchor and control points (two anchor points and two control points)
	AddControlObjects();
	AddControlObjects();
}

function SetLine () {
	if (useDottedLine) {
		line.material = dottedLineMaterial;
		line.lineWidth = 8.0;
		line.textureScale = 1.0;
	}
	else {
		line.lineWidth = 5.0;
		line.material = lineMaterial;
		line.textureScale = 0.0;		
	}
}

function AddControlObjects () {
	anchorObject = Instantiate(anchorPoint, cam.ScreenToViewportPoint(controlLine.points2[pointIndex]), Quaternion.identity);
	anchorObject.GetComponent (CurvePointControl).objectNumber = pointIndex++;
	var controlObject : GameObject = Instantiate(controlPoint, cam.ScreenToViewportPoint(controlLine.points2[pointIndex]), Quaternion.identity);
	controlObject.GetComponent (CurvePointControl).objectNumber = pointIndex++;
	// Make the anchor object have a reference to the control object, so they can move together
	// Having control objects be children of anchor objects would be easier, but parent/child doesn't really work with GUITextures
	anchorObject.GetComponent (CurvePointControl).controlObject = controlObject;
}

function UpdateLine (objectNumber : int, pos : Vector2, go : GameObject) {
	var oldPos = controlLine.points2[objectNumber];	// Get previous position, so we can make the control point move with the anchor point
	controlLine.points2[objectNumber] = pos;
	var curveNumber : int = objectNumber / 4;
	var curveIndex : int = curveNumber * 4;
	line.MakeCurve (controlLine.points2[curveIndex], controlLine.points2[curveIndex+1], controlLine.points2[curveIndex+2], controlLine.points2[curveIndex+3],
					segments, curveNumber * (segments+1));
		
	// If it's an anchor point...
	if (objectNumber % 2 == 0) {
		// Move control point also
		controlLine.points2[objectNumber+1] += pos-oldPos;
		go.GetComponent (CurvePointControl).controlObject.transform.position = cam.ScreenToViewportPoint(controlLine.points2[objectNumber+1]);
		// If it's not an end anchor point, move the next anchor/control points as well, and update the next curve
	 	if (objectNumber > 0 && objectNumber < controlLine.points2.Count-2) {
			controlLine.points2[objectNumber+2] = pos;
			controlLine.points2[objectNumber+3] += pos-oldPos;
			go.GetComponent (CurvePointControl).controlObject2.transform.position = cam.ScreenToViewportPoint(controlLine.points2[objectNumber+3]);
			line.MakeCurve (controlLine.points2[curveIndex+4], controlLine.points2[curveIndex+5], controlLine.points2[curveIndex+6], controlLine.points2[curveIndex+7],
							segments, (curveNumber+1) * (segments+1));
		}
	}
	
	line.Draw();
	controlLine.Draw();	
}

function OnGUI () {
	if (GUI.Button(Rect(20, 20, 100, 30), "Add Point")) {
		AddPoint();
	}
	
	GUI.Label(Rect(20, 59, 200, 30), "Curve resolution: " + segments);
	segments = GUI.HorizontalSlider(Rect(20, 80, 150, 30), segments, 3, 60);
	if (oldSegments != segments) {
		oldSegments = segments;
		ChangeSegments();
	}
	
	useDottedLine = GUI.Toggle(Rect(20, 105, 80, 20), useDottedLine, " Dotted line");
	if (oldDottedLineSetting != useDottedLine) {
		oldDottedLineSetting = useDottedLine;
		SetLine();
		line.Draw();
	}
	
	GUILayout.BeginArea(Rect(20, 150, 150, 800));
	if (GUILayout.Button(listPoints? "Hide points" : "List points", GUILayout.Width(100)) ) {
		listPoints = !listPoints;
	}
	if (listPoints) {
		var idx = 0;
		for (var i = 0; i < controlLine.points2.Count; i += 2) {
			GUILayout.Label("Anchor " + idx + ": (" + parseInt(controlLine.points2[i].x) + ", " + parseInt(controlLine.points2[i].y) + ")");
			GUILayout.Label("Control " + idx++ + ": (" + parseInt(controlLine.points2[i+1].x) + ", " + parseInt(controlLine.points2[i+1].y) + ")");
		}
	}
	GUILayout.EndArea();
}

function AddPoint () {
	// Don't do anything if adding a new point would exceed the max number of vertices per mesh
	if (line.points2.Count + controlLine.points2.Count + segments + 4 > 16383) return;
	
	// Make the first anchor and control points of the new curve be the same as the second anchor/control points of the previous curve
	controlLine.points2.Add (controlLine.points2[pointIndex-2]);
	controlLine.points2.Add (controlLine.points2[pointIndex-1]);
	// Make the second anchor/control points of the new curve be offset a little ways from the first
	var offset = (controlLine.points2[pointIndex-2] - controlLine.points2[pointIndex-4]) * .25;
	controlLine.points2.Add (controlLine.points2[pointIndex-2] + offset);
	controlLine.points2.Add (controlLine.points2[pointIndex-1] + offset);
	// If that made the new anchor point go off the screen, offset them the opposite way
	if (controlLine.points2[pointIndex+2].x > Screen.width || controlLine.points2[pointIndex+2].y > Screen.height ||
			controlLine.points2[pointIndex+2].x < 0 || controlLine.points2[pointIndex+2].y < 0) {
		controlLine.points2[pointIndex+2] = controlLine.points2[pointIndex-2] - offset;
		controlLine.points2[pointIndex+3] = controlLine.points2[pointIndex-1] - offset;
	}
	// For the next control point, make the initial position offset from the anchor point the opposite way as the second control point in the curve
	var controlPointPos = controlLine.points2[pointIndex-1] + (controlLine.points2[pointIndex] - controlLine.points2[pointIndex-1])*2;
	pointIndex++;	// Skip the next anchor point, since we want the second anchor point of one curve and the first anchor point of the next curve
					// to move together (this is handled in UpdateLine)
	controlLine.points2[pointIndex] = controlPointPos;
	// Make another control point
	var controlObject : GameObject = Instantiate (controlPoint, cam.ScreenToViewportPoint (controlPointPos), Quaternion.identity);
	controlObject.GetComponent (CurvePointControl).objectNumber = pointIndex++;
	// For the last anchor object that was made, make a reference to this control point so they can move together
	anchorObject.GetComponent (CurvePointControl).controlObject2 = controlObject;
	// Then make another anchor/control point group
	AddControlObjects();
	
	// Update the control lines
	controlLine.Draw();
	
	// Update the curve with the new points
	line.Resize ((segments+1) * ++numberOfCurves);
	line.MakeCurve (controlLine.points2[pointIndex-4], controlLine.points2[pointIndex-3], controlLine.points2[pointIndex-2], controlLine.points2[pointIndex-1],
					segments, (segments+1) * (numberOfCurves-1));
	line.Draw();
}

function ChangeSegments () {
	// Don't do anything if the requested segments would make the curve exceed the max number of vertices per mesh
	if (segments*4*numberOfCurves > 65534) return;
	
	line.Resize ((segments+1) * numberOfCurves);
	for (var i = 0; i < numberOfCurves; i++) {
		line.MakeCurve (controlLine.points2[i*4], controlLine.points2[i*4+1], controlLine.points2[i*4+2], controlLine.points2[i*4+3], segments, (segments+1)*i);
	}
	line.Draw();
}

function Update () {
	if (Screen.width != oldWidth) {
		oldWidth = Screen.width;
		ChangeResolution();
	}
}

function ChangeResolution () {
	var controlPointObjects = GameObject.FindGameObjectsWithTag("GameController");
	for (obj in controlPointObjects) {
		obj.transform.position = cam.ScreenToViewportPoint(controlLine.points2[obj.GetComponent (CurvePointControl).objectNumber]);
	}
}