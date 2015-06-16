using UnityEngine;
using System.Collections;
using System;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	[Serializable]
	public class USTimelineAnimationGameRunner : ScriptableObject
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

		[SerializeField]
		private List<AnimationClipData> notRunningClips = new List<AnimationClipData>();
		
		[SerializeField]
		private List<AnimationClipData> runningClips = new List<AnimationClipData>();
		
		[SerializeField]
		private List<AnimationClipData> finishedClips = new List<AnimationClipData>();
		
		[SerializeField]
		private List<AnimationClipData> newProcessingClips = new List<AnimationClipData>();

		public void Stop()
		{
			allClips.Clear();
			notRunningClips.Clear();
			runningClips.Clear();
			finishedClips.Clear();
			newProcessingClips.Clear();
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

			SortClipsAtTime(sequenceTime, notRunningClips, AnimationClipData.IsClipNotRunning);
			SortClipsAtTime(sequenceTime, runningClips, AnimationClipData.IsClipRunning);
			SortClipsAtTime(sequenceTime, finishedClips, AnimationClipData.IsClipFinished);
			
			// We've finished updating our new clips, clear our list for the next Update
			SortNewProcessingClips(sequenceTime);
			
			Animator.speed = playbackRate;

			if(Application.isEditor)
				SanityCheckClipData();
		}
		
		public void PauseTimeline()
		{
			
		}
		
		private void SortNewProcessingClips(float sequenceTime)
		{
			var orderedNewProcessingClips = newProcessingClips.OrderBy(processingClip => processingClip.StartTime);
			
			foreach(var clipData in orderedNewProcessingClips)
			{
				if(AnimationClipData.IsClipNotRunning(sequenceTime, clipData))
					notRunningClips.Add(clipData);
				else if(!AnimationClipData.IsClipNotRunning(sequenceTime, clipData) && notRunningClips.Contains(clipData))
					notRunningClips.Remove(clipData);
				
				if(AnimationClipData.IsClipRunning(sequenceTime, clipData))
				{
					runningClips.Add(clipData);
					PlayClip(clipData, clipData.RunningLayer, sequenceTime);
				}
				else if(!AnimationClipData.IsClipRunning(sequenceTime, clipData) && runningClips.Contains(clipData))
					runningClips.Remove(clipData);
				
				if(AnimationClipData.IsClipFinished(sequenceTime, clipData))
					finishedClips.Add(clipData);
				else if(!AnimationClipData.IsClipFinished(sequenceTime, clipData) && finishedClips.Contains(clipData))
					finishedClips.Remove(clipData);
			}
			
			newProcessingClips.Clear();
		}
		
		private void SortClipsAtTime(float sequenceTime, IEnumerable<AnimationClipData> sortInto, AnimationClipData.StateCheck stateCheck)
		{
			for(int clipIndex = 0; clipIndex < allClips.Count; clipIndex++)
			{
				var clipData = allClips[clipIndex];
				var alreadyContains = sortInto.Contains(clipData);
				
				if(stateCheck(sequenceTime, clipData) && !alreadyContains)
				{
					// This check is here because we might add this twice, when processing two different types of lists in the same frame.
					if(!newProcessingClips.Contains(clipData))
						newProcessingClips.Add(clipData);
				}
				else if(!stateCheck(sequenceTime, clipData) && alreadyContains)
				{
					// This check is here because we might add this twice, when processing two different types of lists in the same frame.
					if(!newProcessingClips.Contains(clipData))
						newProcessingClips.Add(clipData);
				}
			}
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
		
		private void SanityCheckClipData()
		{
			// We assert if we've added the same clip data to more than one list.
			var allClips = AnimationTimeline.AnimationTracks.SelectMany(animationTrack => animationTrack.TrackClips);
			foreach(var clip in allClips)
			{
				bool inNotRunning = notRunningClips.Contains(clip);
				bool inRunning = runningClips.Contains(clip);
				bool inFinished = finishedClips.Contains(clip);
				
				if(notRunningClips.Where(element => element == clip).Count() > 1)
					throw new Exception("Clip is in the same list multiple times, this is an error.");
				if(runningClips.Where(element => element == clip).Count() > 1)
					throw new Exception("Clip is in the same list multiple times, this is an error.");
				if(finishedClips.Where(element => element == clip).Count() > 1)
					throw new Exception("Clip is in the same list multiple times, this is an error.");
				
				if(inNotRunning && !inRunning && !inFinished)
					continue;
				if(!inNotRunning && inRunning && !inFinished)
					continue;
				if(!inNotRunning && !inRunning && inFinished)
					continue;
				if(!inNotRunning && !inRunning && !inFinished)
					continue;
				
				throw new Exception("Clip is in multiple lists, this is an error.");
			}
		}
	}
}