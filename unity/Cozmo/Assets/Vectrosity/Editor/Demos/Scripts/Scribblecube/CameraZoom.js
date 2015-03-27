var zoomSpeed = 10.0;
var keyZoomSpeed = 20.0;

function Update () {
	transform.Translate(Vector3.forward * zoomSpeed * Input.GetAxis("Mouse ScrollWheel"));
	transform.Translate(Vector3.forward * keyZoomSpeed * Time.deltaTime * Input.GetAxis("Vertical"));
}