using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ObservedObjectButton : ObservedObjectBox
{
	[SerializeField] protected AudioClip select;

	public virtual void Selection()
	{
		if( RobotEngineManager.instance != null && RobotEngineManager.instance.current != null )
		{
			RobotEngineManager.instance.current.selectedObjects.Clear();
			RobotEngineManager.instance.current.selectedObjects.Add(observedObject);
			RobotEngineManager.instance.current.TrackHeadToObject(observedObject);
		}

		if( audio != null ) audio.PlayOneShot( select );
	}
}
