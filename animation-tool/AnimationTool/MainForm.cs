using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using System.IO;

namespace AnimationTool
{
    public partial class MainForm : Form
    {
        //This list contains all channels so we can access them directly
        List<Component> channelList;

        ChangeDurationForm changeDurationForm;
        BodyForm bodyForm;
        VolumeForm volumeForm;
        StraightForm straightForm;
        ArcForm arcForm;
        TurnInPlaceForm turnInPlaceForm;
        FaceForm faceForm;
        IPForm ipForm;

        OpenFileDialog openFile;
        FolderBrowserDialog selectFolder;
        SaveFileDialog saveFileAs;

        //reference to the currently selected Chart
        Chart curChart;
        ChartArea curChartArea { get { return curChart != null && curChart.ChartAreas.Count > 0 ? curChart.ChartAreas[0] : null; } }
        DataPointCollection curPoints { get { return curChart != null && curChart.Series.Count > 0 ? curChart.Series[0].Points : null; } }

        string jsonFilePath { get { return rootDirectory + "\\animations"; } }

        private static MainForm instance;

        public static string rootDirectory
        {
            get
            {
                if (instance != null && !IsValidRootDirectory(Properties.Settings.Default.rootDirectory))
                {
                    if (MessageBox.Show("Please set the animation assets root directory", "", MessageBoxButtons.OK) != DialogResult.Yes)
                    {
                        instance.SetRootDirectory(null, null);
                    }
                }

                return Properties.Settings.Default.rootDirectory;
            }
        }

        string currentFile
        {
            get
            {
                saveToolStripMenuItem.Enabled = Properties.Settings.Default.currentFile != null;
                Text = Path.GetFileName(Properties.Settings.Default.currentFile);

                return Properties.Settings.Default.currentFile;
            }
            set
            {
                Properties.Settings.Default["currentFile"] = value;
                Properties.Settings.Default.Save();
                saveToolStripMenuItem.Enabled = value != null;
                Text = Path.GetFileName(value);
            }
        }

        bool movingDataPointsWithMouse;

