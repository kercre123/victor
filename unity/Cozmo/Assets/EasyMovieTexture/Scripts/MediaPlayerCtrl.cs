

using UnityEngine;
using System.Runtime.InteropServices;
using System;
using System.Collections;
using UnityEngine.UI;




public class MediaPlayerCtrl : MonoBehaviour {
	
	public string m_strFileName;
	public GameObject [] m_TargetMaterial = null; 
	private Texture2D m_VideoTexture = null;
	private Texture2D m_VideoTextureDummy = null;
	private MEDIAPLAYER_STATE m_CurrentState;
	private int m_iCurrentSeekPosition;
	private float m_fVolume = 1.0f;
    private int m_iWidth;
    private int m_iHeight;

    public bool m_bFullScreen = false;//Please use only in FullScreen prefab.
    public bool m_bSupportRockchip = true; //Using a device support Rochchip or Low-end devices
                                           //(Reason 1 : Not directly play in StreamingAssets)
                                           //(Reason 2 : Video buffer is RGB565 only supported)

    public delegate void VideoEnd();
    public delegate void VideoReady();
    public delegate void VideoError(MEDIAPLAYER_ERROR errorCode, MEDIAPLAYER_ERROR errorCodeExtra);
    public delegate void VideoFirstFrameReady();
	public delegate void VideoResize ();


	public VideoResize OnResize;
    public VideoReady OnReady;
    public VideoEnd OnEnd;
    public VideoError OnVideoError;
    public VideoFirstFrameReady OnVideoFirstFrameReady;

#if UNITY_IPHONE || UNITY_TVOS
	
	private int m_iPauseFrame;
#endif





#if UNITY_ANDROID && !UNITY_EDITOR && UNITY_5
	[DllImport ("BlueDoveMediaRender")]
	private static extern void InitNDK();

// DHC: Changed to fix warnings
//#if UNITY_5_2
#if UNITY_5_3_OR_NEWER
	[DllImport ("BlueDoveMediaRender")]
	private static extern IntPtr EasyMovieTextureRender();
#endif
#endif



  // DHC: Added to fix warnings
  #if UNITY_ANDROID && !UNITY_EDITOR && UNITY_5
	private int m_iAndroidMgrID;
  #endif
	private bool m_bIsFirstFrameReady;

    public enum MEDIAPLAYER_ERROR
    {
        MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK = 200,
        MEDIA_ERROR_IO = -1004,
        MEDIA_ERROR_MALFORMED = -1007,
        MEDIA_ERROR_TIMED_OUT = -110,
        MEDIA_ERROR_UNSUPPORTED = -1010,
        MEDIA_ERROR_SERVER_DIED = 100,
        MEDIA_ERROR_UNKNOWN = 1
    }

    public enum MEDIAPLAYER_STATE
    {
        NOT_READY = 0,
        READY = 1,
        END = 2,
        PLAYING = 3,
        PAUSED = 4,
        STOPPED = 5,
        ERROR = 6
    }

    public enum MEDIA_SCALE
    {
        SCALE_X_TO_Y = 0,
        SCALE_X_TO_Z = 1,
        SCALE_Y_TO_X = 2,
        SCALE_Y_TO_Z = 3,
        SCALE_Z_TO_X = 4,
        SCALE_Z_TO_Y = 5,
        SCALE_X_TO_Y_2 = 6,
    }

    bool m_bFirst = false;

    public MEDIA_SCALE m_ScaleValue;
    public GameObject[] m_objResize = null;
    public bool m_bLoop = false;
    public bool m_bAutoPlay = true;
    private bool m_bStop = false;

	public bool m_bInit = false;
	
	
	
	void Awake(){
		



		
		if( SystemInfo.deviceModel.Contains("rockchip"))
		{
			m_bSupportRockchip = true;
		}
		else
		{
			m_bSupportRockchip = false;
		}


		#if UNITY_IPHONE || UNITY_TVOS
		
		if(m_TargetMaterial!=null)
		{

			for( int iIndex = 0; iIndex < m_TargetMaterial.Length; iIndex++)
			{
				if( m_TargetMaterial[iIndex] != null)
				{
					if(m_TargetMaterial[iIndex].GetComponent<MeshFilter>() != null)
					{
						Vector2 [] vec2UVs= m_TargetMaterial[iIndex].GetComponent<MeshFilter>().mesh.uv;
						
						for(int i = 0; i < vec2UVs.Length; i++)
						{
							vec2UVs[i] = new Vector2(vec2UVs[i].x, 1.0f -vec2UVs[i].y);
						}
						
						m_TargetMaterial[iIndex].GetComponent<MeshFilter>().mesh.uv = vec2UVs;
					}
					
					if(m_TargetMaterial[iIndex].GetComponent<RawImage>() != null)
					{
						m_TargetMaterial[iIndex].GetComponent<RawImage>().uvRect = new Rect(0,1,1,-1);
					}
				}

			}


		}

		
		
		#endif		



		
		
	}
	// Use this for initialization
	void Start () {


		
		#if UNITY_ANDROID && !UNITY_EDITOR 
		
		#if UNITY_5
		if( SystemInfo.graphicsMultiThreaded == true)
			InitNDK();
		#endif
		m_iAndroidMgrID = Call_InitNDK();
		#endif
		
		Call_SetUnityActivity();
		
		
		
		
		
		
		
		#if UNITY_ANDROID 
		if (Application.dataPath.Contains(".obb")) {
			
			Call_SetSplitOBB(true,Application.dataPath);
		}
		else
		{
			Call_SetSplitOBB(false, null);
		}
		#endif

		m_bInit = true;
		
	}
	
	
	void OnApplicationQuit()
	{
		
		if( System.IO.Directory.Exists( Application.persistentDataPath + "/Data" ) == true)
			System.IO.Directory.Delete( Application.persistentDataPath + "/Data", true);
	}
	
	bool m_bCheckFBO = false;

	void OnDisable() 
	{
		if( GetCurrentState() == MEDIAPLAYER_STATE.PLAYING)
		{
			Pause();
		}
	}

