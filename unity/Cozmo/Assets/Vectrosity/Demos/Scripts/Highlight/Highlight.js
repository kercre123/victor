#pragma strict
import Vectrosity;

var lineWidth = 5;
var energyLineWidth = 4;
var selectionSize = .5;
var force = 20.0;
var pointsInEnergyLine = 100;

private var line : VectorLine;
private var energyLine : VectorLine;
private var hit : RaycastHit;
private var selectIndex = 0;
private var energyLevel = 0.0;
private var canClick : boolean;
private var spheres : GameObject[];
private var timer : double = 0.0;
private var ignoreLayer : int;
private var defaultLayer : int;
private var fading = false;

function Start () {
	Time.fixedDeltaTime = .01;
	spheres = new GameObject[GetComponent(MakeSpheres).numberOfSpheres];
	ignoreLayer = LayerMask.NameToLayer("Ignore Raycast");
	defaultLayer = LayerMask.NameToLayer("Default");

	// Set up the two lines
	line = new VectorLine("Line", new List.<Vector2>(), null, lineWidth);
	line.color = Color.green;
	line.capLength = lineWidth*.5;
	energyLine = new VectorLine("Energy", new Vector2[pointsInEnergyLine], null, energyLineWidth, LineType.Continuous);
	SetEnergyLinePoints();
}

function SetEnergyLinePoints () {
	for (var i = 0; i < energyLine.points2.Count; i++) {
		var xPoint = Mathf.Lerp(70, Screen.width-20, (i+0.0)/energyLine.points2.Count);
		energyLine.points2[i] = Vector2(xPoint, Screen.height*.1);
	}
}

function Update () {
	// Don't allow clicking in the left-most 50 pixels (where the slider is), or if the spheres are currently fading
	if (Input.GetMouseButtonDown(0) && Input.mousePosition.x > 50 && !fading) {
		// If neither shift key is down, reset selection
		if (!(Input.GetKey (KeyCode.LeftShift) || Input.GetKey (KeyCode.RightShift)) && selectIndex > 0) {
			ResetSelection (true);
		}
		// See if we clicked on an object (the room is set to the IgnoreRaycast layer, so we can't select it)
		if (Physics.Raycast (Camera.main.ScreenPointToRay (Input.mousePosition), hit)) {
			spheres[selectIndex] = hit.collider.gameObject;
			spheres[selectIndex].layer = ignoreLayer;	// So it can't be clicked again (unless reset)
			selectIndex++;
			line.Resize (selectIndex * 10);
		}
	}
	
	// Draw a square for each selected object
	for (var i = 0; i < selectIndex; i++) {
		// Make the size of the square larger or smaller depending on the object's Z distance from the camera
		var squareSize = (Screen.height * selectionSize) / Camera.main.transform.InverseTransformPoint(spheres[i].transform.position).z;
		var screenPoint = Camera.main.WorldToScreenPoint (spheres[i].transform.position);
		var thisSquare = Rect(screenPoint.x-squareSize, screenPoint.y-squareSize, squareSize*2, squareSize*2);
		line.MakeRect (thisSquare, i*10);
		// Make a line connecting from the midpoint of the square's left edge to the energyLevel slider position
		line.points2[i*10 + 8] = Vector2(thisSquare.x - lineWidth*.25, thisSquare.y + squareSize);
		line.points2[i*10 + 9] = Vector2(35, Mathf.Lerp (65, Screen.height-25, energyLevel));
		// Change color of selected objects
		spheres[i].GetComponent(Renderer).material.SetColor("_Emission", Color(energyLevel, energyLevel, energyLevel));
		spheres[i].GetComponent(Renderer).material.color.a = energyLevel * .25;
	}
}

function FixedUpdate () {
	// Move y position of all points to the left by one
	for (var i = 0; i < energyLine.points2.Count-1; i++) {
		energyLine.points2[i] = new Vector2(energyLine.points2[i].x, energyLine.points2[i+1].y);
	}
	// Calculate new point based on the energy level and time
	timer += Time.deltaTime * Mathf.Lerp (5.0, 20.0, energyLevel);
	energyLine.points2[i] = new Vector2(energyLine.points2[i].x, Screen.height * (.1 + Mathf.Sin(timer) * .08 * energyLevel));
}

function LateUpdate () {
	line.Draw();
	energyLine.Draw();
}

function ResetSelection (instantFade : boolean) {
	// Fade sphere colors back to normal
	if (energyLevel > 0.0) {
		FadeColor(instantFade);
	}
	// Reset the selection index and erase all squares and lines that might have been made
	selectIndex = 0;
	energyLevel = 0.0;
	line.points2.Clear();
	line.Draw();
	// Reset sphere layers so they can be clicked again
	for (sphere in spheres) {
		if (sphere) sphere.layer = defaultLayer;
	}
}

function FadeColor (instantFade : boolean) {
	if (instantFade) {
		// Set all spheres to normal color instantly
		for (var i = 0; i < selectIndex; i++) {
			spheres[i].GetComponent(Renderer).material.SetColor ("_Emission", Color.black);
			spheres[i].GetComponent(Renderer).material.color.a = 0.0;
		}
	}
	else {
		// Do a gradual fade
		fading = true;
		var startColor = Color(energyLevel, energyLevel, energyLevel, 0.0);
		var startAlpha = energyLevel*.25;
		var thisIndex = selectIndex;	// Since selectIndex is set back to 0 this frame
		for (var t = 0.0; t < 1.0; t += Time.deltaTime) {
			for (i = 0; i < thisIndex; i++) {
				spheres[i].GetComponent(Renderer).material.SetColor ("_Emission", Color.Lerp(startColor, Color.black, t));
				spheres[i].GetComponent(Renderer).material.color.a = Mathf.Lerp(startAlpha, 0.0, t);
			}
			yield;
		}
		fading = false;
	}
}

function OnGUI () {
	GUI.Label(Rect(60, 20, 600, 40), "Click to select sphere, shift-click to select multiple spheres\nThen change energy level slider and click Go");
	energyLevel = GUI.VerticalSlider (Rect(30, 20, 10, Screen.height-80), energyLevel, 1.0, 0.0);
	// Prevent energy slider from working if nothing is selected
	if (selectIndex == 0) {
		energyLevel = 0.0;
	}
	if (GUI.Button (Rect(20, Screen.height-40, 32, 20), "Go")) {
		for (var i = 0; i < selectIndex; i++) {
			spheres[i].GetComponent(Rigidbody).AddRelativeForce (Vector3.forward * force * energyLevel, ForceMode.VelocityChange);
		}
		ResetSelection (false);
	}
}