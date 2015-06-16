using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

namespace WellFired.UI
{
	public class WindowStack 
	{
		private Stack<IWindow> windowStack = new Stack<IWindow>();
		private Canvas rootCanvas;
		private EventSystem eventSystem;

		public WindowStack(Canvas canvas, EventSystem eventSystem)
		{
			this.rootCanvas = canvas;
			this.eventSystem = eventSystem;
		}

		public IWindow OpenWindow(Type windowTypeToOpen)
		{
			var windowType = windowTypeToOpen.Name.ToString();
			var windowPath = string.Format("UI/Window/{0}/{1}", windowType, windowType);

			// Instantiate our object and get the components we need
			var loadedObject = Resources.Load(windowPath) as GameObject;
			var instantiatedObject = GameObject.Instantiate(loadedObject) as GameObject;
			var rectTransform = instantiatedObject.GetComponent<RectTransform>();
			var window = instantiatedObject.GetComponent<Window>();

			// Put our transform back to how it was in the prefab after re parenting.
			instantiatedObject.transform.parent = rootCanvas.transform;
			rectTransform.offsetMin = loadedObject.GetComponent<RectTransform>().offsetMin;
			rectTransform.offsetMax = loadedObject.GetComponent<RectTransform>().offsetMax;

			// Alter the Z Position if needed.
			if(windowStack.Count > 0 &&  windowStack.Peek() != default(IWindow))
			{
				var peekRectTransform = (windowStack.Peek() as MonoBehaviour).GetComponent<RectTransform>();
				var peekedLocalZ = peekRectTransform.localPosition.z;

				var currentLocalPosition = rectTransform.localPosition;
				currentLocalPosition.z = peekedLocalZ - 5.0f;
				rectTransform.localPosition = currentLocalPosition;
			}

			PushWindow(window);

			window.WindowStack = this;

			if(eventSystem != default(EventSystem))
			{
				if(window.FirstSelectedGameObject != null)
				{
					eventSystem.SetSelectedGameObject(window.FirstSelectedGameObject);
				}
			}

			return windowStack.Peek();
		}

		public void CloseWindow(IWindow window)
		{
			if(windowStack.Peek() != window)
			{
				throw new Exception (string.Format ("Trying to pop a window {0} that is not at the head of the stack", window.GetType ()));
			}

			PopWindow();
		}

		private void PushWindow(IWindow window)
		{
			windowStack.Push(window);
		}

		private void PopWindow()
		{
			windowStack.Pop();
		}
	}
}