using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	[Serializable]
	public partial class ObserverEditor : ScriptableObject, ISelectableContainer
	{
		[SerializeField]
		private List<ObserverRenderData> cachedObserverRenderData;
	
		[SerializeField]
		private USTimelineObserver observerTimeline;
		public USTimelineObserver ObserverTimeline
		{
			get { return observerTimeline; }
			set { observerTimeline = value; }
		}
		
		[SerializeField]
		public Rect DisplayArea
		{
			get;
			private set;
		}
		
		[SerializeField]
		private Rect HeaderArea
		{
			get;
			set;
		}
		
		public float Duration
		{
			get;
			set;
		}
	
		public float XScale
		{
			get;
			set;
		}
	
		public float XScroll
		{
			get;
			set;
		}
		
		public float YScroll
		{
			get;
			set;
		}
		
		private GUIStyle TimelineBackground
		{
			get
			{
				return USEditorUtility.USeqSkin.GetStyle("TimelinePaneBackground");
			}
		}
	
		private void OnEnable()
		{
			hideFlags = HideFlags.HideAndDontSave;
			
			if(cachedObserverRenderData == null)
				cachedObserverRenderData = new List<ObserverRenderData>();
			
			if(SelectedObjects == null)
				SelectedObjects = new List<UnityEngine.Object>();
	
			Undo.undoRedoPerformed -= UndoRedoCallback;
			Undo.undoRedoPerformed += UndoRedoCallback;
			EditorApplication.update -= Update;
			EditorApplication.update += Update;
		}
		
		private void OnDestroy()
		{
			Undo.undoRedoPerformed -= UndoRedoCallback;
			EditorApplication.update -= Update;
		}
	
		private void UndoRedoCallback()
		{
			if(ObserverTimeline)
				ObserverTimeline.OnEditorUndo();
		}
		
		public void InitializeWithKeyframes(IEnumerable<USObserverKeyframe> keyframes)
		{
			foreach(var keyframe in keyframes)
			{
				var cachedData = ScriptableObject.CreateInstance<ObserverRenderData>();
				cachedData.Keyframe = keyframe;
				cachedObserverRenderData.Add(cachedData);
			}
		}
	
		private void Update()
		{

		}
		
		public void OnGUI()
		{
			if(ObserverTimeline != null && ObserverTimeline.Sequence != null && ObserverTimeline.Sequence.HasSequenceBeenStarted)			
				WellFired.Shared.TransitionHelper.ForceGameViewRepaint();

			GUILayout.Box("", TimelineBackground, GUILayout.MaxHeight(17.0f), GUILayout.ExpandWidth(true));
			if(Event.current.type == EventType.Repaint)
				DisplayArea = GUILayoutUtility.GetLastRect();
	
			Rect helperRect = DisplayArea;
			helperRect.y -= 17.0f;

			var validCamerasMessage = string.Empty;
			if(ObserverTimeline.Sequence.RunningTime > 0.0f && Camera.allCameras.Count(camera => ObserverTimeline.IsValidCamera(camera)) > 1)
				validCamerasMessage = " -  You have more than one active camera in your scene, is this intentional?.";

			GUI.Label(helperRect, string.Format("Current Keyframe : {0} {1}", ObserverTimeline.CurrentlyActiveKeyframe != null ? ObserverTimeline.CurrentlyActiveKeyframe.KeyframeCamera.name : "null", validCamerasMessage));
			
			var sortedKeyframes = cachedObserverRenderData.OrderBy(element => element.Keyframe.FireTime).ToList();
			for(int index = 0; index < sortedKeyframes.Count(); index++)
			{
				var data = sortedKeyframes[index];
	
				float xPos = DisplayArea.width * (data.Keyframe.FireTime / Duration);
				float xMaxPos = DisplayArea.width * (data.Keyframe.TransitionDuration / Duration);
				xMaxPos = Mathf.Clamp(xMaxPos, 4.0f, float.MaxValue);

				data.RenderPosition = new Vector2(DisplayArea.x + ((xPos * XScale) - XScroll), DisplayArea.y);
				data.RenderRect = new Rect((DisplayArea.x + ((xPos * XScale) - XScroll)) - 3.0f, DisplayArea.y, xMaxPos, DisplayArea.height - 1.0f);
	
				using(new WellFired.Shared.GUIChangeColor(SelectedObjects.Contains(data.Keyframe) ? Color.yellow : GUI.color))
				{
					GUI.Box(data.RenderRect, "");
	
					Rect labelRect = data.RenderRect;
					labelRect.x += 4.0f;
					labelRect.width = 1000.0f;
					GUI.Label(labelRect, data.Keyframe.KeyframeCamera ? data.Keyframe.KeyframeCamera.name : "Null");
				}
			}
			
			HandleEvent();
		}
		
		private void HandleEvent()
		{
			bool isContext = Event.current.type == EventType.MouseDown && Event.current.button == 1;
			
			if(!isContext)
				return;
			
			GenericMenu contextMenu = new GenericMenu();
	
			float newTime = (((Event.current.mousePosition.x + XScroll - DisplayArea.x) / DisplayArea.width) * Duration) / XScale;
			USObserverKeyframe overKeyframe = null;
			foreach(var data in cachedObserverRenderData)
			{
				if(data.RenderRect.Contains(Event.current.mousePosition))
				{
					contextMenu.AddItem(new GUIContent("Remove Observer Keyframe"), false, () => { RemoveKeyframe(data.Keyframe); });
					contextMenu.AddSeparator("");
					overKeyframe = data.Keyframe;
					break;
				}
			}
			
			Camera[] cameras = Resources.FindObjectsOfTypeAll(typeof(Camera)) as Camera[];
			cameras = cameras.OrderBy(camera => camera.name).ToArray();
			foreach(var camera in cameras)
			{
				if(!ObserverTimeline.IsValidCamera(camera))
					continue;

				var assetPath = AssetDatabase.GetAssetPath(camera.gameObject.transform.root.gameObject);
				if(!string.IsNullOrEmpty(assetPath))
					continue;

				var cutTransition = WellFired.Shared.TypeOfTransition.Cut;
				var cutDuration = WellFired.Shared.TransitionHelper.DefaultTransitionTimeFor(cutTransition);
				contextMenu.AddItem(new GUIContent(String.Format("Set Camera/Cut/{0}", camera.name)), false, (settingCamera) => SetCamera(overKeyframe, newTime, cutTransition, cutDuration, (Camera)settingCamera), camera);

				if(Application.HasProLicense())
				{
					var transitions = Enum.GetValues(typeof(WellFired.Shared.TypeOfTransition)).Cast<WellFired.Shared.TypeOfTransition>();
					foreach(var transition in transitions)
					{
						if(transition == WellFired.Shared.TypeOfTransition.Cut)
							continue;

						var transitionType = transition; // Keep a local copy of this, so it's safed for our delegate.
						var transitionDuration = WellFired.Shared.TransitionHelper.DefaultTransitionTimeFor(transitionType);
						contextMenu.AddItem(new GUIContent(String.Format("Set Camera/Transition/{0}/{1}", camera.name, transitionType)), false, (settingCamera) => SetCamera(overKeyframe, newTime, transitionType, transitionDuration, (Camera)settingCamera), camera);
					}
				}
				else
				{
					contextMenu.AddDisabledItem(new GUIContent(String.Format("Set Camera/Transition (Unity Pro Only)")));
				}
			}
	
			if(DisplayArea.Contains(Event.current.mousePosition))
			{
				Event.current.Use();
				contextMenu.ShowAsContext();
			}
		}
		
		private void RemoveKeyframe(USObserverKeyframe keyframe)
		{
			USUndoManager.RegisterCompleteObjectUndo(this, "Remove Keyframe");
	
			var data = cachedObserverRenderData.Where(element => element.Keyframe == keyframe).First();
			cachedObserverRenderData.Remove(data);
	
			USUndoManager.PropertyChange(ObserverTimeline, "Remove Keyframe");
			ObserverTimeline.RemoveKeyframe(keyframe);
			
			ObserverTimeline.Process(ObserverTimeline.Sequence.RunningTime, ObserverTimeline.Sequence.PlaybackRate);
	
			USUndoManager.DestroyImmediate(data);
			USUndoManager.DestroyImmediate(keyframe);
		}
	
		private void SetCamera(USObserverKeyframe keyframe, float time, WellFired.Shared.TypeOfTransition transitionType, float transitionDuration, Camera camera)
		{
			if(WellFired.Animation.IsInAnimationMode)
			{
				ObserverTimeline.StopTimeline();
				ObserverTimeline.StartTimeline();
			}

			if(keyframe != null)
			{
				USUndoManager.PropertyChange(keyframe, "Set Camera");
				keyframe.KeyframeCamera = camera;
			}
			else
			{
				USUndoManager.RegisterCompleteObjectUndo(this, "Set Camera");
	
				USObserverKeyframe newKeyframe = ScriptableObject.CreateInstance<USObserverKeyframe>();
				USUndoManager.RegisterCreatedObjectUndo(newKeyframe, "Set Camera");
				newKeyframe.FireTime = time;
				newKeyframe.KeyframeCamera = camera;
				newKeyframe.TransitionType = transitionType;
				newKeyframe.TransitionDuration = transitionDuration;
				USUndoManager.PropertyChange(ObserverTimeline, "Set Camera");
				ObserverTimeline.AddKeyframe(newKeyframe);
	
				var cachedData = ScriptableObject.CreateInstance<ObserverRenderData>();
				cachedData.Keyframe = newKeyframe;
				cachedObserverRenderData.Add(cachedData);
			}
	
			ObserverTimeline.Process(ObserverTimeline.Sequence.RunningTime, ObserverTimeline.Sequence.PlaybackRate);
		}
	}
}