using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using WellFired.Shared;

namespace WellFired
{
	[Serializable]
	public partial class ObjectPathEditor : ScriptableObject, ISelectableContainer
	{
		[SerializeField]
		private List<ObjectPathRenderData> cachedObjectPathRenderData;

		[SerializeField]
		public USTimelineObjectPath ObjectPathTimeline
		{
			get;
			set;
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
			get { return ObjectPathTimeline.Sequence.Duration; }
			private set { ; }
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
		
		public int SelectedNodeIndex
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
	
			if(SelectedObjects == null)
				SelectedObjects = new List<UnityEngine.Object>();

			if(cachedObjectPathRenderData == null)
			{
				cachedObjectPathRenderData = new List<ObjectPathRenderData>();
				cachedObjectPathRenderData.Add(ScriptableObject.CreateInstance<ObjectPathRenderData>());
				cachedObjectPathRenderData.Add(ScriptableObject.CreateInstance<ObjectPathRenderData>());
			}
	
			EditorApplication.update -= Update;
			EditorApplication.update += Update;
		}
		
		private void OnDestroy()
		{
			EditorApplication.update -= Update;
		}
	
		private void Update()
		{
	
		}
	
		public void OnCollapsedGUI()
		{
			if(Event.current.commandName == "UndoRedoPerformed")
			{				
				ObjectPathTimeline.BuildCurveFromKeyframes();
				return;
			}

			if(ObjectPathTimeline.ObjectSpline == null)
				ObjectPathTimeline.BuildCurveFromKeyframes();

			cachedObjectPathRenderData[0].Keyframe = ObjectPathTimeline.FirstNode;
			cachedObjectPathRenderData[1].Keyframe = ObjectPathTimeline.LastNode;

			if(ObjectPathTimeline.ObjectSpline.SplineSolver == null)
				ObjectPathTimeline.BuildCurveFromKeyframes();

			GUILayout.Box("", TimelineBackground, GUILayout.MaxHeight(17.0f), GUILayout.ExpandWidth(true));
			if(Event.current.type == EventType.Repaint)
				DisplayArea = GUILayoutUtility.GetLastRect();

			float startX = ConvertTimeToXPos(ObjectPathTimeline.StartTime);
			float endX = ConvertTimeToXPos(ObjectPathTimeline.EndTime);

			Rect splineAreaBox = new Rect(startX, DisplayArea.y, endX - startX, DisplayArea.height);
			GUI.Box(splineAreaBox, "");

			float width = 2.0f;
			cachedObjectPathRenderData[0].RenderRect = new Rect(startX, DisplayArea.y, width * 2.0f, DisplayArea.height);
			cachedObjectPathRenderData[1].RenderRect = new Rect(endX - (width * 2.0f), DisplayArea.y, width * 2.0f, DisplayArea.height);
			
			cachedObjectPathRenderData[0].RenderPosition = new Vector2(startX , DisplayArea.y);
			cachedObjectPathRenderData[1].RenderPosition = new Vector2(endX, DisplayArea.y);
			
			using(new WellFired.Shared.GUIChangeColor(SelectedObjects.Contains(cachedObjectPathRenderData[0].Keyframe) ? Color.yellow : GUI.color))
				GUI.Box(cachedObjectPathRenderData[0].RenderRect, "");
			using(new WellFired.Shared.GUIChangeColor(SelectedObjects.Contains(cachedObjectPathRenderData[1].Keyframe) ? Color.yellow : GUI.color))
				GUI.Box(cachedObjectPathRenderData[1].RenderRect, "");

			GUI.Label(splineAreaBox, "Double Click on an existing keyframe in the Scene view to add a new one (backspace or delete to remove).");
		}

