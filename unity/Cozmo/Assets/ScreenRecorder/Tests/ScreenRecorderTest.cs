using UnityEngine;
using System.Collections;

public class ScreenRecorderTest : MonoBehaviour 
{

	void Start () 
	{
		TestVideoResolutionDetails();
	}
	
	void TestVideoResolutionDetails()
	{
		const int testSize = 4;
		int [] ws = new int[testSize];
		int [] hs = new int[testSize];
		//iphone
		ws[0] = 480; hs[0] = 320;
		ws[1] = 960; hs[1] = 640;
		//ipad
		ws[2] = 1024; hs[2] = 768;
		ws[3] = 2048; hs[3] = 1536;

		foreach (ScreenRecorder.VideoResolution vr in System.Enum.GetValues(typeof(ScreenRecorder.VideoResolution)))
		{
			for(int i=0; i<testSize; ++i)
			{
				Resolution res = ScreenRecorder.videoResolutionForQualityAndScreenSize(vr, ws[i], hs[i]);
				TestVideoResolutionDetailsCheck(res, vr, ws[i], hs[i]);
				//printTestCase(res, vr, ws[i], hs[i]);

				res = ScreenRecorder.videoResolutionForQualityAndScreenSize(vr, hs[i], ws[i]);
				TestVideoResolutionDetailsCheck(res, vr, hs[i], ws[i]);
				//printTestCase(res, vr, hs[i], ws[i]);
			}
		}
	}
	
	bool TestVideoResolutionDetailsCheck(Resolution res, ScreenRecorder.VideoResolution vr, int w, int h)
	{
		if(     res.width/8*8 != res.width
			|| res.height/8*8 != res.height)
		{
			Debug.LogError("Size is not aligned by 8");
			printTestCase(res, vr, w, h);
			return false;
		}
		
		if( Mathf.Abs((float)res.width/(float)res.height - (float)w/(float)h) > 0.03)
		{
			Debug.LogError("Aspect ratio is wrong");
			printTestCase(res, vr, w, h);
			return false;			
		}
		
		if(    res.width - w > 7
			|| res.height - h > 7)
		{
			Debug.LogError("Video resilution is bigger than screen resolution");
			printTestCase(res, vr, w, h);
			return false;			
		}
			
		return true;
	}
	
	void printTestCase(Resolution res, ScreenRecorder.VideoResolution vr, int w, int h)
	{
		Debug.Log("TestCase:"+ vr +" w:" + w + " h:" + h);
		Debug.Log("Result:"+ " w:"+res.width+" h:"+res.height);
		Debug.Log("||==========================================||");
	}
}
