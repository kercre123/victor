using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class CozmoVision1 : CozmoVision
{
	public class DistancePair
	{
		public float distance;
		public ObservedObjectButton1 button;
		public ObservedObjectBox1 box;
	}

	[SerializeField] protected Vector2 lineWidth;
	[SerializeField] protected Color lineColor;

	protected ObservedObjectButton1[] observedObjectButtons;
	protected List<DistancePair> distancePairs = new List<DistancePair>();

	protected override void Awake()
	{
		base.Awake();

		observedObjectButtons = observedObjectCanvas.GetComponentsInChildren<ObservedObjectButton1>(true);

		ObservedObjectBox[] temp = new ObservedObjectBox[observedObjectBoxes.Length - observedObjectButtons.Length];

		for( int i = 0, j = 0; i < observedObjectBoxes.Length; ++i )
		{
			ObservedObjectButton1 button = observedObjectBoxes[i] as ObservedObjectButton1;

			if( button == null )
			{
				temp[j++] = observedObjectBoxes[i];
			}
		}

		observedObjectBoxes = temp;
	}

	protected override void VisionEnabled()
	{
		Color color;
		float alpha = 1f;
		
		color = image.color;
		color.a = alpha;
		image.color = color;
		
		color = select;
		color.a = alpha;
		select = color;
		
		color = selected;
		color.a = alpha;
		selected = color;
	}

	protected void Update()
	{
		if( RobotEngineManager.instance == null || RobotEngineManager.instance.current == null )
		{
			DisableButtons();
			return;
		}

		robot = RobotEngineManager.instance.current;

		Dings();
		SetActionButtons();
		
		distancePairs.Clear();
		
		for( int i = 0; i < maxObservedObjects; ++i )
		{
			if( observedObjectsCount > i && !robot.isBusy )
			{
				ObservedObjectBox1 box = observedObjectBoxes[i] as ObservedObjectBox1;

				box.button = null;
				
				ObservedObjectSeen( box, robot.observedObjects[i] );
				
				if( robot.selectedObjects.Count == 0 )
				{
					for( int j = 0; j < observedObjectButtons.Length && j < observedObjectsCount; ++j )
					{
						DistancePair dp = new DistancePair();
						
						dp.box = box;
						dp.button = observedObjectButtons[j];
						dp.distance = Vector3.Distance( dp.box.image.transform.position, dp.button.transform.position );
						
						distancePairs.Add( dp );
					}
				}
			}
			else
			{
				observedObjectBoxes[i].image.gameObject.SetActive( false );
			}
		}

		distancePairs.Sort( ( dp1, dp2 ) => { return dp1.distance.CompareTo( dp2.distance ); } );
		
		for( int i = 0; i < observedObjectButtons.Length; ++i )
		{
			observedObjectButtons[i].box = null;
			observedObjectButtons[i].gameObject.SetActive( false );
			observedObjectButtons[i].line.SetColor( new Color( 0f, 0f, 0f, 0f ) );
		}
		
		for( int i = 0; i < distancePairs.Count; ++i )
		{
			if( distancePairs[i].button.box == null && distancePairs[i].box.button == null )
			{
				distancePairs[i].button.box = distancePairs[i].box;
				distancePairs[i].box.button = distancePairs[i].button;

				distancePairs[i].button.observedObject = distancePairs[i].box.observedObject;

				distancePairs[i].button.gameObject.SetActive( true );

				distancePairs[i].button.text.text = distancePairs[i].box.text.text;
				distancePairs[i].button.text.color = distancePairs[i].box.text.color;
				distancePairs[i].button.image.color = distancePairs[i].box.image.color;

				distancePairs[i].button.line.points2.Clear();
				Vector3 buttonPosition = distancePairs[i].button.position;
				distancePairs[i].button.line.points2.Add( buttonPosition );
				distancePairs[i].button.line.points2.Add( distancePairs[i].box.Position( buttonPosition ) );
				distancePairs[i].button.line.SetColor( distancePairs[i].button.box.image.color );
				distancePairs[i].button.line.Draw();
			}
		}
	}
}