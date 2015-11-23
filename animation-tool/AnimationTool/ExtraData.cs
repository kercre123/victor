using System.Windows.Forms.DataVisualization.Charting;
using System.Windows.Forms;
using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.IO;
using System.Drawing;
using System.Drawing.Imaging;
using System.Reflection;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;

namespace AnimationTool.Sequencer
{
    public class ExtraData
    {
        public virtual string FileName { get; set; }
        public string FileNameWithPath { get; protected set; }
        public double Length { get; set; }
        public virtual string Image { get { return string.Empty; } }
        public static Dictionary<string, ExtraData> Entries { get; set; }
        public static int Count = 0;
        public const string Key = "sequencerDataKey";
        public bool Exists { get; protected set; }

        public ExtraData()
        {
            Exists = true;
            Length = MoveSelectedDataPoints.DELTA_X;
            FileName = string.Empty;
            FileNameWithPath = string.Empty;
        }

        public static void Reset()
        {
            Entries.Clear();
            Count = 0;
        }

        public static double GetDistanceFromNextRight(DataPoint dataPoint, DataPointCollection points)
        {
            double x = MoveSelectedDataPoints.DELTA_X;

            for (int i = 0; i < points.Count - 1; ++i)
            {
                if (points[i] == dataPoint)
                {
                    x = Math.Round(points[i + 1].YValues[0] - dataPoint.YValues[0], 1);
                    break;
                }
            }

            return x;
        }
    }

    public class ExtraAudioData : ExtraData
    {
        public List<string> Sounds { get; protected set; }
        public static string FullPath { get { return MainForm.rootDirectory + "\\sounds"; } }
        public override string FileName { get { return Path.GetFileName(FileNameWithPath) + " (Volume: " + Volume + ")"; } }
        public double Volume { get; set; }

        public ExtraAudioData(string path, double jsonDataLength = -1, double jsonDataVolume = 1)
        {
            Length = GetSoundLength(path, jsonDataLength);
            FileNameWithPath = path;
            Exists = Directory.Exists(path);
            Volume = jsonDataVolume;
        }

        public static ExtraAudioData OpenWavFile(FolderBrowserDialog selectFolder)
        {
            if (selectFolder == null) return null;

            selectFolder.SelectedPath = FullPath;

            ExtraAudioData extraData = null;

            if (selectFolder.ShowDialog() == DialogResult.OK)
            {
                extraData = new ExtraAudioData(selectFolder.SelectedPath);
            }

            return extraData;
        }

        public double GetSoundLength(string path, double jsonData)
        {
            double length = jsonData;
            Sounds = new List<string>();

            if (!Directory.Exists(path))
            {
                return length;
            }

            string[] files = Directory.GetFiles(path);

            for (int i = 0; i < files.Length; ++i)
            {
                string file = files[i];

                if (IsWav(file))
                {
                    Sounds.Add(file);

                    double l = GetSoundLength(file);

                    if (l > length)
                    {
                        length = l;
                    }
                }
            }

            return length;
        }

        private static bool IsWav(string file)
        {
            return file.EndsWith(".wav");
        }

        [DllImport("winmm.dll")]
        private static extern uint mciSendString(string command, StringBuilder returnValue, int returnLength, IntPtr winHandle);

        private static double GetSoundLength(string fileName)
        {
            StringBuilder lengthBuf = new StringBuilder(32);

            mciSendString(string.Format("open \"{0}\" type waveaudio alias wave", fileName), null, 0, IntPtr.Zero);
            mciSendString("status wave length", lengthBuf, lengthBuf.Capacity, IntPtr.Zero);
            mciSendString("close wave", null, 0, IntPtr.Zero);

            double length = 0;
            double.TryParse(lengthBuf.ToString(), out length);

            length /= 1000;

            length = Math.Round(length, 1);

            if (length < MoveSelectedDataPoints.DELTA_X)
            {
                length = MoveSelectedDataPoints.DELTA_X;
            }

            return length;
        }
    }

    public class ExtraFaceAnimationData : ExtraData
    {
        public Dictionary<double, string> Images { get; protected set; }
        public static string FullPath { get { return MainForm.rootDirectory + "\\faceAnimations"; } }

