var rotateSpeed = 10.0;

function Update () {
	transform.Rotate(Vector3.up * Time.deltaTime * rotateSpeed);
}