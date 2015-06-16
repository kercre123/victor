using UnityEngine;
using UnityEngine.UI;

namespace WellFired
{
	/// <summary>
	/// You can add this component to a Unity UI button to hookup Skip Sequence functionality to that button.
	/// </summary>
	[RequireComponent(typeof(Button))]
	public class SkipSequenceButton : MonoBehaviour 
	{
		[SerializeField]
		private USSequencer sequenceToSkip;

		[SerializeField]
		private bool manageInteractiveState = true;
		
		private void Start() 
		{
			var button = GetComponent<Button>();
			
			if(!button)
			{
				Debug.LogError("The component Skip Sequence button must be added to a Unity UI Button");
				return;
			}
			
			if(!sequenceToSkip)
			{
				Debug.LogError("The Sequence to skip field must be hooked up in the Inspector");
				return;
			}
			
			button.onClick.AddListener(() => SkipSequence());
			
			button.interactable = !sequenceToSkip.IsPlaying;
			if(manageInteractiveState)
			{
				sequenceToSkip.OnRunningTimeSet += (sequence) => button.interactable = USRuntimeUtility.CanSkipSequence(sequence);
				sequenceToSkip.PlaybackStarted += (sequence) => button.interactable = true;
				sequenceToSkip.PlaybackPaused += (sequence) => button.interactable = true;
				sequenceToSkip.PlaybackFinished += (sequence) => button.interactable = false;
				sequenceToSkip.PlaybackStopped += (sequence) => button.interactable = true;
			}
		}
		
		private void SkipSequence()
		{
			sequenceToSkip.SkipTimelineTo(sequenceToSkip.Duration);
		}
	}
}