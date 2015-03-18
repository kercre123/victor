var rotateSpeed = 5.0;

function Update () {
	transform.RotateAround (Vector3.zero, Vector3.right, rotateSpeed * Time.deltaTime);
}