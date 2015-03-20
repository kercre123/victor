import Vectrosity;
import System.Collections.Generic;

var segments = 250;
var loop = true;
var usePoints = false;

function Start () {
	var splinePoints = new List.<Vector3>();
	var i = 1;
	var obj = GameObject.Find ("Sphere"+(i++));
	while (obj != null) {
		splinePoints.Add (obj.transform.position);
		obj = GameObject.Find ("Sphere"+(i++));
	}

	if (usePoints) {
		var dotLine = new VectorPoints("Spline", new Vector3[segments+1], null, 2.0);
		dotLine.MakeSpline (splinePoints.ToArray(), segments, loop);
		dotLine.Draw();
	}
	else {
		var spline = new VectorLine("Spline", new Vector3[segments+1], null, 2.0, LineType.Continuous);
		spline.MakeSpline (splinePoints.ToArray(), segments, loop);
		spline.Draw3D();
	}
}