        public MainForm()
        {
            instance = this;
            Sequencer.ExtraData.Entries = new Dictionary<string, Sequencer.ExtraData>();
            ActionManager actionManager = new ActionManager();

            changeDurationForm = new ChangeDurationForm();
            ipForm = new IPForm();
            faceForm = new FaceForm();
            bodyForm = new BodyForm();
            volumeForm = new VolumeForm();
            straightForm = new StraightForm();
            arcForm = new ArcForm();
            turnInPlaceForm = new TurnInPlaceForm();
            openFile = new OpenFileDialog();
            selectFolder = new FolderBrowserDialog();
            saveFileAs = new SaveFileDialog();
            channelList = new List<Component>();

            RobotEngineMessenger.instance.ConnectionManager.ConnectionTextUpdate += ChangeConnectionText;
            RobotEngineMessenger.instance.ConnectionManager.Start();
            FormClosing += (unused1, unused2) => RobotEngineMessenger.instance.ConnectionManager.Stop();
            Application.ApplicationExit += (unused1, unused2) => RobotEngineMessenger.instance.ConnectionManager.Stop();
            Application.Idle += UpdateConnectionText;
            Resize += MainForm_Resize;

            Sequencer.AddDataPoint.ChangeDuration += ChangeDuration;

            movingDataPointsWithMouse = false;

            if (!Directory.Exists(Sequencer.ExtraBodyMotionData.TEMP_PATH))
            {
                Directory.CreateDirectory(Sequencer.ExtraBodyMotionData.TEMP_PATH);
            }

            DirectoryInfo directory = new DirectoryInfo(Sequencer.ExtraBodyMotionData.TEMP_PATH);

            foreach (FileInfo file in directory.GetFiles())
            {
                if (!file.IsReadOnly)
                {
                    try
                    {
                        file.Delete();
                    }
                    catch (Exception) { }
                }
            }

            InitializeComponent();

            headAngle.panel.Controls.Add(cHeadAngle);
            headAngle.checkBox.CheckedChanged += HeadAngleCheckBox;
            liftHeight.panel.Controls.Add(cLiftHeight);
            liftHeight.checkBox.CheckedChanged += LiftHeightCheckBox;
            bodyMotion.panel.Controls.Add(cBodyMotion);
            bodyMotion.checkBox.CheckedChanged += BodyMotionCheckBox;
            proceduralFace.panel.Controls.Add(cProceduralFace);
            proceduralFace.checkBox.CheckedChanged += ProceduralFaceCheckBox;
            faceAnimation.panel.Controls.Add(cFaceAnimation);
            faceAnimation.checkBox.CheckedChanged += FaceAnimationCheckBox;
            audioRobot.panel.Controls.Add(cAudioRobot);
            audioRobot.checkBox.CheckedChanged += AudioRobotCheckBox;
            audioDevice.panel.Controls.Add(cAudioDevice);
            audioDevice.checkBox.CheckedChanged += AudioDeviceCheckBox;
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            if (Sequencer.ExtraData.Entries == null || channelList == null) return;

            ActionManager.Reset();
            Sequencer.ExtraData.Reset();
            faceForm.Reset();
            channelList.Clear();

            double interval = ChangeChartDuration.Clamp(Math.Round(Properties.Settings.Default.maxTime * 0.1, 1), 0.1, 0.5);

            //Head Chart
            cHeadAngle.Series[0].Points.Clear();
            cHeadAngle.ChartAreas[0].AxisX.Minimum = 0;
            cHeadAngle.ChartAreas[0].AxisX.Maximum = Properties.Settings.Default.maxTime;
            cHeadAngle.ChartAreas[0].AxisX.LabelStyle.Interval = interval;
            cHeadAngle.ChartAreas[0].AxisY.Minimum = -25;
            cHeadAngle.ChartAreas[0].AxisY.Maximum = 34;
            ActionManager.Do(new DisableChart(cHeadAngle, headAngle.checkBox), true);
            ActionManager.Do(new EnableChart(cHeadAngle, headAngle.checkBox), true);
            channelList.Add(cHeadAngle);

            //Lift Chart
            cLiftHeight.Series[0].Points.Clear();
            cLiftHeight.ChartAreas[0].AxisX.Minimum = 0;
            cLiftHeight.ChartAreas[0].AxisX.Maximum = Properties.Settings.Default.maxTime;
            cLiftHeight.ChartAreas[0].AxisX.LabelStyle.Interval = interval;
            cLiftHeight.ChartAreas[0].AxisY.Minimum = 31;
            cLiftHeight.ChartAreas[0].AxisY.Maximum = 92;
            ActionManager.Do(new DisableChart(cLiftHeight, liftHeight.checkBox), true);
            ActionManager.Do(new EnableChart(cLiftHeight, liftHeight.checkBox), true);
            channelList.Add(cLiftHeight);

            //Body Chart
            cBodyMotion.Series[0].Points.Clear();
            cBodyMotion.ChartAreas[0].AxisY.Minimum = 0;
            cBodyMotion.ChartAreas[0].AxisY.Maximum = Properties.Settings.Default.maxTime;
            cBodyMotion.ChartAreas[0].AxisY.LabelStyle.Interval = interval;
            ActionManager.Do(new DisableChart(cBodyMotion, bodyMotion.checkBox), true);
            ActionManager.Do(new Sequencer.EnableChart(cBodyMotion, bodyMotion.checkBox), true);
            channelList.Add(cBodyMotion);

            //Face Animation Data Chart
            cProceduralFace.Series[0].Points.Clear();
            cProceduralFace.ChartAreas[0].AxisY.Minimum = 0;
            cProceduralFace.ChartAreas[0].AxisY.Maximum = Properties.Settings.Default.maxTime;
            cProceduralFace.ChartAreas[0].AxisY.LabelStyle.Interval = interval;
            ActionManager.Do(new DisableChart(cProceduralFace, proceduralFace.checkBox), true);
            ActionManager.Do(new Sequencer.EnableChart(cProceduralFace, proceduralFace.checkBox), true);
            channelList.Add(cProceduralFace);

            //Face Animation Image Chart
            cFaceAnimation.Series[0].Points.Clear();
            cFaceAnimation.ChartAreas[0].AxisY.Minimum = 0;
            cFaceAnimation.ChartAreas[0].AxisY.Maximum = Properties.Settings.Default.maxTime;
            cFaceAnimation.ChartAreas[0].AxisY.LabelStyle.Interval = interval;
            ActionManager.Do(new DisableChart(cFaceAnimation, faceAnimation.checkBox), true);
            ActionManager.Do(new FaceAnimation.EnableChart(cFaceAnimation, faceAnimation.checkBox), true);
            channelList.Add(cFaceAnimation);

            //Audio Robot Chart
            cAudioRobot.Series[0].Points.Clear();
            cAudioRobot.ChartAreas[0].AxisY.Minimum = 0;
            cAudioRobot.ChartAreas[0].AxisY.Maximum = Properties.Settings.Default.maxTime;
            cAudioRobot.ChartAreas[0].AxisY.LabelStyle.Interval = interval;
            ActionManager.Do(new DisableChart(cAudioRobot, audioRobot.checkBox), true);
            ActionManager.Do(new Sequencer.EnableChart(cAudioRobot, audioRobot.checkBox), true);
            channelList.Add(cAudioRobot);

            //Audio Robot Device
            cAudioDevice.Series[0].Points.Clear();
            cAudioDevice.ChartAreas[0].AxisY.Minimum = 0;
            cAudioDevice.ChartAreas[0].AxisY.Maximum = Properties.Settings.Default.maxTime;
            cAudioDevice.ChartAreas[0].AxisY.LabelStyle.Interval = interval;
            ActionManager.Do(new DisableChart(cAudioDevice, audioDevice.checkBox), true);
            ActionManager.Do(new Sequencer.EnableChart(cAudioDevice, audioDevice.checkBox), true);
            channelList.Add(cAudioDevice);

            if (!File.Exists(currentFile))
            {
                currentFile = null;
            }
            else if (!string.IsNullOrEmpty(File.ReadAllText(currentFile)))
            {
                ReadChartsFromFile(File.ReadAllText(currentFile));
            }
        }

        protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
        {
            if (keyData == Keys.Back)
            {
                Delete(null, null);

                return true;
            }

            if (keyData == Keys.Space)
            {
                PlayAnimation(null, null);

                return true;
            }

            KeyDownHandler(keyData);

            return base.ProcessCmdKey(ref msg, keyData);
        }

        private void KeyDownHandler(Keys keyData)
        {
            if (movingDataPointsWithMouse) return;

            bool left = ModifierKeys.HasFlag(Keys.Left) || keyData == Keys.Left;
            bool right = ModifierKeys.HasFlag(Keys.Right) || keyData == Keys.Right;
            bool up = ModifierKeys.HasFlag(Keys.Up) || keyData == Keys.Up;
            bool down = ModifierKeys.HasFlag(Keys.Down) || keyData == Keys.Down;

            if (left || right || up || down)
            {
                if (curDataPoint != null && curPreviewBar != null && curPreviewBar.MarkerColor == SelectDataPoint.MarkerColor)
                {
                    if (ActionManager.Do(new FaceAnimation.MoveSelectedPreviewBar(curPreviewBar, curDataPoint, left, right, faceAnimation.pictureBox)))
                    {
                        cFaceAnimation.Refresh();
                    }
                }
                else if (ActionManager.Do(new MoveSelectedDataPointsOfSelectedCharts(channelList, left, right, up, down)))
                {
                    foreach (Chart chart in channelList)
                    {
                        chart.Refresh();
                    }
                }
            }
        }

        private void Undo(object o, EventArgs e)
        {
            if (channelList == null) return;

            ActionManager.Undo();

            foreach (Chart chart in channelList)
            {
                chart.Refresh();
            }
        }

        private void Redo(object o, EventArgs e)
        {
            if (channelList == null) return;

            ActionManager.Redo();

            foreach (Chart chart in channelList)
            {
                chart.Refresh();
            }
        }

        private void Delete(object o, EventArgs e)
        {
            if (channelList == null) return;

            ActionManager.Do(new RemoveSelectedDataPointsOfSelectedCharts(channelList));

            foreach (Chart chart in channelList)
            {
                chart.Refresh();
            }
        }

