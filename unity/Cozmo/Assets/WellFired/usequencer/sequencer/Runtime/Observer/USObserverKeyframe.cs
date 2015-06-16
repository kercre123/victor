
#if WELLFIRED_INTERNAL
#define DEBUG_OBSERVER
#endif

using UnityEngine;
using System;
using System.Collections;

namespace WellFired
{
	[Serializable]
	public class USObserverKeyframe : ScriptableObject 
	{
		#region Member Variables
		public USTimelineObserver observer = null;
		public bool prevActiveState = false;

		private AudioListener cachedListener;
		private WellFired.Shared.BaseTransition transition;

		[SerializeField]
		private WellFired.Shared.TypeOfTransition transitionType = WellFired.Shared.TypeOfTransition.Cut;
		
		[SerializeField]
		private float transitionDuration = 0.0f;
		
		[SerializeField]
		private Camera camera;
		
		[SerializeField]
		public string cameraPath = "";
			
		[SerializeField]
		private float fireTime = 0.0f;
		#endregion
		
		#region Properties
		public bool Fired
		{
			get;
			private set;
		}

		public float FireTime
		{
			set 
			{
				if(observer)
					fireTime = Mathf.Min(Mathf.Max(value, 0.0f), observer.Sequence.Duration); 
				else
					fireTime = value;
			}
			get { return fireTime; }
		}

		public float TransitionDuration 
		{
			get { return (transitionType == WellFired.Shared.TypeOfTransition.Cut ? 0.0f : transitionDuration); }
			set { transitionDuration = value; }
		}
		
		public WellFired.Shared.TypeOfTransition TransitionType 
		{
			get { return transitionType; }
			set { transitionType = value; }
		}
		
		public WellFired.Shared.BaseTransition ActiveTransition 
		{
			get { return transition; }
			set { transition = value; }
		}
		
		public Camera KeyframeCamera
		{
			set
			{
				camera = value;
			}
			get 
			{
				return camera;
			}
		}

		public AudioListener AudioListener
		{
			set { ; }
			get
			{
				if(!KeyframeCamera)
					return default(AudioListener);

				cachedListener = KeyframeCamera.GetComponent<AudioListener>();
				
				return cachedListener;
			}
		}

		public void Fire(Camera previousCamera)
		{
			Fired = true;

			if(transitionType != WellFired.Shared.TypeOfTransition.Cut)
			{
				if(previousCamera != null)
				{
					ActiveTransition = new WellFired.Shared.BaseTransition();
					ActiveTransition.InitializeTransition(previousCamera, KeyframeCamera, transitionType);
				}
				else
				{
					Debug.LogWarning("Cannot use a transition as the first cut in a sequence.");
				}
			}

			// If no transition, instantly enable the camera
			if(ActiveTransition == null)
			{
				if(KeyframeCamera)
					KeyframeCamera.enabled = true;
			}

			// Always enable the listener.
			if(AudioListener)
				AudioListener.enabled = true;
			
#if DEBUG_OBSERVER
			Debug.Log("Fire " + KeyframeCamera);
#endif
		}

		public void UnFire()
		{
			Fired = false;
			
			if(ActiveTransition == null)
			{
				if(KeyframeCamera)
					KeyframeCamera.enabled = false;
			}

#if DEBUG_OBSERVER
			Debug.Log("UnFire " + KeyframeCamera);
#endif
		}
		
		public void End()
		{
			Fired = false;
			
			if(ActiveTransition != null)
				ActiveTransition.TransitionComplete();

			ActiveTransition = null;
			
#if DEBUG_OBSERVER
			Debug.Log("End " + KeyframeCamera);
#endif
		}
		
		public void Revert()
		{
			Fired = false;

			if(ActiveTransition != null)
			{
				ActiveTransition.RevertTransition();
				
				if(KeyframeCamera)
					KeyframeCamera.enabled = false;
				if(AudioListener)
					AudioListener.enabled = false;
			}
			else
			{
				if(KeyframeCamera)
					KeyframeCamera.enabled = false;
				if(AudioListener)
					AudioListener.enabled = false;
			}

			ActiveTransition = null;

#if DEBUG_OBSERVER
			Debug.Log("Revert " + KeyframeCamera);
#endif
		}

		public void ProcessFromOnGUI()
		{
			if(ActiveTransition != null)
				ActiveTransition.ProcessTransitionFromOnGUI();
		}
		
		public void Process(float time)
		{
			if(transitionType == WellFired.Shared.TypeOfTransition.Cut)
				return;

			if(time > TransitionDuration)
				return;

			if(ActiveTransition != null)
				ActiveTransition.ProcessEventFromNoneOnGUI(time, TransitionDuration);

			Fired = true;
			
#if DEBUG_OBSERVER
			Debug.Log("Process " + KeyframeCamera);
#endif
		}
		#endregion
	}
}