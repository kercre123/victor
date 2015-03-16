var spherePrefab : GameObject;
var numberOfSpheres = 12;
var area = 4.5;

function Start () {
	for (var i = 0; i < numberOfSpheres; i++) {
		Instantiate(spherePrefab, Vector3(Random.Range(-area, area), Random.Range(-area, area), Random.Range(-area, area)), Random.rotation);
	}
}