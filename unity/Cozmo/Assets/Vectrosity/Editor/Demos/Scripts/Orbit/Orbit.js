import Vectrosity;

var orbitSpeed = -45.0;
var rotateSpeed = 200.0;
var orbitLineResolution = 150;

function Start () {
	var orbitLine = new VectorLine("OrbitLine", new Vector3[orbitLineResolution], null, 2.0, LineType.Continuous);
	orbitLine.MakeCircle (Vector3.zero, Vector3.up, Vector3.Distance(transform.position, Vector3.zero));
	orbitLine.Draw3DAuto();
}

function Update () {
	transform.RotateAround (Vector3.zero, Vector3.up, orbitSpeed * Time.deltaTime);
	transform.Rotate (Vector3.up * rotateSpeed * Time.deltaTime);
}