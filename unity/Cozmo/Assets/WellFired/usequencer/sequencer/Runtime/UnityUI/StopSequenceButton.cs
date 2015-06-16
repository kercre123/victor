using UnityEngine;
using UnityEngine.UI;

namespace WellFired
{
	/// <summary>
	/// You can add this component to a Unity UI button to hookup Stop Sequence functionality to that button.
	/// </summary>
	[RequireComponent(typeof(Button))]
	public class StopSequenceButton : MonoBehaviour 
	{
		[SerializeField]
		private USSequencer sequenceToStop;
		
		[SerializeField]
		private bool manageInteractiveState = true;
		
		private void Start() 
		{
			var button = GetComponent<Button>();
			
			if(!button)
			{
				Debug.LogError("The component Stop Sequence button must be added to a Unity UI Button");
				return;
			}
			
			if(!sequenceToStop)
			{
				Debug.LogError("The Sequence to stop field must be hooked up in the Inspector");
				return;
			}
			
			button.onClick.AddListener(() => StopSequence());
			
			button.interactable = sequenceToStop.IsPlaying;
			if(manageInteractiveState)
			{
				sequenceToStop.OnRunningTimeSet += (sequence) => button.interactable = USRuntimeUtility.CanStopSequence(sequence);
				sequenceToStop.PlaybackStarted += (sequence) => button.interactable = true;
				sequenceToStop.PlaybackPaused += (sequence) => button.interactable = true;
				sequenceToStop.PlaybackFinished += (sequence) => button.interactable = true;
				sequenceToStop.PlaybackStopped += (sequence) => button.interactable = false;
			}
		}
		
		private void StopSequence()
		{
			sequenceToStop.Stop();
		}
	}
}