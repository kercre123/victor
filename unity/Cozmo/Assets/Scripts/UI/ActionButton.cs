using UnityEngine;
using UnityEngine.UI;
using UnityEngine.Events;
using System;
using System.Collections;

[System.Serializable]
public class ActionButton : MonoBehaviour
{
	public enum Mode
	{
		DISABLED,
		TARGET,
		PICK_UP,
		DROP,
		STACK,
		ROLL,
		ALIGN,
		CHANGE,
		CANCEL,
		NUM_MODES
	}

	[System.Serializable]
	public struct Hint
	{
		public Image image;
		public Text text;

		private Color _ghost;
		public Color ghost
		{
			get
			{
				if( _ghost == Color.clear && image != null ) _ghost = image.color;
				return _ghost;
			}
		}

		public Color solid
		{
			get
			{ 
				if( _ghost == Color.clear && image != null ) _ghost = image.color;
				return Color.white;
			}
		}
	}

	[SerializeField] private Button button;

	public Hint hint;
	public Image image;
	public Text text;

	private Action<bool, ObservedObject> action;
	private ObservedObject selectedObject;
	private Mode previousMode = Mode.NUM_MODES;
	private string previousAppend = null;
	
	public Mode mode { get; private set; }

	private Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

	private void InvokeActions(bool released, ObservedObject manipulatedObject)
	{
		DefaultAction (released, manipulatedObject);
		if (action != null) {
			action(released, manipulatedObject);
		}
	}

	public void OnRelease()
	{
		InvokeActions (true, selectedObject);
	}
	
	public void OnPress()
	{
		InvokeActions (false, selectedObject);
	}

	public void SetMode( Mode m, ObservedObject selected, string append = null, bool solidHint = false )
	{
		action = null;
		mode = m;
		selectedObject = selected;

		if( robot.isBusy )
		{
			mode = Mode.DISABLED;
		}

		if( mode == Mode.DISABLED || GameActions.instance == null ) 
		{
			if( button != null ) button.gameObject.SetActive( false );
			if( hint.text != null ) hint.text.gameObject.SetActive( false );
			image.gameObject.SetActive( false );
			return;
		}

		GameActions gameActions = GameActions.instance;

		image.sprite = ActionButton.GetModeSprite( mode );

		if (mode != previousMode || previousAppend != append) {
			text.text = ActionButton.GetModeName(mode) + append;
			previousMode = mode;
			previousAppend = append;
		}

		if( hint.image != null )
		{
			if( hint.image.sprite != image.sprite ) hint.image.sprite = image.sprite;

			if( solidHint )
			{
				hint.image.color = hint.solid;
			}
			else
			{
				hint.image.color = hint.ghost;
			}
		}

		if( hint.text != null )
		{
			if( hint.text.text != text.text ) hint.text.text = text.text;

			hint.text.gameObject.SetActive( solidHint );
		}

		switch( mode )
		{
			case Mode.TARGET:
				action = gameActions.Target;
				break;
			case Mode.PICK_UP:
				action = gameActions.PickUp;
				break;
			case Mode.DROP:
				action = gameActions.Drop;
				break;
			case Mode.STACK:
				action = gameActions.Stack;
				break;
			case Mode.ROLL:
				action = gameActions.Roll;
				break;
			case Mode.ALIGN:
				action = gameActions.Align;
				break;
			case Mode.CHANGE:
				action = gameActions.Change;
				break;
			case Mode.CANCEL:
				action = gameActions.Cancel;
				break;
		}

		if( button != null ) button.gameObject.SetActive( true );
		image.gameObject.SetActive( true );
	}
	
	public static Sprite GetModeSprite( Mode mode )
	{
		if( GameActions.instance != null )
		{
			switch( mode )
			{
				case Mode.TARGET: return GameActions.instance.GetActionSprite( mode );
				case Mode.PICK_UP: return GameActions.instance.GetActionSprite( mode );
				case Mode.DROP: return GameActions.instance.GetActionSprite( mode );
				case Mode.STACK: return GameActions.instance.GetActionSprite( mode );
				case Mode.ROLL: return GameActions.instance.GetActionSprite( mode );
				case Mode.ALIGN: return GameActions.instance.GetActionSprite( mode );
				case Mode.CHANGE: return GameActions.instance.GetActionSprite( mode );
				case Mode.CANCEL: return GameActions.instance.GetActionSprite( mode );
			}
		}
		
		return null;
	}
	
	public static string GetModeName( Mode mode )
	{
		if( GameActions.instance != null )
		{
			switch( mode )
			{
				case Mode.TARGET: return GameActions.instance.TARGET;
				case Mode.PICK_UP: return GameActions.instance.PICK_UP;
				case Mode.DROP: return GameActions.instance.DROP;
				case Mode.STACK: return GameActions.instance.STACK;
				case Mode.ROLL: return GameActions.instance.ROLL;
				case Mode.ALIGN: return GameActions.instance.ALIGN;
				case Mode.CHANGE: return GameActions.instance.CHANGE;
				case Mode.CANCEL: return GameActions.instance.CANCEL;
			}
		}
		
		return string.Empty;
	}

	private void DefaultAction( bool onRelease, ObservedObject selectedObject )
	{
		if( onRelease && robot != null )
		{
			robot.searching = false;
			//Debug.Log( "On Release" );
		}
	}
}
