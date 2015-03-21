#pragma strict
import Vectrosity;

var lineMaterial : Material;
var lineMaterial2 : Material;
var lineMaterial3 : Material;
var frontTex : Texture2D;
var backTex : Texture2D;
var capTex : Texture2D;

function Start () {
	VectorLine.SetEndCap ("arrow", EndCap.Front, lineMaterial, frontTex);
	VectorLine.SetEndCap ("arrow2", EndCap.Both, lineMaterial2, frontTex, backTex);
	VectorLine.SetEndCap ("rounded", EndCap.Mirror, lineMaterial3, capTex);

	var line1 = new VectorLine("Arrow", new Vector2[50], lineMaterial, 30.0, LineType.Continuous, Joins.Weld);
	line1.useViewportCoords = true;
	var splinePoints = [Vector2(.1, .15), Vector2(.3, .5), Vector2(.5, .6), Vector2(.7, .5), Vector2(.9, .15)];
	line1.MakeSpline (splinePoints);
	line1.endCap = "arrow";
	line1.Draw();

	var line2 = new VectorLine("Arrow2", new Vector2[50], lineMaterial2, 40.0, LineType.Continuous, Joins.Weld);
	line2.useViewportCoords = true;
	splinePoints = [Vector2(.1, .85), Vector2(.3, .5), Vector2(.5, .4), Vector2(.7, .5), Vector2(.9, .85)];
	line2.MakeSpline (splinePoints);
	line2.endCap = "arrow2";
	line2.continuousTexture = true;
	line2.Draw();
	
	var line3 = new VectorLine("Rounded", [Vector2(.1, .5), Vector2(.9, .5)], lineMaterial3, 20.0);
	line3.useViewportCoords = true;
	line3.endCap = "rounded";
	line3.Draw();
}