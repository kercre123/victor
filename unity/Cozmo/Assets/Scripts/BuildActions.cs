using UnityEngine;
using System.Collections;

public class BuildActions : GameActions {

	public override void Drop( bool onRelease, ObservedObject selectedObject )
	{
		if( !onRelease ) return;

		//GameLayoutTracker.instance
		//run validation prediction
		// if it will fail layout contraints, abort here and throw audio and text about why its a bad drop

		ActionButtonClick();
		base.Drop(onRelease, selectedObject);
	}

	public override void Stack( bool onRelease, ObservedObject selectedObject )
	{
		if( robot == null || !onRelease ) return;

		//run validation prediction
		// if it will fail layout contraints, abort here and throw audio and text about why its a bad drop

		ActionButtonClick();
		base.Stack(onRelease, selectedObject);
	}

}
