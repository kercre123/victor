namespace WellFired
{
	public class PresetInfo
	{
		public string 				Name 			{ get; set; }
		public string 				CapturePath		{ get; set; }
		public USRecord.Upscaling 	UpscaleAmount 	{ get; set; }
		public USRecord.FrameRate 	FrameRate 		{ get; set; }
		
		public PresetInfo(string name, string capturePath, USRecord.Upscaling upscaleAmount, USRecord.FrameRate framerate)
		{
			Name = name;
			CapturePath = capturePath;
			UpscaleAmount = upscaleAmount;
			FrameRate = framerate;
		}
	}
}