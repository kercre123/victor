import Vectrosity;
import System.Collections.Generic;

var segments = 250;
var loop = true;
var cube : Transform;
var speed = .05;

function Start () {
	var splinePoints = new List.<Vector3>();
	var i = 1;
	var obj = GameObject.Find("Sphere"+(i++));
	while (obj != null) {
		splinePoints.Add(obj.transform.position);
		obj = GameObject.Find("Sphere"+(i++));
	}

	var line = new VectorLine("Spline", new Vector3[segments+1], null, 2.0, LineType.Continuous);
	line.MakeSpline (splinePoints.ToArray(), segments, loop);
	line.Draw3D();
	
	// Make the cube "ride" the spline at a constant speed
	do {
		for (var dist = 0.0; dist < 1.0; dist += Time.deltaTime*speed) {
			cube.position = line.GetPoint3D01 (dist);
			yield;
		}
	} while (loop);
}