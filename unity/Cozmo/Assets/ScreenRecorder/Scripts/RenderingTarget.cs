//----------------------------------------------
//            ScreenRecorder
// Copyright © 2011-2012 Andriy Grygorenko
//----------------------------------------------

using UnityEngine;
using System.Collections;

/** RenderingTarget Class manages ScreenRecorder RenderTexture.
 *It allocates RenderTexture with game resolution (Screen.width,Screen.height) at ScreenRecorder.Awake and destroy at ScreenRecorder.OnDestroy.
 *Instance of this class can be accessed thought ScreenRecorder.instance.renderingTarget
*/
public class RenderingTarget : ScriptableObject
{
	#region PRIVATE_MEMBERS
	private FilterMode targetTextureFilterMode = FilterMode.Bilinear;
	private RenderTexture mTargetTexture;
	#endregion
	
	
	#region PUBLIC_PROPS
	/** RenderTexture accessor. 
	 *@return instance of RenderTexture.
	 */
	public RenderTexture target {
		get {
			return mTargetTexture;
		}
	}
	
	/** RenderTexture native id (OpenGL id). 
	 *@return renderTexture native id.
	 */
	public int targetID {
		get {
			if (mTargetTexture != null)
				return mTargetTexture.GetNativeTextureID();
			else
				return -1;
		}
	}
	#endregion

	#region PUBLIC_METHODS
	/** Initializes RenderingTarget and allocate RenderTexture
	 */
	public void Initialize (FilterMode filterMode = FilterMode.Bilinear)
	{
		targetTextureFilterMode = filterMode;
		createTarget ();
	}
	
	/** ScriptableObject callback that is used to destroy RenderingTarget
	 */
	void OnDestroy ()
	{
		destroyTarget ();
	}

	/** Blit content of RenderingTarget on screen
	 */
	public void blit ()
	{
		if (mTargetTexture) {
			Graphics.Blit (mTargetTexture, (RenderTexture)null);
		}
	}
	
	/**  Destroy RenderingTarget
	 */
	public void destroyTarget ()
	{
		if (mTargetTexture != null) {
			mTargetTexture.Release ();
			Object.DestroyImmediate (mTargetTexture);
			mTargetTexture = null;
		}
	}
	#endregion
	
	#region PRIVATE_METHODS
	private Resolution textureResolution {
		get {
			Resolution resolution = new Resolution();
			resolution.width = Screen.width;
			resolution.height = Screen.height;
			return resolution;
		}
	}
	
	private void createTarget ()
	{
		if (mTargetTexture != null)
			return;
		
		mTargetTexture = new RenderTexture (textureResolution.width, 
											textureResolution.height, 
											24, 
											RenderTextureFormat.Default, 
											RenderTextureReadWrite.Default);
		mTargetTexture.useMipMap = false;
		mTargetTexture.isPowerOfTwo = false;
		mTargetTexture.filterMode = targetTextureFilterMode;
		mTargetTexture.isCubemap = false;
		mTargetTexture.Create ();
	}
	#endregion
}