        public ExtraFaceAnimationData(string path, double jsonData = -1)
        {
            Length = GetFaceLength(path, jsonData);
            FileName = Path.GetFileName(path);
            FileNameWithPath = path;
            Exists = Directory.Exists(path);
        }

        public static ExtraFaceAnimationData SelectFaceFolder(FolderBrowserDialog selectFolder)
        {
            selectFolder.SelectedPath = FullPath;

            ExtraFaceAnimationData extraData = null;

            if (selectFolder.ShowDialog() == DialogResult.OK)
            {
                extraData = new ExtraFaceAnimationData(selectFolder.SelectedPath);

                if (extraData.Length < MoveSelectedDataPoints.DELTA_X)
                {
                    extraData = null;
                    MessageBox.Show("Directory has no valid image files.");
                }
            }

            return extraData;
        }

        private static bool IsImage(string file)
        {
            return file.EndsWith(".jpeg") || file.EndsWith(".png") || file.EndsWith(".jpg");
        }

        protected double GetFaceLength(string path, double jsonData)
        {
            double length = jsonData;
            int count = 0;
            Images = new Dictionary<double, string>();

            if (!Directory.Exists(path))
            {
                return length;
            }

            string[] files = Directory.GetFiles(path);

            for (int i = 0; i < files.Length; ++i)
            {
                if (IsImage(files[i]))
                {
                    string name = Path.GetFileNameWithoutExtension(files[i]);
                    int index = name.IndexOf('_');

                    string number = name.Length > index && index > -1 ? name.Substring(index + 1) : name;

                    int value = -1;

                    try
                    {
                        value = Convert.ToInt32(number);
                    }
                    catch (Exception e)
                    {
                        MessageBox.Show(e.Message);
                        continue;
                    }

                    double key = Math.Round(value * 0.033, 1);

                    if (!Images.ContainsKey(key))
                    {
                        Images.Add(key, files[i]);
                    }

                    if (value > count)
                    {
                        count = value;
                    }
                }
            }

            if (Images.Count > 0 && !Images.ContainsKey(0))
            {
                MessageBox.Show(path + " does not have a 0 frame.");
            }
            else if (count > 0) // if at least one image is in folder
            {
                length = count * 0.033;

                length = Math.Round(length, 1);

                if (length < MoveSelectedDataPoints.DELTA_X)
                {
                    length = MoveSelectedDataPoints.DELTA_X;
                }
            }

            return length;
        }
    }

    public class ExtraProceduralFaceData : ExtraData
    {
        public float faceAngle;
        public int faceCenterX;
        public int faceCenterY;
        public float faceScaleX;
        public float faceScaleY;

        public float[] leftEye;
        public float[] rightEye;

        public ExtraProceduralFaceData() : this (new ProceduralFacePointData()) { }

        private const float kEyeCenterXLeft = 32f;
        private const float kEyeCenterXRight = 96f;
        private const float kEyeCenterY = 32f;

        public ExtraProceduralFaceData(ProceduralFacePointData data)
        {
            Length = MoveSelectedDataPoints.DELTA_X;
            Exists = true;

            leftEye = new float[(int)ProceduralEyeParameter.NumParameters];
            rightEye = new float[(int)ProceduralEyeParameter.NumParameters];

            faceAngle = data.faceAngle;
            faceCenterX = data.faceCenterX;
            faceCenterY = data.faceCenterY;
            faceScaleX = data.faceScaleX;
            faceScaleY = data.faceScaleY;

            for (int i = 0; data.leftEye != null && i < data.leftEye.Length && i < leftEye.Length; ++i)
            {
                float leftMod = 0f;
                float rightMod = 0f;
                if ((int)ProceduralEyeParameter.EyeCenterX == i)
                {
                    leftMod = kEyeCenterXLeft;
                    rightMod = kEyeCenterXRight;
                }
                else if ((int)ProceduralEyeParameter.EyeCenterY == i)
                {
                    leftMod = rightMod = kEyeCenterY;
                }

                leftEye[i] = data.leftEye[i] + leftMod;
                rightEye[i] = data.rightEye[i] + rightMod;
            }
        }

