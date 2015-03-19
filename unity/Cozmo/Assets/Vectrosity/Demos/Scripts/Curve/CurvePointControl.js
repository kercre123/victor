#pragma strict

var objectNumber : int;
var controlObject : GameObject;
var controlObject2 : GameObject;

function OnMouseDrag () {
	transform.position = DrawCurve.cam.ScreenToViewportPoint (Input.mousePosition);
	DrawCurve.use.UpdateLine (objectNumber, Input.mousePosition, gameObject);
}