	void OnEnable()
	{
		if( GetCurrentState() == MEDIAPLAYER_STATE.PAUSED)
		{
			Play();
		}
	}
	
	void Update()
	{
		
		if( string.IsNullOrEmpty(m_strFileName) )
		{
			return;
		}
		
		
		if(m_bFirst == false)
		{
			
			
			
			
      // DHC: Added to fix warnings
      #if UNITY_IPHONE || UNITY_ANDROID
			string strName = m_strFileName.Trim();
      #endif
			
			#if UNITY_IPHONE || UNITY_TVOS
			/*if (strName.StartsWith("http",StringComparison.OrdinalIgnoreCase))
			{
				StartCoroutine( DownloadStreamingVideoAndLoad(strName) );
			}
			else*/
			{
				Call_Load(strName,0);
			}
			
			#endif
			#if UNITY_ANDROID 
			
			if(m_bSupportRockchip)
			{
				Call_SetRockchip(m_bSupportRockchip);
				
				if(strName.Contains("://"))
				{
					Call_Load(strName,0);
				}
				else
				{
					//Call_Load(strName,0);
					StartCoroutine( CopyStreamingAssetVideoAndLoad(strName));
				}
				
			}
			else
			{
				Call_Load(strName,0);
			}
			
			
			#endif
			Call_SetLooping(m_bLoop);
			m_bFirst = true;
			
			
		}
		
		
		if(m_CurrentState == MEDIAPLAYER_STATE.PLAYING || m_CurrentState == MEDIAPLAYER_STATE.PAUSED)
		{
			if(m_bCheckFBO == false)
			{


				if(Call_GetVideoWidth() <= 0 || Call_GetVideoHeight() <= 0)
				{
					return;
				}
				
				m_iWidth = Call_GetVideoWidth ();
				m_iHeight = Call_GetVideoHeight ();
				Resize();
				
				if(m_VideoTexture != null)
				{
					
					//Destroy(m_VideoTexture);
					
					if( m_VideoTextureDummy != null)
					{
						Destroy(m_VideoTextureDummy);
						m_VideoTextureDummy = null;
					}
					
					m_VideoTextureDummy = m_VideoTexture;
					m_VideoTexture = null;
					
					
				}
				
				#if UNITY_ANDROID
				if(m_bSupportRockchip)
				{
					m_VideoTexture = new Texture2D(Call_GetVideoWidth(),Call_GetVideoHeight(),TextureFormat.RGB565,false);
					
				}
				else
				{
					m_VideoTexture = new Texture2D(Call_GetVideoWidth(),Call_GetVideoHeight(),TextureFormat.RGBA32,false);
				}
				
				m_VideoTexture.filterMode = FilterMode.Bilinear;
				m_VideoTexture.wrapMode = TextureWrapMode.Clamp;

// DHC: Changed to fix warnings
//#if UNITY_5_2
#if UNITY_5_3_OR_NEWER
				Call_SetUnityTexture((int)m_VideoTexture.GetNativeTexturePtr());
#else
				Call_SetUnityTexture(m_VideoTexture.GetNativeTextureID());
#endif
				#endif
				Call_SetWindowSize();
				m_bCheckFBO = true;
				if (OnResize != null)
					OnResize ();
			} else {
				if (Call_GetVideoWidth () != m_iWidth || Call_GetVideoHeight () != m_iHeight) {
					m_iWidth = Call_GetVideoWidth ();
					m_iHeight = Call_GetVideoHeight ();

					ResizeTexture ();
				}
			}
			
			
			Call_UpdateVideoTexture();
			
			
			
			m_iCurrentSeekPosition = Call_GetSeekPosition();
			
			


        }



        if (m_CurrentState != Call_GetStatus())
        {

            m_CurrentState = Call_GetStatus();



            if (m_CurrentState == MEDIAPLAYER_STATE.READY)
            {

                if (OnReady != null)
                    OnReady();

                if (m_bAutoPlay)
                    Call_Play(0);

                SetVolume(m_fVolume);



            }
            else if (m_CurrentState == MEDIAPLAYER_STATE.END)
            {
                if (OnEnd != null)
                    OnEnd();

                if (m_bLoop == true)
                {
                    Call_Play(0);
                }
            }
            else if (m_CurrentState == MEDIAPLAYER_STATE.ERROR)
            {
                OnError((MEDIAPLAYER_ERROR)Call_GetError(), (MEDIAPLAYER_ERROR)Call_GetErrorExtra());
            }

        }

		GL.InvalidateState ();


    }

	public void ResizeTexture()
	{
		Debug.Log ("ResizeTexture " + m_iWidth + " " + m_iHeight);

		if (m_VideoTexture != null) {

			//Destroy(m_VideoTexture);

			if (m_VideoTextureDummy != null) {
				Destroy (m_VideoTextureDummy);
				m_VideoTextureDummy = null;
			}

			m_VideoTextureDummy = m_VideoTexture;
			m_VideoTexture = null;


		}

	#if UNITY_ANDROID || UNITY_EDITOR || UNITY_STANDALONE
		if (m_bSupportRockchip) {
			m_VideoTexture = new Texture2D (Call_GetVideoWidth (), Call_GetVideoHeight (), TextureFormat.RGB565, false);

		} else {
			m_VideoTexture = new Texture2D (Call_GetVideoWidth (), Call_GetVideoHeight (), TextureFormat.RGBA32, false);
		}

		m_VideoTexture.filterMode = FilterMode.Bilinear;
		m_VideoTexture.wrapMode = TextureWrapMode.Clamp;

  // DHC: Changed to fix warnings
  //#if UNITY_5_2
  #if UNITY_5_3_OR_NEWER
	Call_SetUnityTexture((int)m_VideoTexture.GetNativeTexturePtr());
	#else
		Call_SetUnityTexture (m_VideoTexture.GetNativeTextureID ());
	#endif
	#endif

		Call_SetWindowSize ();
	}

