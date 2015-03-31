using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision2 : CozmoVision
{
	protected SelectionButton2[] selectionButtons;

	protected override void Awake()
	{
		base.Awake();

		selectionButtons = observedObjectCanvas.GetComponentsInChildren<SelectionButton2>( true );
	}

	protected void Update()
	{
		if( RobotEngineManager.instance == null || RobotEngineManager.instance.current == null )
		{
			DisableButtons();
			return;
		}

		robot = RobotEngineManager.instance.current;

		for( int i = 0; i < robot.selectedObjects.Count; ++i )
		{
			if( robot.observedObjects.Find( x => x.ID == robot.selectedObjects[i].ID ) == null )
			{
				robot.selectedObjects.RemoveAt( i-- );
			}
		}

		Dings();
		SetActionButtons();
		ShowObservedObjects();
	}

	protected override void ShowObservedObjects()
	{
		float w = imageRectTrans.sizeDelta.x;
		float h = imageRectTrans.sizeDelta.y;
		
		for( int i = 0; i < maxObservedObjects; ++i )
		{
			if( observedObjectsCount > i && !robot.isBusy )
			{
				ObservedObject observedObject = robot.observedObjects[i];
				
				float boxX = ( observedObject.VizRect.x / 320f ) * w;
				float boxY = ( observedObject.VizRect.y / 240f ) * h;
				float boxW = ( observedObject.VizRect.width / 320f ) * w;
				float boxH = ( observedObject.VizRect.height / 240f ) * h;
				
				selectionButtons[i].image.rectTransform.sizeDelta = new Vector2( boxW, boxH );
				selectionButtons[i].image.rectTransform.anchoredPosition = new Vector2( boxX, -boxY );
				
				if( robot.selectedObjects.Find( x => x.ID == observedObject.ID ) != null )
				{
					selectionButtons[i].image.color = selected;
					selectionButtons[i].text.text = string.Empty;
				}
				else
				{
					selectionButtons[i].image.color = select;
					selectionButtons[i].text.text = "Select " + observedObject.ID;
					selectionButtons[i].observedObject = observedObject;
					
					selectionButtons[i].image.gameObject.SetActive( true );
				}
			}
			else
			{
				selectionButtons[i].image.gameObject.SetActive( false );
			}
		}
	}
}
