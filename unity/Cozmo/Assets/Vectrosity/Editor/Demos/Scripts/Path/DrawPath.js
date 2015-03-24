// Makes a textured path that follows a 3D object
#pragma strict
import Vectrosity;

var lineMaterial : Material;
var maxPoints = 500;
var continuousUpdate = true;
var ballPrefab : GameObject;
var force = 16.0;

private var pathLine : VectorLine;
private var pathIndex = 0;
private var ball : GameObject;

function Start () {
	pathLine = new VectorLine("Path", new List.<Vector3>(), lineMaterial, 12.0, LineType.Continuous);
	pathLine.textureScale = 1.0;
	
	MakeBall();
	SamplePoints (ball.transform);
}

function MakeBall () {
	if (ball) {
		Destroy (ball);
	}
	ball = Instantiate (ballPrefab, Vector3(-2.25, -4.4, -1.9), Quaternion.Euler(300.0, 70.0, 310.0));
	ball.GetComponent(Rigidbody).useGravity = true;
	ball.GetComponent(Rigidbody).AddForce (ball.transform.forward * force, ForceMode.Impulse);
}

function SamplePoints (thisTransform : Transform) {
	// Gets the position of the 3D object at intervals (20 times/second)
	var running = true;
	while (running) {
		pathLine.points3.Add (thisTransform.position);
		if (++pathIndex == maxPoints) {
			running = false;
		}
		yield WaitForSeconds (.05);
		
		if (continuousUpdate) {
			pathLine.Draw();
		}
	}
}

function OnGUI () {
	if (GUI.Button (Rect(10, 10, 100, 30), "Reset")) {
		Reset();
	}
	if (!continuousUpdate && GUI.Button (Rect(10, 45, 100, 30), "Draw Path")) {
		pathLine.Draw();
	}
}

function Reset () {
	StopAllCoroutines();
	MakeBall();
	pathLine.points3.Clear();
	pathLine.Draw();	// Re-draw the cleared line in order to erase all previously drawn segments
	pathIndex = 0;
	SamplePoints (ball.transform);
}