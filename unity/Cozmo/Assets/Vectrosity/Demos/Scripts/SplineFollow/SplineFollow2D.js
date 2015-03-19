import Vectrosity;
import System.Collections.Generic;

var segments = 250;
var loop = true;
var cube : Transform;
var speed = .05;

function Start () {
	var splinePoints = new List.<Vector2>();
	var i = 1;
	var obj = GameObject.Find("Sphere"+(i++));
	while (obj != null) {
		splinePoints.Add(Camera.main.WorldToScreenPoint(obj.transform.position));
		obj = GameObject.Find("Sphere"+(i++));
	}

	var line = new VectorLine("Spline", new Vector2[segments+1], null, 2.0, LineType.Continuous);
	line.MakeSpline (splinePoints.ToArray(), segments, loop);
	line.Draw();
	
	// Make the cube "ride" the spline at a constant speed
	do {
		for (var dist = 0.0; dist < 1.0; dist += Time.deltaTime*speed) {
			var splinePoint = line.GetPoint01 (dist);
			cube.position = Camera.main.ScreenToWorldPoint (Vector3(splinePoint.x, splinePoint.y, 10.0));
			yield;
		}
	} while (loop);
}