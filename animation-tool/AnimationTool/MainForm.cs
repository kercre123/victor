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
        private List<ChartForm> chartForms;

        private ChangeDurationForm changeDurationForm;
        private BodyForm bodyForm;
        private VolumeForm volumeForm;
        private StraightForm straightForm;
        private ArcForm arcForm;
        private TurnInPlaceForm turnInPlaceForm;
        private FaceForm faceForm;
        private IPForm ipForm;

        private OpenFileDialog openFile;
        private FolderBrowserDialog selectFolder;
        private SaveFileDialog saveFileAs;

        //reference to the currently selected Chart
        private Chart curChart;
        private ChartArea curChartArea { get { return curChart != null && curChart.ChartAreas.Count > 0 ? curChart.ChartAreas[0] : null; } }
        private DataPointCollection curPoints { get { return curChart != null && curChart.Series.Count > 0 ? curChart.Series[0].Points : null; } }

        private string jsonFilePath { get { return rootDirectory + "\\animations"; } }

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
            chartForms = new List<ChartForm>();

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
        }

        private void MainForm_Load(object sender, EventArgs e)
        {
            if (Sequencer.ExtraData.Entries == null || chartForms == null) return;

            ActionManager.Reset();
            Sequencer.ExtraData.Reset();
            faceForm.Reset();
            chartForms.Clear();

            double interval = ChangeChartDuration.Clamp(Math.Round(Properties.Settings.Default.maxTime * 0.1, 1), 0.1, 0.5);

            //Head Chart
            headAngle.chart.Series[0].Points.Clear();
            headAngle.chart.ChartAreas[0].AxisX.Minimum = 0;
            headAngle.chart.ChartAreas[0].AxisX.Maximum = Properties.Settings.Default.maxTime;
            headAngle.chart.ChartAreas[0].AxisX.LabelStyle.Interval = interval;
            headAngle.chart.ChartAreas[0].AxisY.Minimum = -25;
            headAngle.chart.ChartAreas[0].AxisY.Maximum = 34;
            ActionManager.Do(new DisableChart(headAngle), true);
            ActionManager.Do(new EnableChart(headAngle), true);
            chartForms.Add(headAngle);

            //Lift Chart
            liftHeight.chart.Series[0].Points.Clear();
            liftHeight.chart.ChartAreas[0].AxisX.Minimum = 0;
            liftHeight.chart.ChartAreas[0].AxisX.Maximum = Properties.Settings.Default.maxTime;
            liftHeight.chart.ChartAreas[0].AxisX.LabelStyle.Interval = interval;
            liftHeight.chart.ChartAreas[0].AxisY.Minimum = 31;
            liftHeight.chart.ChartAreas[0].AxisY.Maximum = 92;
            ActionManager.Do(new DisableChart(liftHeight), true);
            ActionManager.Do(new EnableChart(liftHeight), true);
            chartForms.Add(liftHeight);

            //Body Chart
            bodyMotion.chart.Series[0].Points.Clear();
            bodyMotion.chart.ChartAreas[0].AxisY.Minimum = 0;
            bodyMotion.chart.ChartAreas[0].AxisY.Maximum = Properties.Settings.Default.maxTime;
            bodyMotion.chart.ChartAreas[0].AxisY.LabelStyle.Interval = interval;
            ActionManager.Do(new DisableChart(bodyMotion), true);
            ActionManager.Do(new Sequencer.EnableChart(bodyMotion), true);
            chartForms.Add(bodyMotion);

            //Face Animation Data Chart
            proceduralFace.chart.Series[0].Points.Clear();
            proceduralFace.chart.ChartAreas[0].AxisY.Minimum = 0;
            proceduralFace.chart.ChartAreas[0].AxisY.Maximum = Properties.Settings.Default.maxTime;
            proceduralFace.chart.ChartAreas[0].AxisY.LabelStyle.Interval = interval;
            ActionManager.Do(new DisableChart(proceduralFace), true);
            ActionManager.Do(new Sequencer.EnableChart(proceduralFace), true);
            chartForms.Add(proceduralFace);

            //Face Animation Image Chart
            faceAnimation.chart.Series[0].Points.Clear();
            faceAnimation.chart.ChartAreas[0].AxisY.Minimum = 0;
            faceAnimation.chart.ChartAreas[0].AxisY.Maximum = Properties.Settings.Default.maxTime;
            faceAnimation.chart.ChartAreas[0].AxisY.LabelStyle.Interval = interval;
            ActionManager.Do(new DisableChart(faceAnimation), true);
            ActionManager.Do(new FaceAnimation.EnableChart(faceAnimation), true);
            chartForms.Add(faceAnimation);

            //Audio Robot Chart
            audioRobot.chart.Series[0].Points.Clear();
            audioRobot.chart.ChartAreas[0].AxisY.Minimum = 0;
            audioRobot.chart.ChartAreas[0].AxisY.Maximum = Properties.Settings.Default.maxTime;
            audioRobot.chart.ChartAreas[0].AxisY.LabelStyle.Interval = interval;
            ActionManager.Do(new DisableChart(audioRobot), true);
            ActionManager.Do(new Sequencer.EnableChart(audioRobot), true);
            chartForms.Add(audioRobot);

            //Audio Robot Device
            audioDevice.chart.Series[0].Points.Clear();
            audioDevice.chart.ChartAreas[0].AxisY.Minimum = 0;
            audioDevice.chart.ChartAreas[0].AxisY.Maximum = Properties.Settings.Default.maxTime;
            audioDevice.chart.ChartAreas[0].AxisY.LabelStyle.Interval = interval;
            ActionManager.Do(new DisableChart(audioDevice), true);
            ActionManager.Do(new Sequencer.EnableChart(audioDevice), true);
            chartForms.Add(audioDevice);

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
                        faceAnimation.chart.Refresh();
                    }
                }
                else if (ActionManager.Do(new MoveSelectedDataPointsOfSelectedCharts(chartForms, left, right, up, down)))
                {
                    foreach (ChartForm chartForm in chartForms)
                    {
                        chartForm.chart.Refresh();
                    }
                }
            }
        }

        private void Undo(object o, EventArgs e)
        {
            if (chartForms == null) return;

            ActionManager.Undo();

            foreach (ChartForm chartForm in chartForms)
            {
                chartForm.chart.Refresh();
            }
        }

        private void Redo(object o, EventArgs e)
        {
            if (chartForms == null) return;

            ActionManager.Redo();

            foreach (ChartForm chartForm in chartForms)
            {
                chartForm.chart.Refresh();
            }
        }

        private void Delete(object o, EventArgs e)
        {
            if (chartForms == null) return;

            ActionManager.Do(new RemoveSelectedDataPointsOfSelectedCharts(chartForms));

            foreach (ChartForm chartForm in chartForms)
            {
                chartForm.chart.Refresh();
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
                    ActionManager.Do(new TruncateAllChartDurations(chartForms, changeDurationForm.Duration));
                }
                else if (result == DialogResult.Yes) // scale
                {
                    ActionManager.Do(new ScaleAllChartDurations(chartForms, changeDurationForm.Duration));
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
                    MainForm_Load(o, e);
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
            int size = (int)(Size.Width * 0.97);

            if (headAngle.Size.Width != size)
            {
                headAngle.Size = new System.Drawing.Size(size, headAngle.Size.Height);
                liftHeight.Size = new System.Drawing.Size(size, liftHeight.Size.Height);
                bodyMotion.Size = new System.Drawing.Size(size, bodyMotion.Size.Height);
                proceduralFace.Size = new System.Drawing.Size(size, proceduralFace.Size.Height);
                faceAnimation.Size = new System.Drawing.Size(size, faceAnimation.Size.Height);
                audioRobot.Size = new System.Drawing.Size(size, audioRobot.Size.Height);
                audioDevice.Size = new System.Drawing.Size(size, audioDevice.Size.Height);
            }
        }

        private void ReadAndWritesAllFiles(object sender, EventArgs e)
        {
            if (!Directory.Exists(jsonFilePath)) return;

            DirectoryInfo directory = new DirectoryInfo(jsonFilePath);

            foreach (FileInfo file in directory.GetFiles())
            {
                if (!file.IsReadOnly)
                {
                    currentFile = file.FullName;
                    string text = File.ReadAllText(currentFile);

                    if (!string.IsNullOrEmpty(text))
                    {
                        MainForm_Load(sender, e);
                        SaveFile(sender, e);
                    }
                }
            }
        }
    }
}