    public void Resize()
    {
        if (m_CurrentState != MEDIAPLAYER_STATE.PLAYING)
            return;

        if (Call_GetVideoWidth() <= 0 || Call_GetVideoHeight() <= 0)
        {
            return;
        }

        if (m_objResize != null)
        {
            int iScreenWidth = Screen.width;
            int iScreenHeight = Screen.height;

            float fRatioScreen = (float)iScreenHeight / (float)iScreenWidth;
            int iWidth = Call_GetVideoWidth();
            int iHeight = Call_GetVideoHeight();

            float fRatio = (float)iHeight / (float)iWidth;
            float fRatioResult = fRatioScreen / fRatio;

            for (int i = 0; i < m_objResize.Length; i++)
            {
                if (m_objResize[i] == null)
                    continue;

                if (m_bFullScreen)
                {



                    m_objResize[i].transform.localScale = new Vector3(20.0f / fRatioScreen, 20.0f / fRatioScreen, 1.0f);
                    if (fRatio < 1.0f)
                    {
                        if (fRatioScreen < 1.0f)
                        {
                            if (fRatio > fRatioScreen)
                            {
                                m_objResize[i].transform.localScale *= fRatioResult;
                            }
                        }

                        m_ScaleValue = MEDIA_SCALE.SCALE_X_TO_Y;
                    }
                    else
                    {
                        if (fRatioScreen > 1.0f)
                        {
                            if (fRatio >= fRatioScreen)
                            {
                                m_objResize[i].transform.localScale *= fRatioResult;
                            }
                        }

						m_ScaleValue = MEDIA_SCALE.SCALE_X_TO_Y;
                    }
                }



                if (m_ScaleValue == MEDIA_SCALE.SCALE_X_TO_Y)
                {
                    m_objResize[i].transform.localScale
                        = new Vector3(m_objResize[i].transform.localScale.x
                                      , m_objResize[i].transform.localScale.x * fRatio
                                      , m_objResize[i].transform.localScale.z);
                }
                else if (m_ScaleValue == MEDIA_SCALE.SCALE_X_TO_Y_2)
                {
                    m_objResize[i].transform.localScale
                        = new Vector3(m_objResize[i].transform.localScale.x
                                      , m_objResize[i].transform.localScale.x * fRatio / 2.0f
                                      , m_objResize[i].transform.localScale.z);
                }
                else if (m_ScaleValue == MEDIA_SCALE.SCALE_X_TO_Z)
                {
                    m_objResize[i].transform.localScale
                        = new Vector3(m_objResize[i].transform.localScale.x
                                      , m_objResize[i].transform.localScale.y
                                      , m_objResize[i].transform.localScale.x * fRatio);
                }
                else if (m_ScaleValue == MEDIA_SCALE.SCALE_Y_TO_X)
                {
                    m_objResize[i].transform.localScale
                        = new Vector3(m_objResize[i].transform.localScale.y / fRatio
                                      , m_objResize[i].transform.localScale.y
                                      , m_objResize[i].transform.localScale.z);
                }
                else if (m_ScaleValue == MEDIA_SCALE.SCALE_Y_TO_Z)
                {
                    m_objResize[i].transform.localScale
                        = new Vector3(m_objResize[i].transform.localScale.x
                                      , m_objResize[i].transform.localScale.y
                                      , m_objResize[i].transform.localScale.y / fRatio);
                }
                else if (m_ScaleValue == MEDIA_SCALE.SCALE_Z_TO_X)
                {
                    m_objResize[i].transform.localScale
                        = new Vector3(m_objResize[i].transform.localScale.z * fRatio
                                      , m_objResize[i].transform.localScale.y
                                      , m_objResize[i].transform.localScale.z);
                }
                else if (m_ScaleValue == MEDIA_SCALE.SCALE_Z_TO_Y)
                {
                    m_objResize[i].transform.localScale
                        = new Vector3(m_objResize[i].transform.localScale.x
                                      , m_objResize[i].transform.localScale.z * fRatio
                                      , m_objResize[i].transform.localScale.z);
                }
                else
                {
                    m_objResize[i].transform.localScale
                        = new Vector3(m_objResize[i].transform.localScale.x, m_objResize[i].transform.localScale.y, m_objResize[i].transform.localScale.z);
                }
            }

        }
    }



    //The error code is the following sites related documents.
    //http://developer.android.com/reference/android/media/MediaPlayer.OnErrorListener.html 
    void OnError(MEDIAPLAYER_ERROR iCode, MEDIAPLAYER_ERROR iCodeExtra)
    {
        string strError = "";

        switch (iCode)
        {
            case MEDIAPLAYER_ERROR.MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK:
                strError = "MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK";
                break;
            case MEDIAPLAYER_ERROR.MEDIA_ERROR_SERVER_DIED:
                strError = "MEDIA_ERROR_SERVER_DIED";
                break;
            case MEDIAPLAYER_ERROR.MEDIA_ERROR_UNKNOWN:
                strError = "MEDIA_ERROR_UNKNOWN";
                break;
            default:
                strError = "Unknown error " + iCode;
                break;
        }

        strError += " ";

        switch (iCodeExtra)
        {
            case MEDIAPLAYER_ERROR.MEDIA_ERROR_IO:
                strError += "MEDIA_ERROR_IO";
                break;
            case MEDIAPLAYER_ERROR.MEDIA_ERROR_MALFORMED:
                strError += "MEDIA_ERROR_MALFORMED";
                break;
            case MEDIAPLAYER_ERROR.MEDIA_ERROR_TIMED_OUT:
                strError += "MEDIA_ERROR_TIMED_OUT";
                break;
            case MEDIAPLAYER_ERROR.MEDIA_ERROR_UNSUPPORTED:
                strError += "MEDIA_ERROR_UNSUPPORTED";
                break;
            default:
                strError = "Unknown error " + iCode;
                break;
        }



        Debug.LogError(strError);

        if (OnVideoError != null)
        {
            OnVideoError(iCode, iCodeExtra);
        }
    }


    void OnDestroy()
    {


#if UNITY_ANDROID
		
		//Call_Reset();
		
		#endif
		Call_UnLoad();
		
		if(m_VideoTextureDummy != null)
		{
			Destroy(m_VideoTextureDummy);
			m_VideoTextureDummy = null;
		}
		
		if(m_VideoTexture != null)
			Destroy(m_VideoTexture);
		
		Call_Destroy();
		
		
	}

