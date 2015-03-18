#pragma strict
import Vectrosity;

var numberOfStars = 2000;
private var stars : VectorPoints;

function Start () {
	// Make a bunch of points in a spherical distribution
	var starPoints = new Vector3[numberOfStars];
	for (var i = 0; i < numberOfStars; i++) {
		starPoints[i] = Random.onUnitSphere * 100.0;
	}
	// Make each star have a size of 1 or 2
	var starSizes = new float[numberOfStars];
	for (i = 0; i < numberOfStars; i++) {
		starSizes[i] = Random.Range(1, 3);
	}
	// Make each star have a random shade of grey
	var starColors = new Color[numberOfStars];
	for (i = 0; i < numberOfStars; i++) {
		var thisValue = Random.value * .75 + .25;
		starColors[i] = Color(thisValue, thisValue, thisValue);
	}
	
	// We want the stars to be drawn behind 3D objects, like a skybox. So we use SetCanvasCamera,
	// which makes the canvas draw with RenderMode.OverlayCamera using Camera.main 
	VectorLine.SetCanvasCamera (Camera.main);
	VectorLine.canvas.planeDistance = Camera.main.farClipPlane-1;
	
	stars = new VectorPoints("Stars", starPoints, null, 1.0);
	stars.SetColors (starColors);
	stars.SetWidths (starSizes);
	
	// Make the drawing order for the orbit lines be on top of the stars
	VectorLine.canvas3D.sortingOrder = 1;
}

function LateUpdate () {
	stars.Draw();
}