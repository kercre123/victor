// Use this method if you need more control than you get with Vector.SetLine
import Vectrosity;

function Start () {
	// Make Vector2 array; in this case we just use 2 elements...
	var linePoints = [Vector2(0, Random.Range(0, Screen.height)),				// ...one on the left side of the screen somewhere
					  Vector2(Screen.width-1, Random.Range(0, Screen.height))];	// ...and one on the right
	
	// Make a VectorLine object using the above points and the default material, with a width of 2 pixels
	var line = new VectorLine("Line", linePoints, null, 2.0);
	
	// Draw the line
	line.Draw();
}