// This script draws a number of ellipses using a single discrete line
import Vectrosity;

var lineMaterial : Material;
var segments = 60;
var numberOfEllipses = 10;

function Start () {
	// Make Vector2 array where the size is twice the number of segments (since it's a discrete line where each segment needs two points),
	// multiplied by the total number to be drawn
	var linePoints = new Vector2[(segments*2) * numberOfEllipses];
	
	// Make a VectorLine object using the above points and a material as defined in the inspector, with a width of 3 pixels
	var line = new VectorLine("Line", linePoints, lineMaterial, 3.0);
	// Create the ellipses in the VectorLine object, where the origin is random, and the radii are random
	for (var i = 0; i < numberOfEllipses; i++) {
		var origin = Vector2(Random.Range(0, Screen.width), Random.Range(0, Screen.height));
		line.MakeEllipse (origin, Random.Range(10, Screen.width/2), Random.Range(10, Screen.height/2), segments, i*(segments*2));
	}
	
	// Draw the line
	line.Draw();
}