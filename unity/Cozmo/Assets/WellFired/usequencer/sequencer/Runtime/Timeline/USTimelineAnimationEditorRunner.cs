using UnityEngine;
using System.Collections;
using System;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	[Serializable]
	public class USTimelineAnimationEditorRunner : ScriptableObject
	{
		[SerializeField]
		private Animator animator;

		public Animator Animator
		{
			private get { return animator; }
			set { animator = value; }
		}
		
		[SerializeField]
		private USTimelineAnimation animationTimeline;
		public USTimelineAnimation AnimationTimeline
		{
			private get { return animationTimeline; }
			set { animationTimeline = value; }
		}
		
		[SerializeField]
		private List<AnimationClipData> allClips = new List<AnimationClipData>();
	
		private float previousTime = 0.0f;
	
		public void Stop()
		{
			previousTime = 0.0f;
		}
		
		public void Process(float sequenceTime, float playbackRate)
		{
			allClips.Clear();
			foreach(var track in AnimationTimeline.AnimationTracks)
			{
				allClips.AddRange(track.TrackClips);
				foreach(var trackClip in track.TrackClips)
				{
					trackClip.RunningLayer = track.Layer;
				}
			}

			var totalDeltaTime = sequenceTime - previousTime;
			var absDeltaTime = Mathf.Abs(totalDeltaTime);
			var timelinePlayingInReverse = totalDeltaTime < 0.0f;
			var runningTime = USSequencer.SequenceUpdateRate;
			var runningTotalTime = previousTime + runningTime;

			if(timelinePlayingInReverse)
			{
				AnimationTimeline.ResetAnimation();
				previousTime = 0.0f;
				AnimationTimeline.Process(sequenceTime, playbackRate);
			}
			else
			{
				while(absDeltaTime > 0.0f)
				{	
					// Work out which clips are playing and call .Play OR .Crossfade on them.
					var runningClips = allClips.Where(clip => AnimationClipData.IsClipRunning(runningTotalTime, clip)).OrderBy(clip => clip.StartTime);
				
					foreach(var clip in runningClips)
						PlayClip(clip, clip.RunningLayer, runningTotalTime);
				
					Animator.Update(runningTime);
					
					absDeltaTime -= USSequencer.SequenceUpdateRate;
					if(!Mathf.Approximately(absDeltaTime, Mathf.Epsilon) && absDeltaTime < USSequencer.SequenceUpdateRate)
						runningTime = absDeltaTime;
					
					runningTotalTime += runningTime;
				}
			}

			previousTime = sequenceTime;
		}
		
		public void PauseTimeline()
		{
			Animator.enabled = false;
		}
		
		private void PlayClip(AnimationClipData clipToPlay, int layer, float sequenceTime)
		{
			float normalizedTime = (sequenceTime - clipToPlay.StartTime) / clipToPlay.StateDuration;
			
			if(clipToPlay.CrossFade)
			{
				// The calculation and clamp are here, to resolve issues with big timesteps.
				// crossFadeTime will not always be equal to clipToPlay.transitionDuration, for insance
				// if the timeStep allows for a step of 0.5, we'll be 0.5s into the crossfade.
				var crossFadeTime = clipToPlay.TransitionDuration - (sequenceTime - clipToPlay.StartTime);
				crossFadeTime = Mathf.Clamp(crossFadeTime, 0.0f, Mathf.Infinity);
				
				Animator.CrossFade(clipToPlay.StateName, crossFadeTime, layer, normalizedTime);
			}
			else
				Animator.Play(clipToPlay.StateName, layer, normalizedTime);
			
			//var message = string.Format("state: {0}, nt: {1}, cf: {2}", clipToPlay.StateName, normalizedTime, clipToPlay.CrossFade);
			//Debug.LogError(message);
		}
	}
}