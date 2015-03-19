// This is the same thing as the Simple3D script, except it draws the line in "real" 3D space, so it can be occluded by other 3D objects and so on.
// If the vector object doesn't appear, make sure the scene view isn't visible while in play mode
import Vectrosity;

var lineMaterial : Material;

function Start () {
	// Make a Vector3 array that contains points for a cube that's 1 unit in size
	var cubePoints = [Vector3(-0.5, -0.5, 0.5), Vector3(0.5, -0.5, 0.5), Vector3(-0.5, 0.5, 0.5), Vector3(-0.5, -0.5, 0.5), Vector3(0.5, -0.5, 0.5), Vector3(0.5, 0.5, 0.5), Vector3(0.5, 0.5, 0.5), Vector3(-0.5, 0.5, 0.5), Vector3(-0.5, 0.5, -0.5), Vector3(-0.5, 0.5, 0.5), Vector3(0.5, 0.5, 0.5), Vector3(0.5, 0.5, -0.5), Vector3(0.5, 0.5, -0.5), Vector3(-0.5, 0.5, -0.5), Vector3(-0.5, -0.5, -0.5), Vector3(-0.5, 0.5, -0.5), Vector3(0.5, 0.5, -0.5), Vector3(0.5, -0.5, -0.5), Vector3(0.5, -0.5, -0.5), Vector3(-0.5, -0.5, -0.5), Vector3(-0.5, -0.5, 0.5), Vector3(-0.5, -0.5, -0.5), Vector3(0.5, -0.5, -0.5), Vector3(0.5, -0.5, 0.5)];
	
	// Make a line using the above points and material, with a width of 2 pixels
	var line = new VectorLine("Cube", cubePoints, lineMaterial, 3.0);
	
	// Make this transform have the vector line object that's defined above
	// This object is a rigidbody, so the vector object will do exactly what this object does
	// "false" is added at the end, so that the cube mesh is not replaced by an invisible bounds mesh
	VectorManager.useDraw3D = true;
	VectorManager.ObjectSetup (gameObject, line, Visibility.Dynamic, Brightness.None, false);
}