        public ProceduralFacePointData GetProceduralFacePointData()
        {
            ProceduralFacePointData pointData = new ProceduralFacePointData();
            pointData.faceAngle = faceAngle;
            pointData.faceCenterX = faceCenterX;
            pointData.faceCenterY = faceCenterY;
            pointData.faceScaleX = faceScaleX;
            pointData.faceScaleY = faceScaleY;

            for (int i = 0; i < leftEye.Length && i < pointData.leftEye.Length; ++i)
            {
                pointData.leftEye[i] = (float)Math.Round(leftEye[i], 2);
                pointData.rightEye[i] = (float)Math.Round(rightEye[i], 2);
            }
            AdjustEyeParamsOut(pointData.leftEye, pointData.rightEye);

            return pointData;
        }

        public DisplayProceduralFace GetProceduralFaceMessage()
        {
            DisplayProceduralFace displayProceduralFaceMessage = new DisplayProceduralFace();
            displayProceduralFaceMessage.leftEye = new float[(int)ProceduralEyeParameter.NumParameters];
            displayProceduralFaceMessage.rightEye = new float[(int)ProceduralEyeParameter.NumParameters];
            displayProceduralFaceMessage.robotID = 1;
            displayProceduralFaceMessage.faceAngle = faceAngle;
            displayProceduralFaceMessage.faceCenX = faceCenterX;
            displayProceduralFaceMessage.faceCenY = faceCenterY;
            displayProceduralFaceMessage.faceScaleX = faceScaleX;
            displayProceduralFaceMessage.faceScaleY = faceScaleY;

            for (int i = 0; i < displayProceduralFaceMessage.leftEye.Length && i < leftEye.Length; ++i)
            {
                displayProceduralFaceMessage.leftEye[i] = leftEye[i];
                displayProceduralFaceMessage.rightEye[i] = rightEye[i];
            }
            AdjustEyeParamsOut(displayProceduralFaceMessage.leftEye, displayProceduralFaceMessage.rightEye);

            return displayProceduralFaceMessage;
        }

        public static void AdjustEyeParamsOut(float[] leftParamArray, float[] rightParamArray)
        {
            int curIndex = (int)ProceduralEyeParameter.EyeCenterX;
            leftParamArray[curIndex] = Math.Min(128f, Math.Max(-128f, leftParamArray[curIndex] - kEyeCenterXLeft));
            rightParamArray[curIndex] = Math.Min(128f, Math.Max(-128f, rightParamArray[curIndex] - kEyeCenterXRight));

            curIndex = (int)ProceduralEyeParameter.EyeCenterY;
            leftParamArray[curIndex] = Math.Min(64f, Math.Max(-64f, leftParamArray[curIndex] - kEyeCenterY));
            rightParamArray[curIndex] = Math.Min(64f, Math.Max(-64f, rightParamArray[curIndex] - kEyeCenterY));
        }
    }

    public class ExtraBodyMotionData : ExtraData
    {
        protected static Assembly assembly = Assembly.GetExecutingAssembly();
        protected Stream imageStream;
        protected Image image;

        private static string temp_path = null;
        public static string TEMP_PATH
        {
            get
            {
                if (temp_path == null)
                {
                    try
                    {
                        temp_path = Path.GetTempPath() + "\\AnimationTool";
                    }
                    catch (Exception)
                    {
                        temp_path = Application.StartupPath + "\\Resources";
                    }
                }

                return temp_path;
            }
        }

        protected string ARROW_FORWARD
        {
            get
            {
                string path = TEMP_PATH + "\\arrow-forward.png";

                if (!File.Exists(path))
                {
                    imageStream = assembly.GetManifestResourceStream("AnimationTool.Resources.arrow-forward.png");
                    image = System.Drawing.Image.FromStream(imageStream);
                    image.Save(path, ImageFormat.Png);
                }

                return path;
            }
        }

        protected string ARROW_BACK
        {
            get
            {
                string path = TEMP_PATH + "\\arrow-back.png";

                if (!File.Exists(path))
                {
                    imageStream = assembly.GetManifestResourceStream("AnimationTool.Resources.arrow-back.png");
                    image = System.Drawing.Image.FromStream(imageStream);
                    image.Save(path, ImageFormat.Png);
                }

                return path;
            }
        }

        protected string ARROW_LEFT
        {
            get
            {
                string path = TEMP_PATH + "\\arrow-left.png";

                if (!File.Exists(path))
                {
                    imageStream = assembly.GetManifestResourceStream("AnimationTool.Resources.arrow-left.png");
                    image = System.Drawing.Image.FromStream(imageStream);
                    image.Save(path, ImageFormat.Png);
                }

                return path;
            }
        }