		public void OnSceneGUI()
		{
			if(ObjectPathTimeline == null || ObjectPathTimeline.Keyframes == null)
				return;

			if(SelectedNodeIndex >= 0)
			{
				if(Event.current.isKey && (Event.current.keyCode == KeyCode.Delete || Event.current.keyCode == KeyCode.Backspace))
				{
					USUndoManager.RegisterCompleteObjectUndo(this, "Remove Keyframe");
					USUndoManager.RegisterCompleteObjectUndo(ObjectPathTimeline, "Remove Keyframe");

					Event.current.Use();
					RemoveKeyframeAtIndex(SelectedNodeIndex);
					SelectedNodeIndex = -1;
				}
			}

			if(Event.current.type == EventType.mouseDown)
			{
				var nearestIndex = GetNearestNodeForMousePosition(Event.current.mousePosition);

				if(nearestIndex != -1)
				{
					SelectedNodeIndex = nearestIndex;
					
					if(Event.current.clickCount > 1)
					{
						USUndoManager.PropertyChange(ObjectPathTimeline, "Add Keyframe");
						
						var cameraTransform = UnityEditor.SceneView.currentDrawingSceneView.camera.transform;
					
						var keyframe = GetKeyframeAtIndex(SelectedNodeIndex);
						var nextKeyframe = GetKeyframeAtIndex(SelectedNodeIndex + 1);
					
						var newKeyframePosition = Vector3.zero;
						if(keyframe == null)
							newKeyframePosition = cameraTransform.position + (cameraTransform.forward * 1.0f);
						else
						{
							if(SelectedNodeIndex == ObjectPathTimeline.Keyframes.Count - 1)
							{
								newKeyframePosition = keyframe.Position + (cameraTransform.up * Vector3.Magnitude(keyframe.Position - cameraTransform.position) * 0.1f);
							}
							else
							{
								var directionVector = nextKeyframe.Position - keyframe.Position;
								var halfDistance = Vector3.Magnitude(directionVector) * 0.5f;
								directionVector.Normalize();
								newKeyframePosition = keyframe.Position + (directionVector * halfDistance);
							}
						}
					
						var translatedKeyframe = ScriptableObject.CreateInstance<SplineKeyframe>();
						USUndoManager.RegisterCreatedObjectUndo(translatedKeyframe, "Add Keyframe");
						translatedKeyframe.Position = newKeyframePosition;
					
						ObjectPathTimeline.AddAfterKeyframe(translatedKeyframe, SelectedNodeIndex);
						GUI.changed = true;
					}
				}
			}

			if(Vector3.Distance(ObjectPathTimeline.Keyframes[0].Position, ObjectPathTimeline.Keyframes[ObjectPathTimeline.Keyframes.Count - 1].Position) == 0)
			{
				Handles.Label(ObjectPathTimeline.Keyframes[0].Position, "Start and End");
			}
			else
			{
				Handles.Label(ObjectPathTimeline.Keyframes[0].Position, "Start");
				Handles.Label(ObjectPathTimeline.Keyframes[ObjectPathTimeline.Keyframes.Count - 1].Position, "End");
			}

			for(int nodeIndex = 0; nodeIndex < ObjectPathTimeline.Keyframes.Count; nodeIndex++)
			{
				var node = ObjectPathTimeline.Keyframes[nodeIndex];

				if(node && nodeIndex > 0 && nodeIndex < ObjectPathTimeline.Keyframes.Count - 1)
				{
					float handleSize = HandlesUtility.GetHandleSize(node.Position);
					Handles.Label(node.Position + new Vector3(0.25f * handleSize, 0.0f * handleSize, 0.0f * handleSize), nodeIndex.ToString());
				}

				using(new HandlesChangeColor(ObjectPathTimeline.PathColor))
				{
					USUndoManager.BeginChangeCheck();

					var existingKeyframe = ObjectPathTimeline.Keyframes[nodeIndex];
					Vector3 newPosition = HandlesUtility.PositionHandle(existingKeyframe.Position, Quaternion.identity);

					if(USUndoManager.EndChangeCheck())
					{
						USUndoManager.PropertyChange(ObjectPathTimeline, "Modify Keyframe");

						foreach(var keyframe in ObjectPathTimeline.Keyframes)	
							USUndoManager.PropertyChange(keyframe, "Modify Keyframe");

						ObjectPathTimeline.AlterKeyframe(newPosition, nodeIndex);
						EditorUtility.SetDirty(ObjectPathTimeline);
					}
				}
			}
		}

		private int GetNearestNodeForMousePosition(Vector3 mousePos)
		{
			var bestDistance = float.MaxValue;
			var index = -1;
			var distance = float.MaxValue;
			for(var i = ObjectPathTimeline.Keyframes.Count - 1; i >= 0; i--)
			{
				var nodeToGui = HandleUtility.WorldToGUIPoint(ObjectPathTimeline.Keyframes[i].Position);
				distance = Vector2.Distance(nodeToGui, mousePos);
				
				if(distance < bestDistance)
				{
					bestDistance = distance;
					index = i;
				}
			}	

			if(bestDistance < 26)
				return index;

			return -1;
		}

		private void RemoveKeyframeAtIndex(int index)
		{
			if(index >= ObjectPathTimeline.Keyframes.Count || index < 0)
				return;

			var keyframe = ObjectPathTimeline.Keyframes[index];
			ObjectPathTimeline.RemoveKeyframe(keyframe);
			GUI.changed = true;

			USUndoManager.DestroyImmediate(keyframe);
		}

		private SplineKeyframe GetKeyframeAtIndex(int index)
		{
			if(index >= ObjectPathTimeline.Keyframes.Count || index < 0)
				return null;
			
			return ObjectPathTimeline.Keyframes[index];
		}
		
		protected float ConvertTimeToXPos(float time)
		{
			float xPos = DisplayArea.width * (time / ObjectPathTimeline.Sequence.Duration);
			return DisplayArea.x + ((xPos * XScale) - XScroll);
		}
	}
}