using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	/// <summary>
	/// This is a common interface that is used by uSequencer to ensure that you can select objects in your custom timelines.
	/// </summary>
	public interface ISelectableContainer
	{
		/// <summary>
		/// A Cached List of all currently selected objects.
		/// </summary>
		/// <value>The selected objects.</value>
		List<UnityEngine.Object> SelectedObjects
		{
			get;
			set;
		}

		/// <summary>
		/// Implement this interface method to duplicate anything that is selected in the uSequencer window, Duplication happens when the user holds down the Alt + Ctrl keys and drags a selection.
		/// </summary>
		void DuplicateSelection();
		
		/// <summary>
		/// Implement this interface method to delete anything that is currently selected in the uSequencer window.
		/// </summary>
		void DeleteSelection();

		/// <summary>
		/// Implement this interface method to provide custom logic when the user resets their selection (Probably just clear SelectedObjects) 
		/// Note : This method should also deal with Undo / Redo.
		/// </summary>
		void ResetSelection();

		/// <summary>
		/// This callback happens when the user selects some objects in the uSequencer Window, when this happens, you could cache the values in the SelectedObjects List.
		/// Note : This method should also deal with Undo / Redo.
		/// </summary>
		/// <param name="selectedObjects">Selected objects.</param>
		void OnSelectedObjects(List<UnityEngine.Object> selectedObjects);

		/// <summary>
		/// This callback happens when the user deselects some objects in the uSequencer Window, when this happens, you could remove tha values from the SelectedObjects List.
		/// Note : This method should also deal with Undo / Redo.
		/// </summary>
		/// <param name="selectedObjects">Selected objects.</param>
		void OnDeSelectedObjects(List<UnityEngine.Object> selectedObjects);
	
		/// <summary>
		/// This method should return any selectable objects under your mouse position. You can provide custom logic for this.
		/// </summary>
		/// <returns>The selectable object under position.</returns>
		/// <param name="mousePosition">Mouse position.</param>
		List<UnityEngine.Object> GetSelectableObjectUnderPos(Vector2 mousePosition);

		/// <summary>
		/// This method is called when the user is dragging a selection area in the uSequencer window, you should compare the contents of your rendered area with the rect selectionRect.
		/// </summary>
		/// <returns>The selectable objects under the selectionRect.</returns>
		/// <param name="selectionRect">Selection rect.</param>
		/// <param name="yScroll">Y scroll.</param>
		List<UnityEngine.Object> GetSelectableObjectsUnderRect(Rect selectionRect, float yScroll);
	
		/// <summary>
		/// This method is called back when the user is dragging any selected objects. You should implement this method to provide custom logic.
		/// Note : This method should also deal with Undo / Redo.
		/// </summary>
		/// <param name="mouseDelta">Mouse delta.</param>
		/// <param name="snap">If set to <c>true</c> snap.</param>
		/// <param name="snapValue">Snap value.</param>
		void ProcessDraggingObjects(Vector2 mouseDelta, bool snap, float snapValue);

		/// <summary>
		/// This method is called back when the user starts to drag objects in the uSequencer window.
		/// uSequencers implementation of this method will cache all the source positions so that we can easily calculate the delta in the ProcessDraggingObjects method.
		/// </summary>
		void StartDraggingObjects();
	}
}