	bool m_bPause = false;
	
	void OnApplicationPause(bool bPause)
	{
		Debug.Log ("ApplicationPause : " +bPause);
		if(bPause == true)
		{

			if( m_CurrentState == MEDIAPLAYER_STATE.PAUSED)
			{
				m_bPause = true;
			}

#if (UNITY_IPHONE  || UNITY_TVOS) 
			m_iPauseFrame = m_iCurrentSeekPosition;
			Stop();
			
			
			#else
			Call_Pause();
			#endif
			
			
		}
		else
		{
#if ( UNITY_IPHONE  || UNITY_TVOS )
			m_bStop = false;
			Call_Play(m_iPauseFrame);
			#else
			Call_RePlay ();
			#endif

			if( m_bPause == true)
			{
				Call_Pause();
				m_bPause = false;
			}
			
		}
		
	}
	
	
	public MEDIAPLAYER_STATE GetCurrentState()
	{
		return m_CurrentState;
	}
	
	public Texture2D GetVideoTexture()
	{
		return m_VideoTexture;
	}
	
	public void Play()
	{
		if(m_bStop == true)
		{
			Call_Play(0);
			m_bStop = false;
		}
		
		if(m_CurrentState == MEDIAPLAYER_STATE.PAUSED  )
		{
			Call_RePlay();
		}
		else if( m_CurrentState == MEDIAPLAYER_STATE.READY || m_CurrentState == MEDIAPLAYER_STATE.STOPPED || m_CurrentState == MEDIAPLAYER_STATE.END)
		{
			Call_Play(0);
		}
	}
	
	public void Stop()
	{
		if(m_CurrentState == MEDIAPLAYER_STATE.PLAYING)
			Call_Pause();
		
		
		m_bStop = true;
		m_CurrentState = MEDIAPLAYER_STATE.STOPPED;
		m_iCurrentSeekPosition = 0;
	}
	
	public void Pause()
	{
		if(m_CurrentState == MEDIAPLAYER_STATE.PLAYING)
			Call_Pause();
		
		m_CurrentState = MEDIAPLAYER_STATE.PAUSED;
	}
	
	public void Load(string strFileName)
	{
		if( GetCurrentState() != MEDIAPLAYER_STATE.NOT_READY)
			UnLoad();


    // DHC: Added to fix warnings
    #if UNITY_ANDROID && !UNITY_EDITOR && UNITY_5
		m_bIsFirstFrameReady = false;
    #endif

		m_bFirst = false;
		m_bCheckFBO = false;
		
		m_strFileName = strFileName;

		if( m_bInit == false)
			return;
		
		
		m_CurrentState = MEDIAPLAYER_STATE.NOT_READY;
	}
	
	
	
	public void SetVolume(float fVolume)
	{
		if( m_CurrentState == MEDIAPLAYER_STATE.PLAYING || m_CurrentState == MEDIAPLAYER_STATE.PAUSED  || m_CurrentState == MEDIAPLAYER_STATE.END || m_CurrentState == MEDIAPLAYER_STATE.READY || m_CurrentState == MEDIAPLAYER_STATE.STOPPED)
		{
			m_fVolume = fVolume;
			Call_SetVolume(fVolume);
		}
	}
	
	//return milisecond  
	public int GetSeekPosition()
	{

		if( m_CurrentState == MEDIAPLAYER_STATE.PLAYING || m_CurrentState == MEDIAPLAYER_STATE.PAUSED  || m_CurrentState == MEDIAPLAYER_STATE.END )
			return m_iCurrentSeekPosition;
		else 
			return 0;
	}
	
	public void SeekTo(int iSeek)
	{
		if( m_CurrentState == MEDIAPLAYER_STATE.PLAYING|| m_CurrentState == MEDIAPLAYER_STATE.READY  || m_CurrentState == MEDIAPLAYER_STATE.PAUSED  || m_CurrentState == MEDIAPLAYER_STATE.END || m_CurrentState == MEDIAPLAYER_STATE.STOPPED)
			Call_SetSeekPosition(iSeek);

    }


    //Gets the duration of the file.
    //Returns
    //the duration in milliseconds, if no duration is available (for example, if streaming live content), -1 is returned.
    public int GetDuration()
    {
        if (m_CurrentState == MEDIAPLAYER_STATE.PLAYING || m_CurrentState == MEDIAPLAYER_STATE.PAUSED || m_CurrentState == MEDIAPLAYER_STATE.END || m_CurrentState == MEDIAPLAYER_STATE.READY || m_CurrentState == MEDIAPLAYER_STATE.STOPPED)
            return Call_GetDuration();
        else
            return 0;
    }

	public float GetSeekBarValue()
	{
		if (m_CurrentState == MEDIAPLAYER_STATE.PLAYING || m_CurrentState == MEDIAPLAYER_STATE.PAUSED || m_CurrentState == MEDIAPLAYER_STATE.END || m_CurrentState == MEDIAPLAYER_STATE.READY || m_CurrentState == MEDIAPLAYER_STATE.STOPPED)
		{
			if (GetDuration () == 0) {
				return 0;
			}
			return (float)GetSeekPosition() / (float)GetDuration() ;
		}
		else
			return 0;
	}

	public void SetSeekBarValue(float fValue)
	{
		if (m_CurrentState == MEDIAPLAYER_STATE.PLAYING || m_CurrentState == MEDIAPLAYER_STATE.PAUSED || m_CurrentState == MEDIAPLAYER_STATE.END || m_CurrentState == MEDIAPLAYER_STATE.READY || m_CurrentState == MEDIAPLAYER_STATE.STOPPED)
		{
			if (GetDuration () == 0) {
				return;
			}

			SeekTo( (int)((float)GetDuration() * fValue) );
		}
		else
			return;
	}

