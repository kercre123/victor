using UnityEngine;
using System.Collections;

namespace WellFired.Shared
{
	public class BaseTransition 
	{
		private RenderTexture IntroRenderTexture 
		{
			get
			{
				if(introRenderTexture == null)
					introRenderTexture = new RenderTexture((int)TransitionHelper.MainGameViewSize.x, (int)TransitionHelper.MainGameViewSize.y, 24);
				
				return introRenderTexture;
			}
			set { ; }
		}
		
		private RenderTexture OutroRenderTexture 
		{
			get
			{
				if(outroRenderTexture == null)
					outroRenderTexture = new RenderTexture((int)TransitionHelper.MainGameViewSize.x, (int)TransitionHelper.MainGameViewSize.y, 24);
				
				return outroRenderTexture;
			}
			set { ; }
		}

		public Camera SourceCamera 
		{
			get { return sourceCamera; }
			set { sourceCamera = value; }
		}
		
		private Camera sourceCamera;
		private Camera destinationCamera;
		private Material renderMaterial;
		private RenderTexture introRenderTexture;
		private RenderTexture outroRenderTexture;
		private bool shouldRender;
		private bool prevIntroCameraState;
		private bool prevOutroCameraState;
		private float ratio;

		public void InitializeTransition(Camera sourceCamera, Camera destinationCamera, TypeOfTransition transitionType)
		{
			if(sourceCamera == null || destinationCamera == null)
				Debug.LogError(string.Format("Cannot create a transition with sourceCamera({0}) and destinationCamera({1}), one of them is null", sourceCamera, destinationCamera));

			renderMaterial = new Material(Resources.Load(string.Format("Transitions/WellFired{0}", transitionType.ToString()), typeof(Material)) as Material);

			if(renderMaterial == null)
				Debug.LogError(string.Format("Couldn't load render material for {0}", transitionType));

			this.sourceCamera = sourceCamera;
			this.destinationCamera = destinationCamera;

			prevIntroCameraState = this.sourceCamera.enabled;
			prevOutroCameraState = this.destinationCamera.enabled;
		}		

		public void ProcessTransitionFromOnGUI()
		{
			if(!shouldRender)
				return;

			renderMaterial.SetTexture("_SecondTex", OutroRenderTexture);
			renderMaterial.SetFloat("_Alpha", ratio);
			Graphics.Blit(IntroRenderTexture, default(RenderTexture), renderMaterial);
		}
		
		
		public void ProcessEventFromNoneOnGUI(float deltaTime, float duration)
		{
			sourceCamera.enabled = false;
			destinationCamera.enabled = false;

			sourceCamera.targetTexture = IntroRenderTexture;
			sourceCamera.Render();
	
			destinationCamera.targetTexture = OutroRenderTexture;
			destinationCamera.Render();

			ratio = 1.0f - (deltaTime / duration);
			shouldRender = true;
		}

		public void TransitionComplete()
		{
			shouldRender = false;

			destinationCamera.enabled = true;
			destinationCamera.targetTexture = null;

			sourceCamera.enabled = false;
			sourceCamera.targetTexture = null;
		}

		public void RevertTransition()
		{
			if(sourceCamera != null)
			{
				sourceCamera.enabled = prevIntroCameraState;
				sourceCamera.targetTexture = null;
			}

			if(destinationCamera != null)
			{
				destinationCamera.enabled = prevOutroCameraState;
				destinationCamera.targetTexture = null;
			}

			RenderTexture.DestroyImmediate(IntroRenderTexture);
			RenderTexture.DestroyImmediate(OutroRenderTexture);
			
			shouldRender = false;
		}
	}
}