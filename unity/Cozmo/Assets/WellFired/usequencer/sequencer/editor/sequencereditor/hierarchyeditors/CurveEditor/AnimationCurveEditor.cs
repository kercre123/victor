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
	public partial class AnimationCurveEditor : ScriptableObject, ISelectableContainer
	{
		private Dictionary<USInternalKeyframe, AnimationKeyframeRenderData> cachedKeyframePositions = new Dictionary<USInternalKeyframe, AnimationKeyframeRenderData>();
		
		[SerializeField]
		private EditorWindow editorWindow;
		public EditorWindow EditorWindow
		{
			get { return editorWindow; }
			set { editorWindow = value; }
		}
		
		[SerializeField]
		private float currentXMarkerDist;
		public float CurrentXMarkerDist
		{
			get { return currentXMarkerDist; }
			set { currentXMarkerDist = value; }
		}
	
		public bool RebuildCurvesOnNextGUI
		{
			get;
			set;
		}
		
		[SerializeField]
		public bool AreCurvesDirty
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
		public float MaxHeight
		{
			get;
			set;
		}
		
		[SerializeField]
		private Vector2 Min
		{
			get;
			set;
		}
		
		[SerializeField]
		private Vector2 Max
		{
			get;
			set;
		}
		
		[SerializeField]
		private int MaxDisplayY
		{
			get;
			set;
		}
		
		[SerializeField]
		private int MinDisplayY
		{
			get;
			set;
		}
	
		[SerializeField]
		private USInternalCurve SelectedCurve
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
		
		[SerializeField]
		private int selectedTangent = -1;
		private int SelectedTangent
		{
			get { return selectedTangent; }
			set { selectedTangent = value; }
		}

		public static CurveAutoTangentModes AutoTangentMode
		{
			get;
			set;
		}
	
		public float Duration
		{
			get;
			set;
		}
	
		[SerializeField]
		private List<USInternalCurve> _curves;
		public List<USInternalCurve> Curves
		{
			get { return _curves; }
			set
			{
				_curves = value;
				CalculateBounds();
				RebuildCachedCurveInformation();
			}
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
	
			MaxHeight = Mathf.Infinity;
	
			if(SelectedObjects == null)
				SelectedObjects = new List<UnityEngine.Object>();
	
			Undo.undoRedoPerformed -= UndoRedoCallback;
			Undo.undoRedoPerformed += UndoRedoCallback;
		}
		
		private void OnDestroy()
		{
			Undo.undoRedoPerformed -= UndoRedoCallback;
		}
	
		private void UndoRedoCallback()
		{
			RebuildCurvesOnNextGUI = true;
			AreCurvesDirty = true;

			if(Curves != null)
			{
				// We must rebuild our Animation Curves here, this needs to happen on Undo / Redo
				foreach(var curve in Curves)
					curve.BuildAnimationCurveFromInternalCurve();
			}
		}
	
		public void OnCollapsedGUI()
		{
			GUILayout.Box("", TimelineBackground, GUILayout.ExpandWidth(true), GUILayout.ExpandHeight(true), GUILayout.MaxHeight(MaxHeight));
			if(Event.current.type == EventType.Repaint)
				DisplayArea = GUILayoutUtility.GetLastRect();
		}
		
		public void OnGUI()
		{
			if(Event.current.commandName == "UndoRedoPerformed")
				return;
	
			if(Curves == null || Curves.Count == 0)
			{
				GUILayout.Box("", TimelineBackground, GUILayout.ExpandWidth(true), GUILayout.ExpandHeight(true), GUILayout.MaxHeight(MaxHeight));
				if(Event.current.type == EventType.Repaint)
					DisplayArea = GUILayoutUtility.GetLastRect();
				GUI.Label(DisplayArea, "Select or Add a curve for this Property Timeline to start editting");
				return;
			}
	
			if(Curves.Any(curve => curve == null))
				throw new NullReferenceException("One of the curves is null, this shouldn't happen");
	
			if(EditorWindow == null)
				throw new NullReferenceException("Editor Window must be assigned a value");
			
			if(RebuildCurvesOnNextGUI)
			{
				CalculateBounds();
				RebuildCachedCurveInformation();
				
				// Sometimes, depending on the update order vs the serialization order, this can be null during Undo / Redo :/
				if(EditorWindow != null)
					EditorWindow.Repaint();
			}
			RebuildCurvesOnNextGUI = false;
	
			foreach(var cachedRenderingData in cachedKeyframePositions)
			{
				if(SelectedObjects.Contains(cachedRenderingData.Key))
					cachedRenderingData.Value.IsKeyframeSelected = true;
				else
					cachedRenderingData.Value.IsKeyframeSelected = false;
			}
	
			if(cachedKeyframePositions == null || (cachedKeyframePositions.Count == 0 && Curves.Count > 0))
				RebuildCachedCurveInformation();
			
			GUILayout.Box("", TimelineBackground, GUILayout.ExpandWidth(true), GUILayout.ExpandHeight(true), GUILayout.MaxHeight(MaxHeight));
	
			if(Event.current.type == EventType.Repaint)
			{
				Rect displayAreaRect = GUILayoutUtility.GetLastRect();
				Rect previousDisplayArea = DisplayArea;

				DisplayArea = displayAreaRect;
				
				if(DisplayArea != previousDisplayArea)
					RebuildCachedCurveInformation();
			}
	
			using(new WellFired.Shared.GUIBeginArea(DisplayArea))
			{
				if(Event.current.type == EventType.Repaint)
					DrawGrid();
	
				for(int n = 0; n < Curves.Count; n++)
					DrawCurve(Curves[n], AnimationCurveEditorUtility.GetCurveColor(n));
	
				HandleEvent();
			}
		}
		
		void CalculateBounds()
		{	
			Max = new Vector2(-Mathf.Infinity, -Mathf.Infinity);
			Min = new Vector2(Mathf.Infinity, Mathf.Infinity);
			
			for(int curveIndex = 0; curveIndex < Curves.Count; curveIndex++)
			{	
				var curve = Curves[curveIndex];
				for(int keyframeIndex = 0; keyframeIndex < curve.Keys.Count; keyframeIndex++)
				{
					var keyframe = curve.Keys[keyframeIndex];
					Min = new Vector2(Mathf.Min(Min.x, keyframe.Time), Mathf.Min(Min.y, keyframe.Value));
					Max = new Vector2(Mathf.Max(Max.x, keyframe.Time), Mathf.Max(Max.y, keyframe.Value));
				}
			}
	
			float difference = Mathf.Abs(Max.y - Min.y);
			MaxDisplayY = Mathf.CeilToInt(Max.y + (difference * 0.1f));
			MinDisplayY = Mathf.FloorToInt(Min.y - (difference * 0.1f));
			
			if(MaxDisplayY == MinDisplayY)
			{
				MaxDisplayY += 1;
				MinDisplayY -= 1;
			}
		}
		
		void DrawGrid()
		{
			float totalAbs = Mathf.Abs(MaxDisplayY - MinDisplayY);
			float yStep = totalAbs / (AnimationCurveEditorUtility.NUM_LINES - 1);
	
			int maxHorizontalLines = 5;
			int totalHorizontalLines = maxHorizontalLines * 2 + 1;
			float yPixelStep = DisplayArea.height / (totalHorizontalLines - 1);
	
			using(new WellFired.Shared.GUIChangeColor(AnimationCurveEditorUtility.LABEL_COLOR))
			{
				int xStep = 0;
				float runningTotal = XScroll * -1.0f;
				while(runningTotal <= DisplayArea.width)
				{
					if(runningTotal >= 0)
					{
						Vector2 startPos = new Vector2(runningTotal, 0.0f);
						Vector2 endPos = new Vector2(runningTotal, DisplayArea.height);
	
						using(new HandlesChangeColor((xStep % AnimationCurveEditorUtility.BOLD_STEP == 0) ? AnimationCurveEditorUtility.BOLD_GRID : AnimationCurveEditorUtility.LIGHT_GRID))
							AnimationCurveEditorGUIUtility.DrawLine(startPos, endPos);
					}
	
					xStep++;
					runningTotal += CurrentXMarkerDist;

					if(xStep > 200)
						break;
				}
				
				for(int y = 0; y <= AnimationCurveEditorUtility.NUM_LINES; y++)
				{
					Vector2 startPos = new Vector2(0.0f, y * yPixelStep);
					Vector2 endPos = new Vector2(DisplayArea.xMax, startPos.y);
					
					startPos.y = DisplayArea.height - startPos.y;
					endPos.y = startPos.y;
					
					Handles.color = AnimationCurveEditorUtility.LIGHT_GRID;
					
					if (y % AnimationCurveEditorUtility.BOLD_STEP == 0)
					{	
						GUIStyle style = null;
						GUISkin skin = USEditorUtility.USeqSkin;
						if(skin)
							style = skin.GetStyle("AnimCurveLabel");
						using(new WellFired.Shared.GUIChangeColor(AnimationCurveEditorUtility.GRID_LABEL_COLOR))
							GUI.Label(new Rect(startPos.x, y > 0 ? startPos.y : startPos.y - 15, 50, 20), string.Format("{0:F}", y * yStep + MinDisplayY), style);
						Handles.color = AnimationCurveEditorUtility.BOLD_GRID;
					}
					
					AnimationCurveEditorGUIUtility.DrawLine(startPos, endPos);
				}
			}
		}
		
		void DrawCurve(USInternalCurve curve, Color curveColour)
		{
			if(curve.Keys.Count == 0)
				return;
	
			using(new HandlesChangeColor(curveColour))
			{
				for(int keyframeIndex = 1; keyframeIndex < curve.Keys.Count; keyframeIndex++)
				{
					var keyframe = curve.Keys[keyframeIndex];
					for(int segmentIndex = 1; segmentIndex < cachedKeyframePositions[keyframe].CurveSegments.Count; segmentIndex++)
						AnimationCurveEditorGUIUtility.DrawLine(
							cachedKeyframePositions[keyframe].CurveSegments[segmentIndex - 1], 
							cachedKeyframePositions[keyframe].CurveSegments[segmentIndex],
							XScroll,
							XScale);
				}
	
				for(int keyframeIndex = 0; keyframeIndex < curve.Keys.Count; keyframeIndex++)
				{
					cachedKeyframePositions[curve.Keys[keyframeIndex]].LeftTangentColor = Color.white;
					cachedKeyframePositions[curve.Keys[keyframeIndex]].RightTangentColor = Color.white;
					if(SelectedTangent == 0)
						cachedKeyframePositions[curve.Keys[keyframeIndex]].LeftTangentColor = Color.yellow;
					if(SelectedTangent == 1)
						cachedKeyframePositions[curve.Keys[keyframeIndex]].RightTangentColor = Color.yellow;
	
					AnimationCurveEditorGUIUtility.KeyframeLabel(
						curve.Keys[keyframeIndex],
						cachedKeyframePositions[curve.Keys[keyframeIndex]],
						(keyframeIndex == 0 && curve.UseCurrentValue),
						XScroll,
						XScale);

					if(SelectedObjects.Count == 1 && SelectedObjects.Contains(curve.Keys[keyframeIndex]))
						DrawTangent(cachedKeyframePositions[curve.Keys[keyframeIndex]], curve.Keys[keyframeIndex]);
				}
			}
		}
	
		private void DrawTangent(AnimationKeyframeRenderData keyframeRenderData, USInternalKeyframe keyframe)
		{
			Vector2 FinalRenderPosition = keyframeRenderData.RenderPosition;
			FinalRenderPosition.x = (FinalRenderPosition.x * XScale) - XScroll;
			
			Rect leftTangentRect = new Rect(keyframeRenderData.LeftTangentEnd.x - 5, keyframeRenderData.LeftTangentEnd.y - 4, 10, 10);
			Rect rightTangentRect = new Rect(keyframeRenderData.RightTangentEnd.x - 5, keyframeRenderData.RightTangentEnd.y - 4, 10, 10);
	
			if(keyframeRenderData.HasLeftTangent)
			{
				float inAngle = -Mathf.Rad2Deg * Mathf.Atan(keyframe.InTangent);
				Vector2 inDir = Quaternion.Euler(0, 0, inAngle) * Vector2.right;
				
				float ratio = DisplayArea.height / DisplayArea.width;
				inDir.y = inDir.y * ratio;
				inDir.Normalize();
	
				keyframeRenderData.LeftTangentEnd = FinalRenderPosition + (-inDir * AnimationCurveEditorUtility.TANGENT_LENGTH);
				using(new WellFired.Shared.GUIChangeColor(keyframeRenderData.LeftTangentColor))
					GUI.Label(leftTangentRect, AnimationCurveEditorUtility.KeyframeTexture);
			}
			if(keyframeRenderData.HasRightTangent)
			{
				float outAngle = -Mathf.Rad2Deg * Mathf.Atan(keyframe.OutTangent);
				Vector2 outDir = Quaternion.Euler(0, 0, outAngle) * Vector2.right;
				
				float ratio = DisplayArea.height / DisplayArea.width;
				outDir.y = outDir.y * ratio;
				outDir.Normalize();
	
				keyframeRenderData.RightTangentEnd = FinalRenderPosition + (outDir * AnimationCurveEditorUtility.TANGENT_LENGTH);
				using(new WellFired.Shared.GUIChangeColor(keyframeRenderData.RightTangentColor))
					GUI.Label(rightTangentRect, AnimationCurveEditorUtility.KeyframeTexture);
			}
	
			if(Event.current.type == EventType.MouseDown && leftTangentRect.Contains(Event.current.mousePosition))
				SelectTangent(keyframeRenderData, 0);
			if(Event.current.type == EventType.MouseDown && rightTangentRect.Contains(Event.current.mousePosition))
				SelectTangent(keyframeRenderData, 1);
	
			if(Event.current.type == EventType.MouseDrag && SelectedTangent != -1)
				DragTangent(keyframeRenderData, 0);
			if(Event.current.type == EventType.MouseDrag && SelectedTangent != -1)
				DragTangent(keyframeRenderData, 1);
	
			if(Event.current.rawType == EventType.MouseUp && SelectedTangent != -1)
			{
				Event.current.Use();
				SelectedTangent = -1;
			}
		}
	
		private void DragTangent(AnimationKeyframeRenderData keyframeRenderData, int tangentIndex)
		{
			AreCurvesDirty = true;
			USInternalKeyframe keyframe = SelectedObjects[0] as USInternalKeyframe;
			
			USUndoManager.PropertyChange(keyframe, "Alter keyframe");
	
			Vector3 direction = (Event.current.mousePosition - keyframeRenderData.RenderPosition).normalized;
			
			float ratio = DisplayArea.height / DisplayArea.width;
			direction.y = direction.y / ratio;
			direction.Normalize();
	
			bool needsFlip = direction.y > 0.0f;
			float angle = Vector2.Angle(Vector2.right, direction);
	
			if(SelectedTangent == 1 && angle >= 90.0f)
				angle = 90.0f;
			if(SelectedTangent == 0 && angle <= 90.0f)
				angle = 90.0f;
			
			angle = Mathf.Deg2Rad * angle;
			angle = Mathf.Tan(angle);
			
			if(needsFlip)
				angle = -1.0f * angle;
			
			if(angle > 10000.0f || angle < -10000.0f)
				angle = Mathf.Infinity;
			
			if(!keyframe.BrokenTangents)
			{
				if(SelectedTangent == 0)
				{
					keyframe.InTangent = angle;
					keyframe.OutTangent = angle;
				}
				if(SelectedTangent == 1)
				{
					keyframe.OutTangent = angle;
					keyframe.InTangent = angle;
				}
			}
			else
			{	
				if(SelectedTangent == 0)
					keyframe.InTangent = angle;
				if(SelectedTangent == 1)
					keyframe.OutTangent = angle;
			}
			
			int keyframeIndex = keyframe.curve.Keys.ToList().FindIndex(element => element == keyframe);
			RebuildCachedKeyframeInformation(keyframe);
			if(keyframeIndex != 0)
				RebuildCachedKeyframeInformation(keyframe.curve.Keys[keyframeIndex - 1]);
			if(keyframeIndex < keyframe.curve.Keys.Count - 1)
				RebuildCachedKeyframeInformation(keyframe.curve.Keys[keyframeIndex + 1]);
	
			keyframe.curve.BuildAnimationCurveFromInternalCurve();
			Event.current.Use();
		}
	
		private void SelectTangent(AnimationKeyframeRenderData keyframeRenderData, int tangentIndex)
		{
			USUndoManager.PropertyChange(this, "Select Tangent");
			SelectedTangent = tangentIndex;
			Event.current.Use();
		}
	
		public float convertValueToPanePosition(float value, Rect myArea)
		{
			float startY = myArea.y;
			float height = myArea.height;
			float paneValue = 0.0f;
			
			// To
			float keyframeRatio = (value - MinDisplayY) / (MaxDisplayY - MinDisplayY);
			paneValue = height * keyframeRatio;
			paneValue = height - paneValue;
			
			return paneValue + startY;
		}
		
		private void RebuildCachedCurveInformation()
		{
			if(cachedKeyframePositions == null)
				cachedKeyframePositions = new Dictionary<USInternalKeyframe, AnimationKeyframeRenderData>();
	
			cachedKeyframePositions.Clear();
	
			if(Curves == null)
				return;
	
			for(int curveIndex = 0; curveIndex < Curves.Count; curveIndex++)
			{
				var curve = Curves[curveIndex];
				for(int keyframeIndex = 0; keyframeIndex < curve.Keys.Count; keyframeIndex++)
					RebuildCachedKeyframeInformation(curve.Keys[keyframeIndex]);
			}
		}
	
		private void RebuildCachedKeyframeInformation(USInternalKeyframe keyframe)
		{
			cachedKeyframePositions[keyframe] = new AnimationKeyframeRenderData();
	
			int keyframeIndex = keyframe.curve.Keys.ToList().FindIndex(element => element == keyframe);

			float keyframeRatio = (keyframe.Value - MinDisplayY) / (MaxDisplayY - MinDisplayY);
			Vector2 keyframePos = new Vector2(DisplayArea.width * (keyframe.Time / Duration), DisplayArea.height * keyframeRatio);
			keyframePos.y = DisplayArea.height - keyframePos.y;
			cachedKeyframePositions[keyframe].RenderPosition = keyframePos;
			cachedKeyframePositions[keyframe].RenderRect = new Rect(cachedKeyframePositions[keyframe].RenderPosition.x - 6, cachedKeyframePositions[keyframe].RenderPosition.y - 6, 16, 16);
			cachedKeyframePositions[keyframe].HasLeftTangent = false;
			cachedKeyframePositions[keyframe].HasRightTangent = false;
	
			// Evaluate, stepCount steps per curve section.
			if(keyframeIndex != 0)
			{
				var prevKeyframe = keyframe.curve.Keys[keyframeIndex - 1];
				float timeDifference = keyframe.Time - prevKeyframe.Time;
				float timeStep = timeDifference / AnimationCurveEditorUtility.CURVE_KEYFRAME_STEP_COUNT;
				float startTime = prevKeyframe.Time;
				for(int n = 0; n <= AnimationCurveEditorUtility.CURVE_KEYFRAME_STEP_COUNT; n++)
				{	
					float nextEvaluationTime = startTime + timeStep;
					float prevSampleValue = keyframe.curve.Evaluate(startTime);
					float sampleValue = keyframe.curve.Evaluate(nextEvaluationTime);
						
					float prevRatio = (prevSampleValue - MinDisplayY) / (MaxDisplayY - MinDisplayY);
					float nextRatio = (sampleValue - MinDisplayY) / (MaxDisplayY - MinDisplayY);
						
					Vector2 startPos = new Vector2(DisplayArea.width * (startTime / Duration), DisplayArea.height * prevRatio);
					Vector2 endPos = new Vector2(DisplayArea.width * (nextEvaluationTime / Duration), DisplayArea.height * nextRatio);
				
					startPos.y = DisplayArea.height - startPos.y;
					endPos.y = DisplayArea.height - endPos.y;
				
					cachedKeyframePositions[keyframe].CurveSegments.Add(startPos);
					startTime = nextEvaluationTime;
				}
			}
	
			if(keyframeIndex < keyframe.curve.Keys.Count - 1)
				cachedKeyframePositions[keyframe].HasRightTangent = true;
			
			if(keyframeIndex > 0)
				cachedKeyframePositions[keyframe].HasLeftTangent = true;
		}
	}
}