        private void ChangeDuration(object o, EventArgs e)
        {
            if (changeDurationForm == null) return;

            double maxTime = Properties.Settings.Default.maxTime;
            double minTime = MoveSelectedDataPoints.DELTA_X;

            if (o != null)
            {
                try
                {
                    maxTime = Convert.ToDouble(o);
                    minTime = maxTime; // don't allow below this number
                }
                catch (Exception) { }
            }

            DialogResult result = changeDurationForm.Open(Location, maxTime, minTime);

            if (changeDurationForm.Duration != Properties.Settings.Default.maxTime)
            {
                if (result == DialogResult.OK) // truncate
                {
                    ActionManager.Do(new TruncateAllChartDurations(channelList, changeDurationForm.Duration));
                }
                else if (result == DialogResult.Yes) // scale
                {
                    ActionManager.Do(new ScaleAllChartDurations(channelList, changeDurationForm.Duration));
                }
            }
        }

        private void SetRootDirectory(object o, EventArgs e)
        {
            if (selectFolder == null) return;

            selectFolder.SelectedPath = Properties.Settings.Default.rootDirectory;

            if (selectFolder.ShowDialog() == DialogResult.OK)
            {
                if (!IsValidRootDirectory(selectFolder.SelectedPath))
                {
                    SetRootDirectory(o, e);
                }

                Properties.Settings.Default["rootDirectory"] = selectFolder.SelectedPath;
                Properties.Settings.Default.Save();
            }
        }

        private static bool IsValidRootDirectory(string path)
        {
            string json = path + "\\animations";
            string face = path + "\\faceAnimations";
            string sounds = path + "\\sounds";

            return !string.IsNullOrEmpty(path) && Directory.Exists(path) && Directory.Exists(json) && Directory.Exists(face) && Directory.Exists(sounds);
        }

        private void OpenFile(object o, EventArgs e)
        {
            openFile.Filter = "json files|*.json";
            openFile.InitialDirectory = jsonFilePath;

            if (openFile.ShowDialog() == DialogResult.OK)
            {
                if (File.Exists(openFile.FileName))
                {
                    currentFile = openFile.FileName;
                    MainForm_Load(null, null);
                }
            }
        }

        private void NewFile(object o, EventArgs e)
        {
            currentFile = null;
            MainForm_Load(o, e);
        }

        private void SaveFile(object o, EventArgs e)
        {
            if (currentFile != null)
            {
                string write = WriteToFile();

                if (!string.IsNullOrEmpty(write))
                {
                    File.WriteAllText(currentFile, write);
                }
            }
        }

        private void SaveFileAs(object o, EventArgs e)
        {
            string directory = jsonFilePath;

            saveFileAs.InitialDirectory = directory;
            saveFileAs.Filter = "json files|*.json";

            if (saveFileAs.ShowDialog() == DialogResult.OK)
            {
                currentFile = saveFileAs.FileName;
                string write = WriteToFile();

                if (!string.IsNullOrEmpty(write))
                {
                    File.WriteAllText(saveFileAs.FileName, write);
                }
            }
        }

        private void PlayAnimation(object sender, EventArgs e)
        {
            RobotEngineMessenger.instance.SendAnimation(Path.GetFileNameWithoutExtension(currentFile));
        }

        private void SetIPAddress(object o, EventArgs e)
        {
            ipForm.Open(Location);
        }

        private void UpdateConnectionText(object o, EventArgs e)
        {
            connectionToolStripMenuItem.Text = RobotEngineMessenger.instance.ConnectionManager.ConnectionText;
        }

        private void ChangeConnectionText(string connectionText)
        {
            // is sometimes call on another thread
            if (InvokeRequired)
            {
                Invoke((Action<string>)ChangeConnectionText, connectionText);
                return;
            }

            //connectionTextComponent.Text = connectionText;
        }

        private void MainForm_Resize(object sender, EventArgs e)
        {
            if (headAngle.Size.Width != Size.Width - 35)
            {
                headAngle.Size = new System.Drawing.Size(Size.Width - 35, headAngle.Size.Height);
                liftHeight.Size = new System.Drawing.Size(Size.Width - 35, liftHeight.Size.Height);
                bodyMotion.Size = new System.Drawing.Size(Size.Width - 35, bodyMotion.Size.Height);
                proceduralFace.Size = new System.Drawing.Size(Size.Width - 35, proceduralFace.Size.Height);
                faceAnimation.Size = new System.Drawing.Size(Size.Width - 35, faceAnimation.Size.Height);
                audioRobot.Size = new System.Drawing.Size(Size.Width - 35, audioRobot.Size.Height);
                audioDevice.Size = new System.Drawing.Size(Size.Width - 35, audioDevice.Size.Height);
            }
        }
    }
}
