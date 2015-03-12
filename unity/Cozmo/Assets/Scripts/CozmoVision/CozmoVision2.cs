using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision2 : CozmoVision
{
	[SerializeField] protected SelectionButton2[] selectionButtons;

	protected override void Update()
	{
		base.Update();

		image.gameObject.SetActive( PlayerPrefs.GetInt( "CozmoVision2" ) == 1 );

		if( image.gameObject.activeSelf && RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			DetectNewObservedObjects();
			SetActionButtons();

			for( int i = 0; i < maxObservedObjects; ++i )
			{
				if( observedObjectsCount > i && robot.selectedObject == -1 )
				{
					ObservedObject observedObject = robot.observedObjects[i];

					selectionButtons[i].image.rectTransform.sizeDelta = new Vector2( observedObject.VizRect.width, observedObject.VizRect.height );
					selectionButtons[i].image.rectTransform.anchoredPosition = new Vector2( observedObject.VizRect.x, -observedObject.VizRect.y );

					selectionButtons[i].text.text = "Select " + observedObject.ID;
					selectionButtons[i].ID = observedObject.ID;

					selectionButtons[i].image.gameObject.SetActive( true );
				}
				else
				{
					selectionButtons[i].image.gameObject.SetActive( false );
				}
			}
		}
	}
}
