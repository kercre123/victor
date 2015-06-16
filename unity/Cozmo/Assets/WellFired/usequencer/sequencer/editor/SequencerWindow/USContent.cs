using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	[Serializable]
	public class USContent : ScriptableObject 
	{
		private List<USTimelineMarkerCachedData> cachedMarkerData = new List<USTimelineMarkerCachedData>();
		
		private Dictionary<ISelectableContainer, List<UnityEngine.Object>> objectContainerMap = new Dictionary<ISelectableContainer, List<UnityEngine.Object>>();
	
		private float totalPixelWidthOfTimeline = 1.0f;
		private bool extendingFloatingWidth = false;
		private float additionalFloatingWidth = 0.0f;
	
		[SerializeField]
		private USSequencer currentSequence;
		private USSequencer CurrentSequence
		{
			get { return currentSequence; }
			set 
			{
				if(USHierarchy != null)
				{
					USHierarchy.Reset();
				}

				currentSequence = value; 
				UpdateCachedMarkerInformation(); 
				SequenceWindow.Repaint(); 
			}
		}
		
		[SerializeField]
		private USZoomInfo zoomInfo;
		private USZoomInfo ZoomInfo
		{
			get { return zoomInfo; }
			set { zoomInfo = value; }
		}
		
		[SerializeField]
		private USScrollInfo scrollInfo;
		private USScrollInfo ScrollInfo
		{
			get { return scrollInfo; }
			set { scrollInfo = value; }
		}
	
		[SerializeField]
		private float baseFloatingWidth = 200.0f;
		private float BaseFloatingWidth
		{
			get { return baseFloatingWidth; }
			set { baseFloatingWidth = value; }
		}
		
		[SerializeField]
		private USWindow sequenceWindow;
		public USWindow SequenceWindow
		{
			get { return sequenceWindow; }
			set { sequenceWindow = value; }
		}
		
		[SerializeField]
		private bool snap;
		public bool Snap
		{
			get { return snap; }
			set { snap = value; }
		}

		public float SnapAmount
		{
			get { return EditorPrefs.GetFloat("WellFired-uSequencer-SnapAmout", 1.0f); }
			set { EditorPrefs.SetFloat("WellFired-uSequencer-SnapAmout", value); }
		}
	
		private float FloatingWidth
		{
			get;
			set;
		}
		
		private Rect FloatingArea
		{
			get;
			set;
		}
		
		private Rect ScrubArea
		{
			get;
			set;
		}
		
		private Rect HierarchyArea
		{
			get;
			set;
		}
		
		private Rect HorizontalScrollArea
		{
			get;
			set;
		}
		
		private Rect VerticalScrollArea
		{
			get;
			set;
		}
		
		[SerializeField]
		private USHierarchy usHierarcy;
		public USHierarchy USHierarchy
		{
			get { return usHierarcy; }
			set { usHierarcy = value; }
		}
		
		[SerializeField]
		private Texture cachedDragTexture;
		private Texture DragAreaTexture
		{
			get
			{
				if(!cachedDragTexture)	
					cachedDragTexture = Resources.Load("DragAreaTexture", typeof(Texture)) as Texture;
				return cachedDragTexture;
			}
			set { ; }
		}
		
		[SerializeField]
		private Rect SelectableArea
		{
			get;
			set;
		}
		
		[SerializeField]
		private Rect SelectionArea
		{
			get;
			set;
		}
		
		[SerializeField]
		private bool IsDragging
		{
			get;
			set;
		}
		
		[SerializeField]
		private bool HasStartedDrag
		{
			get;
			set;
		}
		
		[SerializeField]
		private bool HasProcessedInitialDrag
		{
			get;
			set;
		}
		
		[SerializeField]
		private bool IsBoxSelecting
		{
			get;
			set;
		}
		
		[SerializeField]
		private bool IsDuplicating
		{
			get;
			set;
		}
		
		[SerializeField]
		private bool HasDuplicated
		{
			get;
			set;
		}
		
		[SerializeField]
		private Vector2 DragStartPosition
		{
			get;
			set;
		}
	
		[SerializeField]
		public bool ScrubHandleDrag
		{
			get;
			private set;
		}
	
		private void OnEnable() 
		{
			hideFlags = HideFlags.HideAndDontSave;
			FloatingWidth = BaseFloatingWidth + additionalFloatingWidth;
	
			if(ScrollInfo == null)
			{
				ScrollInfo = ScriptableObject.CreateInstance<USScrollInfo>();
				ScrollInfo.Reset();
			}
			if(ZoomInfo == null)
			{
				ZoomInfo = ScriptableObject.CreateInstance<USZoomInfo>();
				ZoomInfo.Reset();
			}
	
			if(CurrentSequence)
				UpdateCachedMarkerInformation();
		}
	
		private void OnDestroy()
		{
			USHierarchy.OnItemClicked -= OnItemClicked;
			USHierarchy.OnItemContextClicked -= OnItemContextClicked;
		}
	
		private void LayoutAreas()
		{
			using(new WellFired.Shared.GUIBeginVertical())
			{
				using(new WellFired.Shared.GUIBeginHorizontal())
				{
					using(new WellFired.Shared.GUIBeginVertical())
					{
						using(new WellFired.Shared.GUIBeginHorizontal())
						{
							GUILayout.Box("Floating", USEditorUtility.ContentBackground, GUILayout.Width(FloatingWidth), GUILayout.Height(18.0f));
							if(Event.current.type == EventType.Repaint)
							{
								if(FloatingArea != GUILayoutUtility.GetLastRect())
								{
									FloatingArea = GUILayoutUtility.GetLastRect();
									SequenceWindow.Repaint();
								}
							}
							
							GUILayout.Box("Scrub", USEditorUtility.ContentBackground, GUILayout.ExpandWidth(true), GUILayout.Height(18.0f));
							if(Event.current.type == EventType.Repaint)
							{
								if(ScrubArea != GUILayoutUtility.GetLastRect())
								{
									ScrubArea = GUILayoutUtility.GetLastRect();
									SequenceWindow.Repaint();
									UpdateCachedMarkerInformation();
								}
							}
						}
	
						GUILayout.Box("Hierarchy", USEditorUtility.ContentBackground, GUILayout.ExpandWidth(true), GUILayout.ExpandHeight(true));
						if(Event.current.type == EventType.Repaint)
						{
							if(HierarchyArea != GUILayoutUtility.GetLastRect())
							{
								HierarchyArea = GUILayoutUtility.GetLastRect();

								SequenceWindow.Repaint();
								UpdateCachedMarkerInformation();
							}
						}
					}
	
					GUILayout.Box("Scroll", USEditorUtility.ContentBackground, GUILayout.Width(18.0f), GUILayout.ExpandHeight(true));
					if(Event.current.type == EventType.Repaint)
					{
						if(VerticalScrollArea != GUILayoutUtility.GetLastRect())
						{
							VerticalScrollArea = GUILayoutUtility.GetLastRect();
							SequenceWindow.Repaint();
							UpdateCachedMarkerInformation();
						}
					}
				}
	
				using(new WellFired.Shared.GUIBeginHorizontal())
				{
					GUILayout.Box("Scroll", USEditorUtility.ContentBackground, GUILayout.ExpandWidth(true), GUILayout.Height(18.0f));			
					if(Event.current.type == EventType.Repaint)
					{
						if(HorizontalScrollArea != GUILayoutUtility.GetLastRect())
						{
							HorizontalScrollArea = GUILayoutUtility.GetLastRect();
							SequenceWindow.Repaint();
							UpdateCachedMarkerInformation();
						}
					}
	
					GUILayout.Box("block bit", USEditorUtility.ContentBackground, GUILayout.Width(18.0f), GUILayout.Height(18.0f));
				}
			}
		}
	
		public void OnGUI()
		{
			LayoutAreas();
	
			GUI.Box(FloatingArea, "", USEditorUtility.ContentBackground);
			GUI.Box(ScrubArea, "", USEditorUtility.ScrubBarBackground);
			
			float widthOfContent = ScrubArea.x + (CurrentSequence.Duration / ZoomInfo.meaningOfEveryMarker * ZoomInfo.currentXMarkerDist);
			if(Event.current.type == EventType.ScrollWheel && ScrubArea.Contains(Event.current.mousePosition))
			{	
				float zoom = (Event.current.delta.y * USPreferenceWindow.ZoomFactor);
				if(Event.current.control)
					zoom *= 10.0f;

				ZoomInfo.currentZoom += (zoom * (USPreferenceWindow.ZoomInvert == true ? -1.0f : 1.0f));
				if(ZoomInfo.currentZoom <= 1.0f)
					ZoomInfo.currentZoom = 1.0f;
	
				ScrollInfo.currentScroll.x += Event.current.delta.x;
				ScrollInfo.currentScroll.x = Mathf.Clamp(ScrollInfo.currentScroll.x, 0, widthOfContent);

				Event.current.Use();
				UpdateCachedMarkerInformation();

				var newWidthOfContent = ScrubArea.x + (CurrentSequence.Duration / ZoomInfo.meaningOfEveryMarker * ZoomInfo.currentXMarkerDist);
				var woc = newWidthOfContent - FloatingWidth;
				var mxFromLeft = (Event.current.mousePosition.x - FloatingWidth + ScrollInfo.currentScroll.x);
				var ratio = mxFromLeft / woc;
				var contentWidthDiff = (newWidthOfContent - widthOfContent) * ratio;
				ScrollInfo.currentScroll.x += contentWidthDiff;
				
				UpdateCachedMarkerInformation();
			}
					
			using(new WellFired.Shared.GUIBeginArea(ScrubArea))
			{
				foreach(var cachedMarker in cachedMarkerData)
				{
					GUI.DrawTexture(cachedMarker.MarkerRenderRect, USEditorUtility.TimelineMarker);
					if(cachedMarker.MarkerRenderLabel != string.Empty)
						GUI.Label(cachedMarker.MarkerRenderLabelRect, cachedMarker.MarkerRenderLabel);
				}
				
				// Render our scrub Handle
				float currentScrubPosition = TimeToContentX(CurrentSequence.RunningTime);
				float halfScrubHandleWidth = 5.0f;
				Rect scrubHandleRect = new Rect(currentScrubPosition - halfScrubHandleWidth, 0.0f, halfScrubHandleWidth * 2.0f, ScrubArea.height);
				using(new WellFired.Shared.GUIChangeColor(new Color(1.0f, 0.1f, 0.1f, 0.65f)))
					GUI.DrawTexture(scrubHandleRect, USEditorUtility.TimelineScrubHead);
				
				// Render the running time here
				scrubHandleRect.x += scrubHandleRect.width;
				scrubHandleRect.width = 100.0f;
				GUI.Label(scrubHandleRect, CurrentSequence.RunningTime.ToString("#.####"));
	
				if(Event.current.type == EventType.MouseDown)
					ScrubHandleDrag = true;
				if(Event.current.rawType == EventType.MouseUp)
					ScrubHandleDrag = false;
				
				if(ScrubHandleDrag && Event.current.isMouse)
				{	
					float mousePosOnTimeline = ContentXToTime(FloatingWidth + Event.current.mousePosition.x);
					sequenceWindow.SetRunningTime(mousePosOnTimeline);
					Event.current.Use();
				}
			}
					
			UpdateGrabHandle();
	
			float height = USHierarchy.TotalArea.height;
			if(USHierarchy.TotalArea.height < HierarchyArea.height)
				height = HierarchyArea.height;

			ScrollInfo.currentScroll.y = GUI.VerticalScrollbar(VerticalScrollArea, ScrollInfo.currentScroll.y, HierarchyArea.height, 0.0f, height);			
			ScrollInfo.currentScroll.x = GUI.HorizontalScrollbar(HorizontalScrollArea, ScrollInfo.currentScroll.x, HierarchyArea.width, 0.0f, widthOfContent);
			ScrollInfo.currentScroll.x = Mathf.Clamp(ScrollInfo.currentScroll.x, 0, widthOfContent);
			ScrollInfo.currentScroll.y = Mathf.Clamp(ScrollInfo.currentScroll.y, 0, USHierarchy.TotalArea.height);
	
			ContentGUI();
			
			// Render our red line
			Rect scrubMarkerRect = new Rect(ScrubArea.x + TimeToContentX(CurrentSequence.RunningTime), HierarchyArea.y, 1.0f, HierarchyArea.height);
	
			// Don't render an offscreen scrub line.
			if(scrubMarkerRect.x < HierarchyArea.x)
				return;
			
			using(new WellFired.Shared.GUIChangeColor(new Color(1.0f, 0.1f, 0.1f, 0.65f)))
				GUI.DrawTexture(scrubMarkerRect, USEditorUtility.TimelineScrubTail);
		}
		
		private void ContentGUI()
		{	
			MaintainHierarchyFor(CurrentSequence);
			using(new WellFired.Shared.GUIBeginArea(HierarchyArea))
			{
				if(Event.current.type == EventType.ScrollWheel)
				{
					ScrollInfo.currentScroll.x += Event.current.delta.x;
					ScrollInfo.currentScroll.y += Event.current.delta.y;
	
					float widthOfContent = ScrubArea.x + (CurrentSequence.Duration / ZoomInfo.meaningOfEveryMarker * ZoomInfo.currentXMarkerDist);
					ScrollInfo.currentScroll.x = Mathf.Clamp(ScrollInfo.currentScroll.x, 0, widthOfContent);
					
					UpdateCachedMarkerInformation();
					
					Event.current.Use();
				}
	
				USHierarchy.DoGUI(0);
				
				Rect selectableArea = USHierarchy.VisibleArea;
				if(selectableArea.Contains(Event.current.mousePosition) || Event.current.rawType == EventType.MouseUp || Event.current.rawType == EventType.MouseDrag)
					HandleEvent(Event.current.rawType == EventType.MouseUp ? Event.current.rawType : Event.current.type, Event.current.button, Event.current.mousePosition);
				
				// Render our mouse drag box.
				if(IsBoxSelecting && HasStartedDrag)
				{
					Vector2 mousePosition = Event.current.mousePosition;
					Vector2 origin = DragStartPosition;
					Vector2 destination = mousePosition;
					
					if(mousePosition.x < DragStartPosition.x)
					{
						origin.x = mousePosition.x;
						destination.x = DragStartPosition.x;
					}
					
					if(mousePosition.y < DragStartPosition.y)
					{
						origin.y = mousePosition.y;
						destination.y = DragStartPosition.y;
					}
					
					Vector2 mouseDelta = destination - origin;
					SelectionArea = new Rect(origin.x, origin.y, mouseDelta.x, mouseDelta.y);
					if(!EditorGUIUtility.isProSkin)
						GUI.Box(SelectionArea, "", USEditorUtility.USeqSkin.box);
					else
						GUI.Box(SelectionArea, "");
					
					SequenceWindow.Repaint();
				}
	
			}
		}
	
		public void UpdateCachedMarkerInformation()
		{
			cachedMarkerData.Clear();
			
			// Update the area
			// The marker area has one big Marker per region and ten small markers per region, as we zoom in the small regions
			// become bigger and as we zoom in, they disappear.
			// A block from n -> n + 1 defines this time region.
			float currentZoomUpper = Mathf.Ceil(ZoomInfo.currentZoom);
			float currentZoomlower = Mathf.Floor(ZoomInfo.currentZoom);
			float zoomRatio = (ZoomInfo.currentZoom - currentZoomlower) / (currentZoomUpper / currentZoomUpper);
	
			float maxDuration = Mathf.Min(CurrentSequence.Duration, 10);
			float minXMarkerDist = ScrubArea.width / maxDuration;
			float maxXMarkerDist = minXMarkerDist * 2.0f;
			ZoomInfo.currentXMarkerDist = maxXMarkerDist * zoomRatio + minXMarkerDist * (1.0f - zoomRatio);
			
			float minXSmallMarkerHeight = ScrubArea.height * 0.1f;
			float maxXSmallMarkerHeight = ScrubArea.height * 0.8f;
			float currentXSmallMarkerHeight = maxXSmallMarkerHeight * zoomRatio + minXSmallMarkerHeight * (1.0f - zoomRatio);
			
			// Calculate our maximum zoom out.
			float maxNumberOfMarkersInPane = ScrubArea.width / minXMarkerDist;
			ZoomInfo.meaningOfEveryMarker = Mathf.Ceil(CurrentSequence.Duration / maxNumberOfMarkersInPane);
	
			int levelsDeep = Mathf.FloorToInt(ZoomInfo.currentZoom);
			while(levelsDeep > 1)
			{
				ZoomInfo.meaningOfEveryMarker *= 0.5f;
				levelsDeep -= 1;
			}
			
			totalPixelWidthOfTimeline = ZoomInfo.currentXMarkerDist * (CurrentSequence.Duration / ZoomInfo.meaningOfEveryMarker);
			
			// Calculate how much we can see, for our horizontal scroll bar, this is for clamping.
			ScrollInfo.visibleScroll.x = (ScrubArea.width / ZoomInfo.currentXMarkerDist) * ZoomInfo.currentXMarkerDist;
			if(ScrollInfo.visibleScroll.x > totalPixelWidthOfTimeline)
				ScrollInfo.visibleScroll.x = totalPixelWidthOfTimeline;
	
			// Create our markers
			float markerValue = 0.0f;
			Rect markerRect = new Rect(-ScrollInfo.currentScroll.x, 0.0f, 1.0f, maxXSmallMarkerHeight);
			while(markerRect.x < ScrubArea.width)
			{
				if(markerValue > CurrentSequence.Duration)
					break;
	
				// Big marker
				cachedMarkerData.Add(new USTimelineMarkerCachedData());
	
				cachedMarkerData[cachedMarkerData.Count - 1].MarkerRenderRect = markerRect;
				cachedMarkerData[cachedMarkerData.Count - 1].MarkerRenderLabelRect = new Rect(markerRect.x + 2.0f, markerRect.y, 40.0f, ScrubArea.height);
				cachedMarkerData[cachedMarkerData.Count - 1].MarkerRenderLabel = markerValue.ToString();
				
				// Small marker
				for(int n = 1; n <= 10; n++)
				{
					float xSmallPos = markerRect.x + ZoomInfo.currentXMarkerDist / 10.0f * n;
					
					Rect smallMarkerRect = markerRect;
					smallMarkerRect.x = xSmallPos;
					smallMarkerRect.height = minXSmallMarkerHeight;
					
					if(n == 5)
						smallMarkerRect.height = currentXSmallMarkerHeight;
	
					cachedMarkerData.Add(new USTimelineMarkerCachedData());
					cachedMarkerData[cachedMarkerData.Count - 1].MarkerRenderRect = smallMarkerRect;
				}
				
				markerRect.x += ZoomInfo.currentXMarkerDist;
				markerValue += ZoomInfo.meaningOfEveryMarker;
			}
		}
		
		private void UpdateGrabHandle() 
		{
			FloatingWidth = additionalFloatingWidth + BaseFloatingWidth;
			Rect resizeHandle = new Rect(FloatingWidth - 10.0f, ScrubArea.y, 10.0f, ScrubArea.height);
			using(new WellFired.Shared.GUIBeginArea(resizeHandle, "", "box"))
			{
				if(Event.current.type == EventType.MouseDown && Event.current.button == 0)
					extendingFloatingWidth = true;
	
				if(extendingFloatingWidth && Event.current.type == EventType.MouseDrag)
				{
					additionalFloatingWidth += Event.current.delta.x;
					FloatingWidth = additionalFloatingWidth + BaseFloatingWidth;
					UpdateCachedMarkerInformation();
					SequenceWindow.Repaint();
				}
			}
	
			if (Event.current.type == EventType.MouseUp)
				extendingFloatingWidth = false;
		}
		
		public void OnSequenceChange(USSequencer newSequence)
		{
			CurrentSequence = newSequence;
			ZoomInfo.Reset();
			ScrollInfo.Reset();
			totalPixelWidthOfTimeline = 1.0f;
			UpdateCachedMarkerInformation();
			
			if(USHierarchy == null)
				USHierarchy = ScriptableObject.CreateInstance(typeof(USHierarchy)) as USHierarchy;
	
			foreach(var newTimelineContainer in CurrentSequence.TimelineContainers)
			{
				var newHierarchyItem = ScriptableObject.CreateInstance(typeof(USTimelineContainerHierarchyItem)) as USTimelineContainerHierarchyItem;
				newHierarchyItem.SetupWithTimelineContainer(newTimelineContainer);
				USHierarchy.AddHierarchyItemToRoot(newHierarchyItem as IUSHierarchyItem);
			}
	
			SequenceWindow.Repaint();
		}
		
		private void MaintainHierarchyFor(USSequencer sequence)
		{
			USHierarchy.OnItemClicked -= OnItemClicked;
			USHierarchy.OnItemClicked += OnItemClicked;
			USHierarchy.OnItemContextClicked -= OnItemContextClicked;
			USHierarchy.OnItemContextClicked += OnItemContextClicked;
			
			USHierarchy.EditorWindow = SequenceWindow;
			USHierarchy.CurrentXMarkerDist = zoomInfo.currentXMarkerDist;
			USHierarchy.FloatingWidth = FloatingWidth;
			USHierarchy.Scrollable = true;
			USHierarchy.ScrollPos = ScrollInfo.currentScroll;
			USHierarchy.XScale = (totalPixelWidthOfTimeline / ScrubArea.width);
	
			USHierarchy.CheckConsistency();
		}
		
		private void OnItemClicked(IUSHierarchyItem item)
		{
			USTimelineContainerHierarchyItem timelineContainerItem = item as USTimelineContainerHierarchyItem;

			if(timelineContainerItem != null && !USRuntimeUtility.HasObserverTimeline(timelineContainerItem.TimelineContainer.transform))
			{
				EditorGUIUtility.PingObject(timelineContainerItem.TimelineContainer.AffectedObject);
				Selection.activeGameObject = timelineContainerItem.TimelineContainer.AffectedObject.gameObject;
				Selection.activeTransform = timelineContainerItem.TimelineContainer.AffectedObject;
			}
			
			IUSTimelineHierarchyItem timelineHierarchyItem = item as IUSTimelineHierarchyItem;
			if(timelineHierarchyItem)
			{
				EditorGUIUtility.PingObject(timelineHierarchyItem.PingableObject);
				Selection.activeTransform = timelineHierarchyItem.PingableObject;
			}
		}
		
		private void OnItemContextClicked(IUSHierarchyItem item)
		{ 
			USTimelineContainerHierarchyItem timelineContainerItem = item as USTimelineContainerHierarchyItem;
			IUSTimelineHierarchyItem timelineHierarchyItem = item as IUSTimelineHierarchyItem;
			GenericMenu contextMenu = new GenericMenu();

			var canContextClickTimelineContainer = !timelineContainerItem || USEditorUtility.CanContextClickTimelineContainer(timelineContainerItem.TimelineContainer);
			if(!canContextClickTimelineContainer)
				return;
			
			var canContextClickTimeline = !timelineHierarchyItem || USEditorUtility.CanContextClickTimeline(timelineHierarchyItem.BaseTimeline);
			if(!canContextClickTimeline)
				return;
			
			if(timelineHierarchyItem || timelineContainerItem)
			{
				contextMenu.AddItem(new GUIContent("Remove"), false, () => ProcessDelete(item));
			}

			// if we context click on a hierarchy container, always allow the duplication
			var isDuplicateEnabled = timelineContainerItem != null ? true :false;

			// If we context click on a hierarchy item, we need to make sure we can duplicate that item.
			if(timelineHierarchyItem)
			{
				var customAttribute = timelineHierarchyItem.GetType().GetCustomAttributes(typeof(USCustomTimelineHierarchyItem), true).FirstOrDefault() as USCustomTimelineHierarchyItem;
				if(customAttribute != default(USCustomTimelineHierarchyItem))
				{
					if(customAttribute.CanBeManuallyAddToSequence && customAttribute.MaxNumberPerSequence > 1)
					{
						isDuplicateEnabled = true;
					}
				}
				else
				{
					isDuplicateEnabled = true;
				}
			}

			if(isDuplicateEnabled)
			{
				contextMenu.AddSeparator("Copy To/This Sequence");
				contextMenu.AddItem(new GUIContent("Copy To/Duplicate"), false, () => ProcessDuplicate(item));
				contextMenu.AddSeparator("Copy To/Other Sequences");
			}
			else
			{
				contextMenu.AddDisabledItem(new GUIContent("Copy To/This Sequence"));
			}

			USSequencer []sequences = FindObjectsOfType(typeof(USSequencer)) as USSequencer[];
			var orderedSequences = sequences.OrderBy(sequence => sequence.name).Where(sequence => sequence != CurrentSequence);
			foreach(USSequencer sequence in orderedSequences)
			{
				contextMenu.AddItem(new GUIContent(string.Format("{0}/{1}", "Copy To", sequence.name)), 
				             false, 
				             (obj) => ProcessDuplicateToSequence(item, (USSequencer)obj), 
				             sequence);
			}

			if(timelineContainerItem)
			{
				var customKeyValuePairs = USEditorUtility.CustomTimelineAttributes;
				foreach(var customKeyValuePair in customKeyValuePairs)
				{
					var customTimelineDescription = customKeyValuePair.Key;
	
					if(!customTimelineDescription.CanBeManuallyAddToSequence)
						continue;
	
					if(USRuntimeUtility.GetNumberOfTimelinesOfType(customTimelineDescription.InspectedType, timelineContainerItem.TimelineContainer) >= customTimelineDescription.MaxNumberPerSequence)
						continue;
	
					contextMenu.AddItem(
						new GUIContent(string.Format("Add Timeline/{0}", customTimelineDescription.FriendlyName)), 
					    false, 
						(obj) => { AddNewTimeline((obj as object[])[0] as USTimelineContainerHierarchyItem, (obj as object[])[1] as USCustomTimelineHierarchyItem); },
						new object[] { timelineContainerItem, customTimelineDescription }
					);
				}
			}

			item.AddContextItems(contextMenu);

			if(timelineContainerItem != null &&
				(timelineContainerItem.TimelineContainer.AffectedObject == null ||
			   	(Selection.activeGameObject != null && 
			   	Selection.activeObject != timelineContainerItem.TimelineContainer.AffectedObject.gameObject)))
			{
				if(Selection.activeGameObject != null)
				{
					contextMenu.AddItem(
						new GUIContent(string.Format("Update Affected Object : {0}", Selection.activeGameObject.name)), 
						false, 
						() => UpdateAffectedObject(timelineContainerItem, Selection.activeGameObject));
				}
			}


			contextMenu.ShowAsContext();
		}
		
		public void ProcessDelete(IUSHierarchyItem item)
		{	
			USTimelineContainerHierarchyItem timelineContainerItem = item as USTimelineContainerHierarchyItem;
			IUSTimelineHierarchyItem timelineHierarchyItem = item as IUSTimelineHierarchyItem;
			
			if(timelineContainerItem)
			{
				foreach(var child in timelineContainerItem.Children.ToList())
					ProcessDelete(child);
				
				USUndoManager.RegisterCompleteObjectUndo(this, "Remove Timeline Container");
				USUndoManager.RegisterCompleteObjectUndo(USHierarchy, "Remove Timeline Container");
				USHierarchy.RootItems.Remove(timelineContainerItem);
				
				var gameObjectToDestroy = timelineContainerItem.TimelineContainer.gameObject;
				USUndoManager.DestroyImmediate(timelineContainerItem);
				USUndoManager.DestroyImmediate(gameObjectToDestroy);
			}
			else if(timelineHierarchyItem)
			{
				IUSHierarchyItem parent = USHierarchy.GetParentOf(timelineHierarchyItem);

				timelineHierarchyItem.RestoreBaseState();
				
				USUndoManager.RegisterCompleteObjectUndo(parent, "Remove Timeline");
				parent.RemoveChild(timelineHierarchyItem);
	
				var gameObjectToDestroy = timelineHierarchyItem.TimelineToDestroy().gameObject;
				USUndoManager.DestroyImmediate(timelineHierarchyItem);
				USUndoManager.DestroyImmediate(gameObjectToDestroy);
			}
		}

		private void ProcessDuplicate(IUSHierarchyItem item)
		{
			var timelineContainer = item as USTimelineContainerHierarchyItem;
			if(timelineContainer != null)
			{
				var newTimelineContainer = USEditor.DuplicateTimelineContainer(timelineContainer.TimelineContainer, CurrentSequence);
				
				USUndoManager.RegisterCreatedObjectUndo(newTimelineContainer.gameObject, "Duplicate Timeline");

				AddNewTimelineContainer(newTimelineContainer);
			}

			var timeline = item as IUSTimelineHierarchyItem;
			if(timeline != null)
			{
				USUndoManager.RegisterCompleteObjectUndo(this, "Duplicate Timeline");
				USUndoManager.RegisterCompleteObjectUndo(USHierarchy, "Duplicate Timeline");

				var newTimeline = USEditor.DuplicateTimeline(timeline.BaseTimeline, timeline.BaseTimeline.TimelineContainer);
				USUndoManager.RegisterCreatedObjectUndo(newTimeline.gameObject, "Duplicate Timeline");

				var parent = USHierarchy.GetParentOf(item) as USTimelineContainerHierarchyItem;
				USUndoManager.RegisterCompleteObjectUndo(parent, "Duplicate Timeline");
				parent.AddTimeline(newTimeline);
			}
		}

		private void ProcessDuplicateToSequence(IUSHierarchyItem item, USSequencer sequence)
		{
			var affectedObject = default(Transform);
			var timelineContainer = item as USTimelineContainerHierarchyItem;
			var timeline = item as IUSTimelineHierarchyItem;

			if(timelineContainer != null)
				affectedObject = timelineContainer.TimelineContainer.AffectedObject;
			if(timeline != null)
				affectedObject = timeline.BaseTimeline.AffectedObject;

			if(timelineContainer && USRuntimeUtility.HasTimelineContainerWithAffectedObject(sequence, affectedObject))
			{
				EditorUtility.DisplayDialog("Cannot continue", string.Format("The sequence {0} already has timelines for the object {1}", sequence.name, affectedObject.name), "Ok");
				return;
			}

			if(timelineContainer)
			{
				var newTimelineContainer = USEditor.DuplicateTimelineContainer(timelineContainer.TimelineContainer, sequence);
				
				USUndoManager.RegisterCreatedObjectUndo(newTimelineContainer.gameObject, "Duplicate Timeline");
			}

			if(timeline)
			{
				var newTimelineContainer = sequence.GetTimelineContainerFor(timeline.BaseTimeline.AffectedObject);
				if(newTimelineContainer == null)
				{
					newTimelineContainer = sequence.CreateNewTimelineContainer(timeline.BaseTimeline.AffectedObject);
					USUndoManager.RegisterCreatedObjectUndo(newTimelineContainer.gameObject, "Duplicate Timeline");
				}
				var newTimeline = USEditor.DuplicateTimeline(timeline.BaseTimeline, newTimelineContainer);
				
				USUndoManager.RegisterCreatedObjectUndo(newTimeline.gameObject, "Duplicate Timeline");
			}
		}

		private void UpdateAffectedObject(USTimelineContainerHierarchyItem timelineContainerHierarchyItem, GameObject newAffectedObject)
		{
			timelineContainerHierarchyItem.TimelineContainer.AffectedObject = newAffectedObject.transform;
			timelineContainerHierarchyItem.Children.Clear(); 
			timelineContainerHierarchyItem.SetupWithTimelineContainer(timelineContainerHierarchyItem.TimelineContainer);
		}
		
		public void AddNewTimeline(USTimelineContainerHierarchyItem hierarchyItem, USCustomTimelineHierarchyItem timelineAttribute)
		{
			GameObject timelineObject = new GameObject(timelineAttribute.FriendlyName);
			timelineObject.transform.parent = hierarchyItem.TimelineContainer.transform;
			timelineObject.transform.position = Vector3.zero;
			timelineObject.transform.rotation = Quaternion.identity;
	
			var timeline = timelineObject.AddComponent(timelineAttribute.InspectedType) as USTimelineBase;
			USUndoManager.RegisterCreatedObjectUndo(timelineObject, "Add New Timeline");
			USUndoManager.PropertyChange(hierarchyItem, "Add New Timeline");
			
			hierarchyItem.AddTimeline(timeline);
		}
		
		public void HandleEvent(EventType eventType, int button, Vector2 mousePosition)
		{
			List<ISelectableContainer> selectableContainers = USHierarchy.ISelectableContainers();
			List<UnityEngine.Object> allObjectsUnderMouse = new List<UnityEngine.Object>();
			bool hasObjectsUnderMouse = false;
			objectContainerMap.Clear();
			
			foreach(var selectableContainer in selectableContainers)
			{
				List<UnityEngine.Object> objectsUnderMouse = new List<UnityEngine.Object>();
				if(IsBoxSelecting && HasStartedDrag)
					objectsUnderMouse = selectableContainer.GetSelectableObjectsUnderRect(SelectionArea, ScrollInfo.currentScroll.y);
				else
					objectsUnderMouse = selectableContainer.GetSelectableObjectUnderPos(mousePosition);
	
				allObjectsUnderMouse.AddRange(objectsUnderMouse);
				if(objectsUnderMouse.Count > 0)
					hasObjectsUnderMouse = true;
				
				objectContainerMap[selectableContainer] = objectsUnderMouse;
			}
			
			switch(eventType)
			{
				case EventType.MouseDown:
				{
					HasProcessedInitialDrag = false;
					IsDragging = false;
					IsBoxSelecting = false;
					DragStartPosition = mousePosition;
					
					if(!hasObjectsUnderMouse && Event.current.button == 0)
						IsBoxSelecting = true;
					if(hasObjectsUnderMouse && Event.current.button == 0)
						IsDragging = true;
					if(IsDragging && Event.current.alt && Event.current.control)
						IsDuplicating = true;
				
					// if we have no objects under our mouse, then we are likely trying to clear our selection
					if(!hasObjectsUnderMouse && (!Event.current.control && !Event.current.command))
					{
						foreach(var selectableContainer in selectableContainers)
							selectableContainer.ResetSelection();
					}
	
					if(!Event.current.control && !Event.current.command)
					{
						Selection.activeGameObject = null;
						Selection.activeObject = null;
						Selection.activeTransform = null;
						Selection.objects = new UnityEngine.Object[] { };
					}
					
					HasStartedDrag = false;
					SequenceWindow.Repaint();
				}
				break;
				case EventType.MouseDrag:
				{
					if(!HasStartedDrag)
						HasStartedDrag = true;
					
					SequenceWindow.Repaint();
				}
				break;
				case EventType.MouseUp:
				{
					HasProcessedInitialDrag = false;
					IsBoxSelecting = false;
					IsDragging = false;
					IsDuplicating = false;
					HasDuplicated = false;
					SequenceWindow.Repaint();
				}
				break;
			}
			
			if(IsBoxSelecting && HasStartedDrag && eventType == EventType.MouseDrag)
			{
				foreach(var selectableContainer in selectableContainers)
				{
					var objectsUnderSelection = objectContainerMap[selectableContainer];
					var difference = objectsUnderSelection.Where(selectedObject => !selectableContainer.SelectedObjects.Contains(selectedObject)).ToList();
					if(difference.Count > 0 || (selectableContainer.SelectedObjects.Count != objectsUnderSelection.Count))
					{
						EditorGUI.FocusTextInControl("");
						selectableContainer.ResetSelection();
						selectableContainer.OnSelectedObjects(objectsUnderSelection);
					}
				}
			}
			if((!Event.current.control && !Event.current.command) && hasObjectsUnderMouse && !HasStartedDrag && (eventType == EventType.MouseUp || (eventType == EventType.MouseDown && button == 1)))
			{
				foreach(var selectableContainer in selectableContainers)
				{
					EditorGUI.FocusTextInControl("");
					var objectsUnderSelection = objectContainerMap[selectableContainer];
					selectableContainer.ResetSelection();
					selectableContainer.OnSelectedObjects(objectsUnderSelection);
				}
	
				if(allObjectsUnderMouse.Count == 1)
				{
					var internalKeyframe = allObjectsUnderMouse[0] as USInternalKeyframe;
					if(internalKeyframe)
						SequenceWindow.SetRunningTime(internalKeyframe.Time);
	
					var observerKeyframe = allObjectsUnderMouse[0] as USObserverKeyframe;
					if(observerKeyframe)
						Selection.activeObject = observerKeyframe;
					
					var eventBase = allObjectsUnderMouse[0] as USEventBase;
					if(eventBase)
					{
						Selection.activeGameObject = eventBase.gameObject;
						Selection.activeTransform = eventBase.transform;
					}
				}
			}
			else if((Event.current.control || Event.current.command) && hasObjectsUnderMouse && !HasStartedDrag && eventType == EventType.MouseUp)
			{
				foreach(var selectableContainer in selectableContainers)
				{
					var objectsUnderSelection = objectContainerMap[selectableContainer];
					foreach(var selectedObject in objectsUnderSelection)
					{
						if(!selectableContainer.SelectedObjects.Contains(selectedObject))
							selectableContainer.OnSelectedObjects(new List<UnityEngine.Object> { selectedObject });
						else
							selectableContainer.OnDeSelectedObjects(new List<UnityEngine.Object> { selectedObject });
					}
				}
			}
			else if(IsDragging && HasStartedDrag)
			{
				DragStartPosition = new Vector2(DragStartPosition.x, DragStartPosition.y);
				Vector2 mouseDelta = Event.current.mousePosition - DragStartPosition;

				if(!HasProcessedInitialDrag)
				{
					foreach(var selectableContainer in selectableContainers)
					{
						// if our under mouse isn't currently selected, clear our selection
						var objectsUnderSelection = objectContainerMap[selectableContainer];
						if(objectsUnderSelection.Count == 1 && !selectableContainer.SelectedObjects.Contains(objectsUnderSelection[0]) && !Event.current.control && !Event.current.command)
						{
							selectableContainer.ResetSelection();
							EditorGUI.FocusTextInControl("");
						}
						selectableContainer.OnSelectedObjects(objectsUnderSelection);
						selectableContainer.StartDraggingObjects();
					}
					
					HasProcessedInitialDrag = true;
				}

				if(IsDuplicating && !HasDuplicated)
				{
					foreach(var selectableContainer in selectableContainers)
						selectableContainer.DuplicateSelection();

					HasDuplicated = true;
				}
				else
				{
					foreach(var selectableContainer in selectableContainers)
						selectableContainer.ProcessDraggingObjects(mouseDelta, Snap, TimeToContentX(SnapAmount));
				}
			}
			else if(!HasStartedDrag && !hasObjectsUnderMouse && eventType == EventType.MouseUp)
			{
				foreach(var selectableContainer in selectableContainers)
				{
					selectableContainer.ResetSelection();
				}
				
				Rect selectableArea = USHierarchy.VisibleArea;
				if(selectableArea.Contains(Event.current.mousePosition))
				{
					EditorGUI.FocusTextInControl("");
				}
			}
		}
	
		public void AddNewTimelineContainer(USTimelineContainer timelineContainer)
		{
			var newHierarchyItem = ScriptableObject.CreateInstance(typeof(USTimelineContainerHierarchyItem)) as USTimelineContainerHierarchyItem;
			USUndoManager.RegisterCreatedObjectUndo(newHierarchyItem, "Add New Timeline Container");
			newHierarchyItem.SetupWithTimelineContainer(timelineContainer);
			USUndoManager.PropertyChange(USHierarchy, "Add New Timeline Container");
			USHierarchy.AddHierarchyItemToRoot(newHierarchyItem as IUSHierarchyItem);
		}
		
		private float TimeToContentX(float time)
		{
			return (time / ZoomInfo.meaningOfEveryMarker * ZoomInfo.currentXMarkerDist) - ScrollInfo.currentScroll.x;
		}
		
		private float ContentXToTime(float pos)
		{
			return (((pos + scrollInfo.currentScroll.x - ScrubArea.x) / ScrubArea.width) * CurrentSequence.Duration) / (totalPixelWidthOfTimeline / ScrubArea.width);
		}
		
		public void StoreBaseState()
		{
			USHierarchy.StoreBaseState();
		}
		
		public void RestoreBaseState()
		{
			if(USHierarchy)
				USHierarchy.RestoreBaseState();
		}
		
		public void ExternalModification()
		{
			if(USHierarchy)
				USHierarchy.ExternalModification();
		}
	
		public void OnSceneGUI()
		{
			if(USHierarchy)
				USHierarchy.OnSceneGUI();
		}
	}
}