    //Get update status in buffering a media stream received through progressive HTTP download. 
    //The received buffering percentage indicates how much of the content has been buffered or played. 
    //For example a buffering update of 80 percent when half the content has already been played indicates that the next 30 percent of the content to play has been buffered.
    //the percentage (0-100) of the content that has been buffered or played thus far 
    public int GetCurrentSeekPercent()
    {
        if (m_CurrentState == MEDIAPLAYER_STATE.PLAYING || m_CurrentState == MEDIAPLAYER_STATE.PAUSED || m_CurrentState == MEDIAPLAYER_STATE.END || m_CurrentState == MEDIAPLAYER_STATE.READY)
            return Call_GetCurrentSeekPercent();
        else
            return 0;
    }

    public int GetVideoWidth()
    {
        return Call_GetVideoWidth();
    }

    public int GetVideoHeight()
    {
        return Call_GetVideoHeight();
    }

    public void UnLoad()
    {
        m_bCheckFBO = false;
#if UNITY_ANDROID
		
		//Call_Reset();
		#endif
		Call_UnLoad();
		
		m_CurrentState = MEDIAPLAYER_STATE.NOT_READY;
		
	}

	

	#if !UNITY_EDITOR && !UNITY_STANDALONE
	#if UNITY_ANDROID
	
	private AndroidJavaObject javaObj = null;
	
	private AndroidJavaObject GetJavaObject()
	{
		if (javaObj == null)
		{
			javaObj = new AndroidJavaObject("com.EasyMovieTexture.EasyMovieTexture");
		}
		
		return javaObj;
	}
	
	
	
	
	private void Call_Destroy()
	{
		#if UNITY_5
		if( SystemInfo.graphicsMultiThreaded == true)
		{
// DHC: Changed to fix warnings
//#if UNITY_5_2
#if UNITY_5_3_OR_NEWER
			GL.IssuePluginEvent(EasyMovieTextureRender(), 5 + m_iAndroidMgrID * 10 + 7000);
#else
			GL.IssuePluginEvent(5 + m_iAndroidMgrID * 10 + 7000);
#endif

		}
		else
		{
			GetJavaObject().Call("Destroy");
		}
		#else
		GetJavaObject().Call("Destroy");
		#endif
		
		
	}
	
	
	private void Call_UnLoad()
	{
		#if UNITY_5
		if( SystemInfo.graphicsMultiThreaded == true)
		{


      // DHC: Changed to fix warnings
      //#if UNITY_5_2
      #if UNITY_5_3_OR_NEWER
			GL.IssuePluginEvent(EasyMovieTextureRender(), 4 + m_iAndroidMgrID * 10 + 7000);
			#else
			GL.IssuePluginEvent(4 + m_iAndroidMgrID * 10 + 7000);
			#endif
		}
		else
		{
			GetJavaObject().Call("UnLoad");
		}
		#else
		GetJavaObject().Call("UnLoad");
		#endif
		
		
	}
	
	private bool Call_Load(string strFileName, int iSeek)
	{
		#if UNITY_5
		if( SystemInfo.graphicsMultiThreaded == true)
		{
			GetJavaObject().Call("NDK_SetFileName", strFileName);

      // DHC: Changed to fix warnings
      //#if UNITY_5_2
      #if UNITY_5_3_OR_NEWER
			GL.IssuePluginEvent(EasyMovieTextureRender(), 1 + m_iAndroidMgrID * 10 + 7000);
			#else
			GL.IssuePluginEvent(1+ m_iAndroidMgrID * 10 + 7000);
			#endif


			Call_SetNotReady();
			
			return true;
		}
		else
		{
			GetJavaObject().Call("NDK_SetFileName", strFileName);
			if (GetJavaObject().Call<bool>("Load"))
			{
				return true;
			}
			else
			{
				OnError(MEDIAPLAYER_ERROR.MEDIA_ERROR_UNKNOWN, MEDIAPLAYER_ERROR.MEDIA_ERROR_UNKNOWN);
				return false;
			}
		}
		#else
		GetJavaObject().Call("NDK_SetFileName", strFileName);
		if (GetJavaObject().Call<bool>("Load"))
		{
			return true;
		}
		else
		{
			OnError(MEDIAPLAYER_ERROR.MEDIA_ERROR_UNKNOWN, MEDIAPLAYER_ERROR.MEDIA_ERROR_UNKNOWN);
			return false;
		}
		#endif
		
		
	}
	
	private void Call_UpdateVideoTexture()
	{
		
		if( Call_IsUpdateFrame() == false)
			return;
		
		if(m_VideoTextureDummy != null)
		{
			Destroy(m_VideoTextureDummy);
			m_VideoTextureDummy = null;
		}


		for( int i = 0; i < m_TargetMaterial.Length; i++)
		{
			if(m_TargetMaterial[i])
			{
				if(m_TargetMaterial[i].GetComponent<MeshRenderer>()!= null)
				{
					if(m_TargetMaterial[i].GetComponent<MeshRenderer>().material.mainTexture != m_VideoTexture)
					{
						m_TargetMaterial[i].GetComponent<MeshRenderer>().material.mainTexture = m_VideoTexture;
					}
				}
				
				if(m_TargetMaterial[i].GetComponent<RawImage>()!= null)
				{
					if(m_TargetMaterial[i].GetComponent<RawImage>().texture != m_VideoTexture)
					{
						m_TargetMaterial[i].GetComponent<RawImage>().texture = m_VideoTexture;
					}
				}
				
				
				
			}
		}

		
		#if UNITY_5
		if( SystemInfo.graphicsMultiThreaded == true)
		{
      // DHC: Changed to fix warnings
      //#if UNITY_5_2
      #if UNITY_5_3_OR_NEWER
			GL.IssuePluginEvent(EasyMovieTextureRender(), 3 + m_iAndroidMgrID * 10 + 7000);
			#else
			GL.IssuePluginEvent(3+ m_iAndroidMgrID * 10 + 7000);
			#endif

		
		}
		else
		{
			GetJavaObject().Call("UpdateVideoTexture");
		}
		#else
		GetJavaObject().Call("UpdateVideoTexture");
		#endif
		
		if (!m_bIsFirstFrameReady)
		{
			m_bIsFirstFrameReady = true;
			if (OnVideoFirstFrameReady != null)
			{
				OnVideoFirstFrameReady();
				OnVideoFirstFrameReady = null;
			}
		}
	}
	