        protected string ARROW_RIGHT
        {
            get
            {
                string path = TEMP_PATH + "\\arrow-right.png";

                if (!File.Exists(path))
                {
                    imageStream = assembly.GetManifestResourceStream("AnimationTool.Resources.arrow-right.png");
                    image = System.Drawing.Image.FromStream(imageStream);
                    image.Save(path, ImageFormat.Png);
                }

                return path;
            }
        }

        protected string ARROW_ARC_FORWARD_LEFT
        {
            get
            {
                string path = TEMP_PATH + "\\arrow-arc-forward-left.png";

                if (!File.Exists(path))
                {
                    imageStream = assembly.GetManifestResourceStream("AnimationTool.Resources.arrow-arc-forward-left.png");
                    image = System.Drawing.Image.FromStream(imageStream);
                    image.Save(path, ImageFormat.Png);
                }

                return path;
            }
        }

        protected string ARROW_ARC_FORWARD_RIGHT
        {
            get
            {
                string path = TEMP_PATH + "\\arrow-arc-forward-right.png";

                if (!File.Exists(path))
                {
                    imageStream = assembly.GetManifestResourceStream("AnimationTool.Resources.arrow-arc-forward-right.png");
                    image = System.Drawing.Image.FromStream(imageStream);
                    image.Save(path, ImageFormat.Png);
                }

                return path;
            }
        }

        protected string ARROW_ARC_BACK_LEFT
        {
            get
            {
                string path = TEMP_PATH + "\\arrow-arc-back-left.png";

                if (!File.Exists(path))
                {
                    imageStream = assembly.GetManifestResourceStream("AnimationTool.Resources.arrow-arc-back-left.png");
                    image = System.Drawing.Image.FromStream(imageStream);
                    image.Save(path, ImageFormat.Png);
                }

                return path;
            }
        }

        protected string ARROW_ARC_BACK_RIGHT
        {
            get
            {
                string path = TEMP_PATH + "\\arrow-arc-back-right.png";

                if (!File.Exists(path))
                {
                    imageStream = assembly.GetManifestResourceStream("AnimationTool.Resources.arrow-arc-back-right.png");
                    image = System.Drawing.Image.FromStream(imageStream);
                    image.Save(path, ImageFormat.Png);
                }

                return path;
            }
        }
    }

    public class ExtraStraightData : ExtraBodyMotionData
    {
        public int Speed_mms { get; set; }
        public override string FileName { get { return "Straight (" + Speed_mms + " mm/s)"; } }
        public override string Image { get { return Speed_mms > 0 ? ARROW_FORWARD : ARROW_BACK; } }

        public ExtraStraightData()
        {
            Speed_mms = 10;
            Length = MoveSelectedDataPoints.DELTA_X;
        }
    }

    public class ExtraArcData : ExtraBodyMotionData
    {
        public int Speed_mms { get; set; }
        public int Radius_mm { get; set; }
        public override string FileName { get { return "Arc (" + Speed_mms + " mm/s) (" + Radius_mm + " mm)"; } }
        public override string Image
        {
            get
            {
                return Radius_mm > 0 && Speed_mms > 0 ? ARROW_ARC_FORWARD_LEFT : Radius_mm > 0 && Speed_mms < 0 ? ARROW_ARC_BACK_LEFT :
                    Radius_mm < 0 && Speed_mms > 0 ? ARROW_ARC_FORWARD_RIGHT : ARROW_ARC_BACK_RIGHT;
            }
        }

        public ExtraArcData()
        {
            Speed_mms = 10;
            Radius_mm = 1;
            Length = MoveSelectedDataPoints.DELTA_X;
        }
    }

    public class ExtraTurnInPlaceData : ExtraBodyMotionData
    {
        public int Angle_deg { get; set; }
        public override string FileName { get { return "Turn In Place (" + Angle_deg + " deg)"; } }
        public override string Image { get { return Angle_deg > 0 ? ARROW_LEFT : ARROW_RIGHT; } }

        public ExtraTurnInPlaceData()
        {
            Angle_deg = 1;
            Length = MoveSelectedDataPoints.DELTA_X;
        }
    }
}
