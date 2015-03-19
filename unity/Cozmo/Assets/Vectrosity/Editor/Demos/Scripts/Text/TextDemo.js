#pragma strict
import Vectrosity;

var text = "Vectrosity!";
var textSize = 40;
private var textLine : VectorLine;

function Start () {
	textLine = new VectorLine("Text", new Vector2[2], null, 1.0);
	textLine.color = Color.yellow;
	textLine.drawTransform = transform;
	textLine.MakeText (text, Vector2(Screen.width/2 - text.Length*textSize/2, Screen.height/2 + textSize/2), textSize);
}

function Update () {
	transform.RotateAround (Vector2(Screen.width/2, Screen.height/2), Vector3.forward, Time.deltaTime * 45.0);
	transform.localScale.x = 1 + Mathf.Sin (Time.time*3)*.3;
	transform.localScale.y = 1 + Mathf.Cos (Time.time*3)*.3;
	textLine.Draw();
}