	private void Call_SetVolume(float fVolume)
	{
		GetJavaObject().Call("SetVolume",fVolume);
	}
	
	private void Call_SetSeekPosition(int iSeek)
	{
		GetJavaObject().Call("SetSeekPosition",iSeek);
	}
	
	private int Call_GetSeekPosition()
	{
		return GetJavaObject().Call<int>("GetSeekPosition");
	}
	
	private void Call_Play(int iSeek)
	{
		GetJavaObject().Call("Play",iSeek);
	}
	
	private void Call_Reset()
	{
		GetJavaObject().Call("Reset");
	}
	
	private void Call_Stop()
	{
		GetJavaObject().Call("Stop");
	}
	
	private void Call_RePlay()
	{
		GetJavaObject().Call("RePlay");
	}
	
	private void Call_Pause()
	{
		GetJavaObject().Call("Pause");
	}
	
	private int Call_InitNDK()
	{
		return GetJavaObject().Call<int>("InitNative",GetJavaObject());
	}
	
	private int Call_GetVideoWidth()
	{
		return GetJavaObject().Call<int>("GetVideoWidth");
	}
	
	private int Call_GetVideoHeight()
	{
		return GetJavaObject().Call<int>("GetVideoHeight");
	}
	
	private bool Call_IsUpdateFrame()
	{
		return GetJavaObject().Call<bool>("IsUpdateFrame");
	}
	
	private void Call_SetUnityTexture(int iTextureID)
	{
		GetJavaObject().Call("SetUnityTexture",iTextureID);
	}
	
	private void Call_SetWindowSize()
	{
		
		#if UNITY_5
		if( SystemInfo.graphicsMultiThreaded == true)
		{


      // DHC: Changed to fix warnings
      //#if UNITY_5_2
      #if UNITY_5_3_OR_NEWER
			GL.IssuePluginEvent(EasyMovieTextureRender(), 2 + m_iAndroidMgrID * 10 + 7000);
			#else
			GL.IssuePluginEvent(2+ m_iAndroidMgrID * 10 + 7000);
			#endif
		}
		else
		{
			GetJavaObject().Call("SetWindowSize");
		}
		#else
		GetJavaObject().Call("SetWindowSize");
		#endif
	}
	
	private void Call_SetLooping(bool bLoop)
	{
		GetJavaObject().Call("SetLooping",bLoop);
	}
	
	private void Call_SetRockchip(bool bValue)
	{
		GetJavaObject().Call("SetRockchip",bValue);
	}
	
	
	private int Call_GetDuration()
	{
		return GetJavaObject().Call<int>("GetDuration");
	}
	
	private int Call_GetCurrentSeekPercent()
	{
		return GetJavaObject().Call<int>("GetCurrentSeekPercent");
	}
	
	private int Call_GetError()
	{
		return GetJavaObject().Call<int>("GetError");
	}
	
	private void Call_SetSplitOBB(bool bValue, string strOBBName)
	{
		GetJavaObject().Call("SetSplitOBB",bValue,strOBBName);
	}
	
	private int Call_GetErrorExtra()
	{
		return GetJavaObject().Call<int>("GetErrorExtra");
	}
	
	
	private void Call_SetUnityActivity()
	{
		AndroidJavaClass jc = new AndroidJavaClass("com.unity3d.player.UnityPlayer");
		AndroidJavaObject jo = jc.GetStatic<AndroidJavaObject>("currentActivity");
		GetJavaObject().Call("SetUnityActivity", jo);
		
		#if UNITY_5
		if( SystemInfo.graphicsMultiThreaded == true)
		{
      // DHC: Changed to fix warnings
      //#if UNITY_5_2
      #if UNITY_5_3_OR_NEWER
			GL.IssuePluginEvent(EasyMovieTextureRender(), 0 + m_iAndroidMgrID * 10 + 7000);
			#else
			GL.IssuePluginEvent(0+ m_iAndroidMgrID * 10 + 7000);
			#endif
		}
		else
		{
			Call_InitJniManager();
		}
		#else
		Call_InitJniManager();
		#endif
		
	}
	
	private void Call_SetNotReady()
	{
		GetJavaObject().Call("SetNotReady");
	}
	
	private void Call_InitJniManager()
	{
		GetJavaObject().Call("InitJniManager");
	}
	
	private MEDIAPLAYER_STATE Call_GetStatus()
	{
		return (MEDIAPLAYER_STATE)GetJavaObject().Call<int>("GetStatus");
	}
	
	
	
#elif UNITY_IPHONE  || UNITY_TVOS
	[DllImport("__Internal")]
	private static extern int VideoPlayerPluginCreateInstance();
	[DllImport("__Internal")]
	private static extern void VideoPlayerPluginDestroyInstance(int iID);
	[DllImport("__Internal")]
	private static extern bool VideoPlayerPluginCanOutputToTexture(string videoURL);
	[DllImport("__Internal")]
	private static extern void VideoPlayerPluginSetLoop(int iID, bool bLoop);
	[DllImport("__Internal")]
	private static extern void VideoPlayerPluginSetVolume(int iID, float fVolume);
	[DllImport("__Internal")]
	private static extern bool VideoPlayerPluginPlayerReady(int iID);
	[DllImport("__Internal")]
	private static extern float VideoPlayerPluginDurationSeconds(int iID);
	[DllImport("__Internal")]
	private static extern void VideoPlayerPluginExtents(int iID,ref int width, ref int height);
	[DllImport("__Internal")]
	private static extern IntPtr VideoPlayerPluginCurFrameTexture(int iID);
	[DllImport("__Internal")]
	private static extern void VideoPlayerPluginLoadVideo(int iID,string videoURL);
	[DllImport("__Internal")]
	private static extern void VideoPlayerPluginPlayVideo(int iID);
	[DllImport("__Internal")]
	private static extern void VideoPlayerPluginPauseVideo(int iID);
	[DllImport("__Internal")]
	private static extern void VideoPlayerPluginResumeVideo(int iID);
	[DllImport("__Internal")]
	private static extern void VideoPlayerPluginRewindVideo(int iID);
	[DllImport("__Internal")]
	private static extern void VideoPlayerPluginSeekToVideo(int iID,float time);
	[DllImport("__Internal")]
	private static extern float VideoPlayerPluginCurTimeSeconds(int iID);
	[DllImport("__Internal")]
	private static extern bool VideoPlayerPluginIsPlaying(int iID);
	[DllImport("__Internal")]
	private static extern void VideoPlayerPluginStopVideo(int iID);
	[DllImport("__Internal")]
	private static extern bool VideoPlayerPluginFinish(int iID);
	[DllImport("__Internal")]
	private static extern bool VideoPlayerPluginError(int iID);
	[DllImport("__Internal")]
	private static extern void VideoPlayerPluginSetTexture(int iID,int iTextureID);
	
	
	
	
	
