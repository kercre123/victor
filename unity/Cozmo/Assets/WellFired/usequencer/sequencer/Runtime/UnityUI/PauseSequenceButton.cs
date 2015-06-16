using UnityEngine;
using UnityEngine.UI;

namespace WellFired
{
	/// <summary>
	/// You can add this component to a Unity UI button to hookup Pause Sequence functionality to that button.
	/// </summary>
	[RequireComponent(typeof(Button))]
	public class PauseSequenceButton : MonoBehaviour 
	{
		[SerializeField]
		private USSequencer sequenceToPause;
		
		[SerializeField]
		private bool manageInteractiveState = true;
		
		private void Start()
		{
			var button = GetComponent<Button>();
			
			if(!button)
			{
				Debug.LogError("The component Pause Sequence button must be added to a Unity UI Button");
				return;
			}
			
			if(!sequenceToPause)
			{
				Debug.LogError("The Sequence to pause field must be hooked up in the Inspector");
				return;
			}
			
			button.onClick.AddListener(() => PauseSequence());

			button.interactable = sequenceToPause.IsPlaying;
			if(manageInteractiveState)
			{
				sequenceToPause.OnRunningTimeSet += (sequence) => button.interactable = USRuntimeUtility.CanPauseSequence(sequence);
				sequenceToPause.PlaybackStarted += (sequence) => button.interactable = true;
				sequenceToPause.PlaybackPaused += (sequence) => button.interactable = false;
				sequenceToPause.PlaybackFinished += (sequence) => button.interactable = false;
				sequenceToPause.PlaybackStopped += (sequence) => button.interactable = false;
			}
		}
		
		private void PauseSequence()
		{
			sequenceToPause.Pause();
		}
	}
}