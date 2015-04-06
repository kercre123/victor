using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision_SelectObjectExtraButtons : CozmoVision_SelectObject
{	
	protected ObservedObjectButton1[] observedObjectButtons;
	protected List<ObservedObjectBox> unassignedActiveObjectBoxes;
	
	protected override void Awake()
	{
		base.Awake();
		
		unassignedActiveObjectBoxes = new List<ObservedObjectBox>();
		
		observedObjectButtons = observedObjectCanvas.GetComponentsInChildren<ObservedObjectButton1>(true);
		
		ObservedObjectBox[] temp = new ObservedObjectBox[observedObjectBoxes.Count - observedObjectButtons.Length];
		
		for( int i = 0, j = 0; i < observedObjectBoxes.Count; ++i )
		{
			ObservedObjectButton1 button = observedObjectBoxes[i] as ObservedObjectButton1;
			
			if( button == null )
			{
				temp[j++] = observedObjectBoxes[i];
			}
		}

		observedObjectBoxes.Clear();
		observedObjectBoxes.AddRange(temp);
	}

	protected override void Update()
	{
		base.Update();

		if( RobotEngineManager.instance == null || RobotEngineManager.instance.current == null )
		{
			return;
		}

		robot = RobotEngineManager.instance.current;
		
		ConnectButtonsToBoxes();
	}

	protected void ConnectButtonsToBoxes()
	{
		unassignedActiveObjectBoxes.Clear();
		unassignedActiveObjectBoxes.AddRange( observedObjectBoxes.FindAll( x => x.gameObject.activeSelf ) );
		
		for( int i = 0; i < observedObjectButtons.Length; ++i )
		{
			ObservedObjectButton1 button = observedObjectButtons[i];
			button.gameObject.SetActive( pertinentObjects != null && i < pertinentObjects.Count && robot.selectedObjects.Count == 0 && !robot.isBusy );
			
			if( !button.gameObject.activeSelf )
			{
				button.line.points2.Clear();
				button.line.Draw();
				continue;
			}
			
			ObservedObjectBox1 box = null;
			Vector2 buttonPosition = (Vector2)button.position;
			Vector2 boxPosition = Vector2.zero;
			float closest = float.MaxValue;
			
			for( int j = 0; j < unassignedActiveObjectBoxes.Count; ++j )
			{
				ObservedObjectBox1 box1 = unassignedActiveObjectBoxes[j] as ObservedObjectBox1;
				Vector2 box1Position = box1.position;
				float dist = ( buttonPosition - box1Position ).sqrMagnitude;
				
				if( dist < closest )
				{
					closest = dist;
					box = box1;
					boxPosition = box1Position;
				}
			}
			
			if( box == null )
			{
				Debug.LogError( "box shouldn't be null here!" );
				continue;
			}
			
			unassignedActiveObjectBoxes.Remove( box );
			
			//Debug.Log("frame("+Time.frameCount+") box("+box.gameObject.name+") linked to button("+button.gameObject.name+")");
			
			button.observedObject = box.observedObject;
			button.box = box;
			button.SetText(box.text.text);
			button.SetTextColor(box.text.color);
			
			button.line.points2.Clear();
			button.line.points2.Add( buttonPosition );
			button.line.points2.Add( boxPosition );
			button.SetLineColor( box.color );
			button.line.Draw();
		}
	}

	protected override void OnDisable()
	{
		base.OnDisable();
		
		for( int i = 0; i < observedObjectButtons.Length; ++i )
		{ 
			if( observedObjectButtons[i] != null )
			{
				observedObjectButtons[i].gameObject.SetActive( false );
				
				if( observedObjectButtons[i].line != null )
				{
					observedObjectButtons[i].line.points2.Clear();
					observedObjectButtons[i].line.Draw();
				}
			}
		}
	}
	
}