	int m_iID = -1;
	/// <summary>
	/// ???????????????true???????    /// </summary>
	public bool ready
	{
		get
		{
			return VideoPlayerPluginPlayerReady(m_iID);
		}
	}
	
	/// <summary>
	/// 
	/// </summary>
	public float duration
	{
		get
		{
			return VideoPlayerPluginDurationSeconds(m_iID);
		}
	}
	
	
	public float currentTime
	{
		get
		{
			return VideoPlayerPluginCurTimeSeconds(m_iID);
		}
	}
	
	
	public bool isPlaying
	{
		get
		{
			return VideoPlayerPluginIsPlaying(m_iID);
		}
	}
	public Vector2 videoSize
	{
		get
		{
			int width = 0, height = 0;
			VideoPlayerPluginExtents(m_iID,ref width, ref height);
			return new Vector2(width, height);
		}
	}
	
	private Texture2D _videoTexture;
	
	public Texture2D videoTexture
	{
		get
		{
			IntPtr nativeTex = ready ? VideoPlayerPluginCurFrameTexture(m_iID) : (IntPtr)0;
			
			
			if (nativeTex != (IntPtr)0)
			{
				if (_videoTexture == null)
				{
					_videoTexture = Texture2D.CreateExternalTexture((int)videoSize.x, (int)videoSize.y, TextureFormat.RGBA32,
					                                                false, false, (IntPtr)nativeTex);
					_videoTexture.filterMode = FilterMode.Bilinear;
					_videoTexture.wrapMode = TextureWrapMode.Clamp;
				}
				
				_videoTexture.UpdateExternalTexture((IntPtr)nativeTex);
			}
			/* else
            {
				if(_videoTexture != null)
				{
					Destroy(_videoTexture);
				}
				
                _videoTexture = null;
            }*/
			
			return _videoTexture;
		}
	}
	
	private void Call_Destroy()
	{
		
		VideoPlayerPluginDestroyInstance(m_iID);
		
		if(_videoTexture != null)
			Destroy(_videoTexture);
		
		_videoTexture = null;
		m_iID = -1;
	}
	
	private void Call_UnLoad()
	{
		
		VideoPlayerPluginStopVideo(m_iID);
		VideoPlayerPluginDestroyInstance(m_iID);
	}
	
	private bool Call_Load(string strFileName, int iSeek)
	{
		if( m_iID == -1)
		{
			Call_SetUnityActivity();
		}

		if(_videoTexture != null)
		{
			Destroy(_videoTexture);
		}
		
		_videoTexture = null;
		
		if (VideoPlayerPluginCanOutputToTexture(strFileName))
		{
			VideoPlayerPluginLoadVideo(m_iID,strFileName);	
		}
		
		
		
		
		return true;
	}
	
	private void Call_UpdateVideoTexture()
	{
		
		if(m_VideoTextureDummy != null)
		{
			Destroy(m_VideoTextureDummy);
			m_VideoTextureDummy = null;
		}
		
		if( m_CurrentState == MEDIAPLAYER_STATE.PLAYING || m_CurrentState == MEDIAPLAYER_STATE.PAUSED)
		{

			if( videoTexture == null)
				return;

			for( int i = 0; i < m_TargetMaterial.Length; i++)
			{
				if(m_TargetMaterial[i])
				{
					if(m_TargetMaterial[i].GetComponent<MeshRenderer>() != null)
						m_TargetMaterial[i].GetComponent<MeshRenderer>().material.mainTexture = _videoTexture;
				}
				
				if(m_TargetMaterial[i])
				{
					if(m_TargetMaterial[i].GetComponent<RawImage>() != null)
						m_TargetMaterial[i].GetComponent<RawImage>().texture= _videoTexture;
				}
			}

			
			m_VideoTexture = _videoTexture;
		}
		
		if (!m_bIsFirstFrameReady)
		{
			m_bIsFirstFrameReady = true;
			if (OnVideoFirstFrameReady != null)
			{
				OnVideoFirstFrameReady();
				OnVideoFirstFrameReady = null;
			}
		}
		
	
		
		
		
		
	}
	
	private void Call_SetVolume(float fVolume)
	{
		VideoPlayerPluginSetVolume(m_iID, fVolume);
	}
	
	private void Call_SetSeekPosition(int iSeek)
	{
		float fSeek = (float)iSeek / 1000.0f;
		VideoPlayerPluginSeekToVideo(m_iID,fSeek);
	}
	
	private int Call_GetSeekPosition()
	{
		
		float fSeek = VideoPlayerPluginCurTimeSeconds(m_iID);
		return (int)(fSeek * 1000.0f);
	}
	
	private void Call_Play(int iSeek)
	{
		float fSeek = (float)iSeek / 1000.0f;
		
		if( isPlaying == true )
		{
			VideoPlayerPluginSeekToVideo(m_iID,fSeek);
		}
		else
		{
			if( m_CurrentState != MEDIAPLAYER_STATE.READY)
				VideoPlayerPluginSeekToVideo(m_iID,fSeek);
			
			VideoPlayerPluginPlayVideo(m_iID);
		}
		
		
		if( m_CurrentState == MEDIAPLAYER_STATE.STOPPED)
			m_CurrentState = MEDIAPLAYER_STATE.PLAYING;
		
		
	}
	
