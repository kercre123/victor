using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision2 : CozmoVision
{
	[SerializeField] protected SelectionButton2[] selectionButtons;

	protected void Update()
	{
		image.gameObject.SetActive( PlayerPrefs.GetInt( "CozmoVision2" ) == 1 );

		if( image.gameObject.activeSelf && RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			for( int i = 0; i < actionButtons.Length; ++i )
			{
				// if no object selected or being actioned
				actionButtons[i].gameObject.SetActive( RobotEngineManager.instance.current.selectedObject < uint.MaxValue - 1 );
			}

			for( int i = 0; i < maxBoxes; ++i )
			{
				if( RobotEngineManager.instance.current.observedObjects.Count > i && RobotEngineManager.instance.current.selectedObject == uint.MaxValue )
				{
					ObservedObject observedObject = RobotEngineManager.instance.current.observedObjects[i];

					selectionButtons[i].image.rectTransform.sizeDelta = new Vector2( observedObject.width, observedObject.height );
					selectionButtons[i].image.rectTransform.anchoredPosition = new Vector2( observedObject.topLeft_x, -observedObject.topLeft_y );

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
