#pragma strict
// This script is put on a plane using a depthmask shader. There are two cameras: the main camera on top that sees all layers except the UI layer,
// and a vector camera underneath that sees only the UI layer. By moving and resizing this depthmask plane, a "window" into
// the vector cam can be made. Since the vector objects are synced to the normal objects and the two cams are in the same position,
// an x-ray like effect is created.

var moveSpeed = 1.0;
var explodePower = 20.0;
var vectorCam : Camera;
private var mouseDown = false;
private var rigidbodies : Rigidbody[];
private var canClick = true;
private var boxDrawn = false;

function Start () {
	GetComponent(Renderer).enabled = false;
	rigidbodies = FindObjectsOfType (Rigidbody);
	VectorLine.SetCanvasCamera (vectorCam);
	VectorLine.canvas.planeDistance = .5;
}

function Update () {
	var mousePos = Input.mousePosition;
	mousePos.z = 1.0;
	var worldPos = Camera.main.ScreenToWorldPoint (mousePos);
	
	if (Input.GetMouseButtonDown(0) && canClick) {
		GetComponent(Renderer).enabled = true;
		transform.position = worldPos;
		mouseDown = true;
	}
	
	if (mouseDown) {
		transform.localScale = Vector3(worldPos.x - transform.position.x, worldPos.y - transform.position.y, 1.0);
	}
	
	if (Input.GetMouseButtonUp(0)) {
		mouseDown = false;
		boxDrawn = true;
	}
	
	transform.Translate (Vector3.up * Time.deltaTime * moveSpeed * Input.GetAxis ("Vertical"));
	transform.Translate (Vector3.right * Time.deltaTime * moveSpeed * Input.GetAxis ("Horizontal"));
}

function OnGUI () {
	GUI.Box (Rect(20, 20, 320, 38), "Draw a box by clicking and dragging with the mouse\nMove the drawn box with the arrow keys");
	var buttonRect = Rect(20, 62, 60, 30);
	// Prevent mouse button click in Update from working if mouse is over the "boom" button
	canClick = (buttonRect.Contains (Event.current.mousePosition)? false : true);
	// The "boom" button is only drawn after a box is made, so users don't get distracted by the physics first ;)
	if (boxDrawn && GUI.Button (buttonRect, "Boom!")) {
		for (body in rigidbodies) {
			body.AddExplosionForce (explodePower, Vector3(0.0, -6.5, -1.5), 20.0, 0.0, ForceMode.VelocityChange);
		}
	}
}