	private void Call_Reset()
	{
		
	}
	
	private void Call_Stop()
	{
		VideoPlayerPluginStopVideo(m_iID);
	}
	
	private void Call_RePlay()
	{
		VideoPlayerPluginResumeVideo(m_iID);
		m_CurrentState = MEDIAPLAYER_STATE.PLAYING;
	}
	
	private void Call_Pause()
	{
		m_CurrentState = MEDIAPLAYER_STATE.PAUSED;
		VideoPlayerPluginPauseVideo(m_iID);
		
	}
	
	private int Call_GetVideoWidth()
	{
		return (int)videoSize.x;
	}
	
	private int Call_GetVideoHeight()
	{
		return (int)videoSize.y;
	}
	
	private void Call_SetUnityTexture(int iTextureID)
	{
		VideoPlayerPluginSetTexture(m_iID,iTextureID);
	}
	
	private void Call_SetWindowSize()
	{
		
	}
	
	private void Call_SetLooping(bool bLoop)
	{
		VideoPlayerPluginSetLoop(m_iID, bLoop);
	}
	
	
	public void Call_SetUnityActivity()
	{
		m_iID = VideoPlayerPluginCreateInstance();
	}
	
	private int Call_GetError()
	{
		return 0;
	}
	
	private int Call_GetErrorExtra()
	{
		return 0;
	}
	
	private int Call_GetDuration()
	{
		return (int)(duration * 1000);
	}
	
	private int Call_GetCurrentSeekPercent()
	{
		return -1;
	}
	
	private MEDIAPLAYER_STATE Call_GetStatus()
	{
		
		if( VideoPlayerPluginFinish(m_iID) )
		{
			return MEDIAPLAYER_STATE.END;
		}

		if( VideoPlayerPluginError(m_iID) )
		{
			return MEDIAPLAYER_STATE.ERROR;
		}
		
		if( m_CurrentState == MEDIAPLAYER_STATE.PAUSED || m_CurrentState == MEDIAPLAYER_STATE.STOPPED)
		{
			return m_CurrentState;
		}
		else if(isPlaying)
		{
			return MEDIAPLAYER_STATE.PLAYING;
		}
		else if(ready)
		{
			return MEDIAPLAYER_STATE.READY;
		}
		
		
		return m_CurrentState;
	}
	#endif
	#else
	
	
	
	private void Call_Destroy()
	{
		
	}
	
	private void Call_UnLoad()
	{
		
	}
	
	private bool Call_Load(string strFileName, int iSeek)
	{
		
		
		return true;
	}
	
	private void Call_UpdateVideoTexture()
	{
		
	}
	
	private void Call_SetVolume(float fVolume)
	{
		
	}
	
	private void Call_SetSeekPosition(int iSeek)
	{
		
	}
	
	private int Call_GetSeekPosition()
	{
		return 0;
	}
	
	private void Call_Play(int iSeek)
	{
		
	}
	
	private void Call_Reset()
	{
		
	}
	
	private void Call_Stop()
	{
		
	}
	
	private void Call_RePlay()
	{
		
	}
	
	private void Call_Pause()
	{
		
	}
	
	private int Call_GetVideoWidth()
	{
		return 0;
	}
	
	private int Call_GetVideoHeight()
	{
		return 0;
	}
	
	private void Call_SetUnityTexture(int iTextureID)
	{
		
	}
	
	private void Call_SetWindowSize()
	{
		
	}
	
	private void Call_SetLooping(bool bLoop)
	{
		
	}
	
	private void Call_SetRockchip(bool bValue)
	{
		
	}
	
	public void Call_SetUnityActivity()
	{
		
	}
	
	private int Call_GetError()
	{
		return 0;
	}
	
	private int Call_GetErrorExtra()
	{
		return 0;
	}
	
	private int Call_GetDuration()
	{
		return -1;
	}
	
	private int Call_GetCurrentSeekPercent()
	{
		return -1;
	}
	
	private void Call_SetSplitOBB(bool bValue, string strOBBName)
	{
	}
	
	private MEDIAPLAYER_STATE Call_GetStatus()
	{
		return (MEDIAPLAYER_STATE)0;
	}
	
#endif // !UNITY_EDITOR



    public IEnumerator DownloadStreamingVideoAndLoad(string strURL)
    {
        strURL = strURL.Trim();

        Debug.Log("DownloadStreamingVideo : " + strURL);


        WWW www = new WWW(strURL);

        yield return www;

        if (string.IsNullOrEmpty(www.error))
        {

            if (System.IO.Directory.Exists(Application.persistentDataPath + "/Data") == false)
                System.IO.Directory.CreateDirectory(Application.persistentDataPath + "/Data");

            string write_path = Application.persistentDataPath + "/Data" + strURL.Substring(strURL.LastIndexOf("/"));

            /*	if(System.IO.File.Exists(write_path) == true)
			{
				Debug.Log("Delete : " + write_path);
				System.IO.File.Delete(write_path);
			}
		*/
            System.IO.File.WriteAllBytes(write_path, www.bytes);

            Load("file://" + write_path);
        }
        else
        {
            Debug.Log(www.error);

        }

        www.Dispose();
        www = null;
        Resources.UnloadUnusedAssets();
    }

    IEnumerator CopyStreamingAssetVideoAndLoad(string strURL)
    {
        strURL = strURL.Trim();

        string write_path = Application.persistentDataPath + "/" + strURL;

        if (System.IO.File.Exists(write_path) == false)
        {
            Debug.Log("CopyStreamingAssetVideoAndLoad : " + strURL);

            WWW www = new WWW(Application.streamingAssetsPath + "/" + strURL);

            yield return www;

            if (string.IsNullOrEmpty(www.error))
            {



                Debug.Log(write_path);
                System.IO.File.WriteAllBytes(write_path, www.bytes);

                Load("file://" + write_path);


            }
            else
            {
                Debug.Log(www.error);

            }

            www.Dispose();
            www = null;
        }
        else
        {
            Load("file://" + write_path);
        }

    }

}
