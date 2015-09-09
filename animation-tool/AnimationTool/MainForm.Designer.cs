using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using System.ComponentModel;
using System.Drawing;

namespace AnimationTool
{
    partial class MainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea1 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Series series1 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea2 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Series series2 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea3 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Series series3 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea4 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Series series4 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea5 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Series series5 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea6 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Series series6 = new System.Windows.Forms.DataVisualization.Charting.Series();
            System.Windows.Forms.DataVisualization.Charting.ChartArea chartArea7 = new System.Windows.Forms.DataVisualization.Charting.ChartArea();
            System.Windows.Forms.DataVisualization.Charting.Series series7 = new System.Windows.Forms.DataVisualization.Charting.Series();
            this.menuStrip = new System.Windows.Forms.MenuStrip();
            this.fileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.newToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.openToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.saveAsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.editToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.undoToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.redoToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.deleteToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.settingsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.durationToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.rootDirectoryToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.setIPAddressToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.animationToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.playAnimationToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.connectionToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.cHeadAngle = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.cBodyMotion = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.cFaceAnimation = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.cProceduralFace = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.cLiftHeight = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.cAudioRobot = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.cAudioDevice = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.headAngle = new AnimationTool.ChartForm();
            this.liftHeight = new AnimationTool.ChartForm();
            this.bodyMotion = new AnimationTool.ChartForm();
            this.proceduralFace = new AnimationTool.ChartForm();
            this.faceAnimation = new AnimationTool.ChartForm();
            this.audioRobot = new AnimationTool.ChartForm();
            this.audioDevice = new AnimationTool.ChartForm();
            this.menuStrip.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.cHeadAngle)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.cBodyMotion)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.cFaceAnimation)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.cLiftHeight)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.cAudioRobot)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.cAudioDevice)).BeginInit();
            this.SuspendLayout();
            // 
            // menuStrip
            // 
            this.menuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.editToolStripMenuItem,
            this.settingsToolStripMenuItem,
            this.animationToolStripMenuItem,
            this.connectionToolStripMenuItem});
            this.menuStrip.Location = new System.Drawing.Point(0, 0);
            this.menuStrip.Name = "menuStrip";
            this.menuStrip.Padding = new System.Windows.Forms.Padding(3, 1, 0, 1);
            this.menuStrip.Size = new System.Drawing.Size(1280, 24);
            this.menuStrip.TabIndex = 0;
            this.menuStrip.Text = "menuStrip";
            // 
            // fileToolStripMenuItem
            // 
            this.fileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.newToolStripMenuItem,
            this.openToolStripMenuItem,
            this.saveToolStripMenuItem,
            this.saveAsToolStripMenuItem});
            this.fileToolStripMenuItem.Name = "fileToolStripMenuItem";
            this.fileToolStripMenuItem.Size = new System.Drawing.Size(37, 22);
            this.fileToolStripMenuItem.Text = "File";
            // 
            // newToolStripMenuItem
            // 
            this.newToolStripMenuItem.Name = "newToolStripMenuItem";
            this.newToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.N)));
            this.newToolStripMenuItem.Size = new System.Drawing.Size(186, 22);
            this.newToolStripMenuItem.Text = "New";
            this.newToolStripMenuItem.Click += new System.EventHandler(this.NewFile);
            // 
            // openToolStripMenuItem
            // 
            this.openToolStripMenuItem.Name = "openToolStripMenuItem";
            this.openToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.O)));
            this.openToolStripMenuItem.Size = new System.Drawing.Size(186, 22);
            this.openToolStripMenuItem.Text = "Open";
            this.openToolStripMenuItem.Click += new System.EventHandler(this.OpenFile);
            // 
            // saveToolStripMenuItem
            // 
            this.saveToolStripMenuItem.Enabled = false;
            this.saveToolStripMenuItem.Name = "saveToolStripMenuItem";
            this.saveToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.S)));
            this.saveToolStripMenuItem.Size = new System.Drawing.Size(186, 22);
            this.saveToolStripMenuItem.Text = "Save";
            this.saveToolStripMenuItem.Click += new System.EventHandler(this.SaveFile);
            // 
            // saveAsToolStripMenuItem
            // 
            this.saveAsToolStripMenuItem.Name = "saveAsToolStripMenuItem";
            this.saveAsToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)(((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Shift) 
            | System.Windows.Forms.Keys.S)));
            this.saveAsToolStripMenuItem.Size = new System.Drawing.Size(186, 22);
            this.saveAsToolStripMenuItem.Text = "Save As";
            this.saveAsToolStripMenuItem.Click += new System.EventHandler(this.SaveFileAs);
            // 
            // editToolStripMenuItem
            // 
            this.editToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.undoToolStripMenuItem,
            this.redoToolStripMenuItem,
            this.deleteToolStripMenuItem});
            this.editToolStripMenuItem.Name = "editToolStripMenuItem";
            this.editToolStripMenuItem.Size = new System.Drawing.Size(39, 22);
            this.editToolStripMenuItem.Text = "Edit";
            // 
            // undoToolStripMenuItem
            // 
            this.undoToolStripMenuItem.Name = "undoToolStripMenuItem";
            this.undoToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Z)));
            this.undoToolStripMenuItem.Size = new System.Drawing.Size(174, 22);
            this.undoToolStripMenuItem.Text = "Undo";
            this.undoToolStripMenuItem.Click += new System.EventHandler(this.Undo);
            // 
            // redoToolStripMenuItem
            // 
            this.redoToolStripMenuItem.Name = "redoToolStripMenuItem";
            this.redoToolStripMenuItem.ShortcutKeys = ((System.Windows.Forms.Keys)(((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Shift) 
            | System.Windows.Forms.Keys.Z)));
            this.redoToolStripMenuItem.Size = new System.Drawing.Size(174, 22);
            this.redoToolStripMenuItem.Text = "Redo";
            this.redoToolStripMenuItem.Click += new System.EventHandler(this.Redo);
            // 
            // deleteToolStripMenuItem
            // 
            this.deleteToolStripMenuItem.Name = "deleteToolStripMenuItem";
            this.deleteToolStripMenuItem.ShortcutKeys = System.Windows.Forms.Keys.Delete;
            this.deleteToolStripMenuItem.Size = new System.Drawing.Size(174, 22);
            this.deleteToolStripMenuItem.Text = "Delete";
            this.deleteToolStripMenuItem.Click += new System.EventHandler(this.Delete);
            // 
            // settingsToolStripMenuItem
            // 
            this.settingsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.durationToolStripMenuItem,
            this.rootDirectoryToolStripMenuItem,
            this.setIPAddressToolStripMenuItem});
            this.settingsToolStripMenuItem.Name = "settingsToolStripMenuItem";
            this.settingsToolStripMenuItem.Size = new System.Drawing.Size(61, 22);
            this.settingsToolStripMenuItem.Text = "Settings";
            // 
            // durationToolStripMenuItem
            // 
            this.durationToolStripMenuItem.Name = "durationToolStripMenuItem";
            this.durationToolStripMenuItem.Size = new System.Drawing.Size(150, 22);
            this.durationToolStripMenuItem.Text = "Duration";
            this.durationToolStripMenuItem.Click += new System.EventHandler(this.ChangeDuration);
            // 
            // rootDirectoryToolStripMenuItem
            // 
            this.rootDirectoryToolStripMenuItem.Name = "rootDirectoryToolStripMenuItem";
            this.rootDirectoryToolStripMenuItem.Size = new System.Drawing.Size(150, 22);
            this.rootDirectoryToolStripMenuItem.Text = "Root Directory";
            this.rootDirectoryToolStripMenuItem.Click += new System.EventHandler(this.SetRootDirectory);
            // 
            // setIPAddressToolStripMenuItem
            // 
            this.setIPAddressToolStripMenuItem.Name = "setIPAddressToolStripMenuItem";
            this.setIPAddressToolStripMenuItem.Size = new System.Drawing.Size(150, 22);
            this.setIPAddressToolStripMenuItem.Text = "Engine IP";
            this.setIPAddressToolStripMenuItem.Click += new System.EventHandler(this.SetIPAddress);
            // 
            // animationToolStripMenuItem
            // 
            this.animationToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.playAnimationToolStripMenuItem});
            this.animationToolStripMenuItem.Name = "animationToolStripMenuItem";
            this.animationToolStripMenuItem.Size = new System.Drawing.Size(75, 22);
            this.animationToolStripMenuItem.Text = "Animation";
            // 
            // playAnimationToolStripMenuItem
            // 
            this.playAnimationToolStripMenuItem.Name = "playAnimationToolStripMenuItem";
            this.playAnimationToolStripMenuItem.ShortcutKeyDisplayString = "Space";
            this.playAnimationToolStripMenuItem.Size = new System.Drawing.Size(193, 22);
            this.playAnimationToolStripMenuItem.Text = "Play Animation";
            this.playAnimationToolStripMenuItem.Click += new System.EventHandler(this.PlayAnimation);
            // 
            // connectionToolStripMenuItem
            // 
            this.connectionToolStripMenuItem.Name = "connectionToolStripMenuItem";
            this.connectionToolStripMenuItem.Size = new System.Drawing.Size(99, 22);
            this.connectionToolStripMenuItem.Text = "[Disconnected]";
            // 
            // cHeadAngle
            // 
            this.cHeadAngle.BorderlineColor = System.Drawing.Color.Transparent;
            this.cHeadAngle.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
            chartArea1.AxisX.Interval = 0.1D;
            chartArea1.AxisX.IsLabelAutoFit = false;
            chartArea1.AxisX.LabelAutoFitMaxFontSize = 8;
            chartArea1.AxisX.LabelAutoFitMinFontSize = 8;
            chartArea1.AxisX.LabelStyle.Interval = 0.5D;
            chartArea1.AxisX.LabelStyle.IntervalOffset = 0D;
            chartArea1.AxisX.MajorGrid.Interval = 1D;
            chartArea1.AxisX.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea1.AxisX.MajorGrid.LineWidth = 2;
            chartArea1.AxisX.Maximum = 6D;
            chartArea1.AxisX.Minimum = 0D;
            chartArea1.AxisX.MinorGrid.Enabled = true;
            chartArea1.AxisX.MinorGrid.Interval = 0.1D;
            chartArea1.AxisX.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea1.AxisX.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea1.AxisX2.Maximum = 6D;
            chartArea1.AxisX2.Minimum = 0D;
            chartArea1.AxisY.Interval = 10D;
            chartArea1.AxisY.IntervalOffset = 5D;
            chartArea1.AxisY.IsLabelAutoFit = false;
            chartArea1.AxisY.LabelStyle.Interval = 10D;
            chartArea1.AxisY.LabelStyle.IntervalOffset = 5D;
            chartArea1.AxisY.MajorGrid.Interval = 5D;
            chartArea1.AxisY.MajorGrid.IntervalOffset = 0D;
            chartArea1.AxisY.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea1.AxisY.MajorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea1.AxisY.MajorTickMark.Interval = 10D;
            chartArea1.AxisY.Maximum = 35D;
            chartArea1.AxisY.Minimum = -25D;
            chartArea1.AxisY.Title = "Head Angle\\n[deg]";
            chartArea1.AxisY.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea1.AxisY.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea1.AxisY2.Maximum = 35D;
            chartArea1.AxisY2.Minimum = -25D;
            chartArea1.Name = "ChartArea1";
            chartArea1.Position.Auto = false;
            chartArea1.Position.Height = 94F;
            chartArea1.Position.Width = 100F;
            chartArea1.Position.Y = 3F;
            this.cHeadAngle.ChartAreas.Add(chartArea1);
            this.cHeadAngle.Dock = System.Windows.Forms.DockStyle.Fill;
            this.cHeadAngle.Location = new System.Drawing.Point(0, 0);
            this.cHeadAngle.Margin = new System.Windows.Forms.Padding(2);
            this.cHeadAngle.Name = "cHeadAngle";
            series1.BorderDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            series1.BorderWidth = 3;
            series1.ChartArea = "ChartArea1";
            series1.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Line;
            series1.Color = System.Drawing.Color.LightSkyBlue;
            series1.MarkerBorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(192)))), ((int)(((byte)(0)))));
            series1.MarkerBorderWidth = 0;
            series1.MarkerColor = System.Drawing.Color.FromArgb(((int)(((byte)(128)))), ((int)(((byte)(255)))), ((int)(((byte)(128)))));
            series1.MarkerSize = 8;
            series1.MarkerStyle = System.Windows.Forms.DataVisualization.Charting.MarkerStyle.Square;
            series1.Name = "SeriesKeyframes";
            this.cHeadAngle.Series.Add(series1);
            this.cHeadAngle.Size = new System.Drawing.Size(1154, 134);
            this.cHeadAngle.TabIndex = 4;
            this.cHeadAngle.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Chart_MouseClick);
            this.cHeadAngle.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.ChartXY_MouseDoubleClick);
            this.cHeadAngle.MouseDown += new System.Windows.Forms.MouseEventHandler(this.ChartXY_MouseDown);
            this.cHeadAngle.MouseMove += new System.Windows.Forms.MouseEventHandler(this.ChartXY_MouseMove);
            // 
            // cBodyMotion
            // 
            this.cBodyMotion.BorderlineColor = System.Drawing.Color.Transparent;
            this.cBodyMotion.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
            chartArea2.AxisX.Interval = 1D;
            chartArea2.AxisX.IsLabelAutoFit = false;
            chartArea2.AxisX.LabelAutoFitMaxFontSize = 8;
            chartArea2.AxisX.LabelAutoFitMinFontSize = 8;
            chartArea2.AxisX.LabelStyle.Enabled = false;
            chartArea2.AxisX.LabelStyle.Interval = 1D;
            chartArea2.AxisX.MajorGrid.Interval = 1D;
            chartArea2.AxisX.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea2.AxisX.MajorGrid.LineWidth = 2;
            chartArea2.AxisX.MajorTickMark.Interval = 1D;
            chartArea2.AxisX.MajorTickMark.IntervalOffset = 0.5D;
            chartArea2.AxisX.Maximum = 1D;
            chartArea2.AxisX.Minimum = 0D;
            chartArea2.AxisX.MinorGrid.Interval = 0.1D;
            chartArea2.AxisX.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea2.AxisX.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea2.AxisX.Title = "Body\\nMotion\\n ";
            chartArea2.AxisX.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea2.AxisX.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea2.AxisY.IntervalOffset = 1D;
            chartArea2.AxisY.IsLabelAutoFit = false;
            chartArea2.AxisY.LabelStyle.Interval = 0.5D;
            chartArea2.AxisY.LabelStyle.IntervalOffset = 0D;
            chartArea2.AxisY.MajorGrid.Interval = 1D;
            chartArea2.AxisY.MajorGrid.IntervalOffset = 0D;
            chartArea2.AxisY.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea2.AxisY.MajorGrid.LineWidth = 2;
            chartArea2.AxisY.MajorTickMark.Interval = 1D;
            chartArea2.AxisY.Maximum = 6D;
            chartArea2.AxisY.Minimum = 0D;
            chartArea2.AxisY.MinorGrid.Enabled = true;
            chartArea2.AxisY.MinorGrid.Interval = 0.1D;
            chartArea2.AxisY.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea2.AxisY.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea2.AxisY.MinorTickMark.Enabled = true;
            chartArea2.AxisY.MinorTickMark.Interval = 0.1D;
            chartArea2.AxisY.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea2.AxisY.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea2.Name = "ChartArea1";
            chartArea2.Position.Auto = false;
            chartArea2.Position.Height = 94F;
            chartArea2.Position.Width = 98F;
            chartArea2.Position.X = 2F;
            chartArea2.Position.Y = 3F;
            this.cBodyMotion.ChartAreas.Add(chartArea2);
            this.cBodyMotion.Dock = System.Windows.Forms.DockStyle.Fill;
            this.cBodyMotion.Location = new System.Drawing.Point(0, 0);
            this.cBodyMotion.Margin = new System.Windows.Forms.Padding(2);
            this.cBodyMotion.Name = "cBodyMotion";
            series2.BorderColor = System.Drawing.Color.DarkBlue;
            series2.ChartArea = "ChartArea1";
            series2.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.RangeBar;
            series2.Color = System.Drawing.Color.LightSkyBlue;
            series2.MarkerBorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(192)))), ((int)(((byte)(0)))));
            series2.MarkerBorderWidth = 0;
            series2.MarkerColor = System.Drawing.Color.FromArgb(((int)(((byte)(128)))), ((int)(((byte)(255)))), ((int)(((byte)(128)))));
            series2.MarkerSize = 8;
            series2.Name = "SeriesKeyframes";
            series2.YValuesPerPoint = 2;
            this.cBodyMotion.Series.Add(series2);
            this.cBodyMotion.Size = new System.Drawing.Size(1154, 134);
            this.cBodyMotion.SuppressExceptions = true;
            this.cBodyMotion.TabIndex = 4;
            this.cBodyMotion.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Chart_MouseClick);
            this.cBodyMotion.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDoubleClick);
            this.cBodyMotion.MouseDown += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDown);
            this.cBodyMotion.MouseMove += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseMove);
            // 
            // cFaceAnimation
            // 
            this.cFaceAnimation.BorderlineColor = System.Drawing.Color.Transparent;
            this.cFaceAnimation.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
            chartArea3.AxisX.Interval = 1D;
            chartArea3.AxisX.IsLabelAutoFit = false;
            chartArea3.AxisX.LabelAutoFitMaxFontSize = 8;
            chartArea3.AxisX.LabelAutoFitMinFontSize = 8;
            chartArea3.AxisX.LabelStyle.Enabled = false;
            chartArea3.AxisX.LabelStyle.Interval = 1D;
            chartArea3.AxisX.MajorGrid.Interval = 1D;
            chartArea3.AxisX.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea3.AxisX.MajorGrid.LineWidth = 2;
            chartArea3.AxisX.MajorTickMark.Interval = 1D;
            chartArea3.AxisX.MajorTickMark.IntervalOffset = 0.5D;
            chartArea3.AxisX.Maximum = 1D;
            chartArea3.AxisX.Minimum = 0D;
            chartArea3.AxisX.MinorGrid.Interval = 0.1D;
            chartArea3.AxisX.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea3.AxisX.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea3.AxisX.Title = "Face\\nDisplay\\n\\n";
            chartArea3.AxisX.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea3.AxisX.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea3.AxisY.IntervalOffset = 1D;
            chartArea3.AxisY.IsLabelAutoFit = false;
            chartArea3.AxisY.LabelStyle.Interval = 0.5D;
            chartArea3.AxisY.LabelStyle.IntervalOffset = 0D;
            chartArea3.AxisY.MajorGrid.Interval = 1D;
            chartArea3.AxisY.MajorGrid.IntervalOffset = 0D;
            chartArea3.AxisY.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea3.AxisY.MajorGrid.LineWidth = 2;
            chartArea3.AxisY.MajorTickMark.Interval = 1D;
            chartArea3.AxisY.Maximum = 6D;
            chartArea3.AxisY.Minimum = 0D;
            chartArea3.AxisY.MinorGrid.Enabled = true;
            chartArea3.AxisY.MinorGrid.Interval = 0.1D;
            chartArea3.AxisY.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea3.AxisY.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea3.AxisY.MinorTickMark.Enabled = true;
            chartArea3.AxisY.MinorTickMark.Interval = 0.1D;
            chartArea3.AxisY.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea3.AxisY.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea3.Name = "ChartArea1";
            chartArea3.Position.Auto = false;
            chartArea3.Position.Height = 94F;
            chartArea3.Position.Width = 98F;
            chartArea3.Position.X = 2F;
            chartArea3.Position.Y = 3F;
            this.cFaceAnimation.ChartAreas.Add(chartArea3);
            this.cFaceAnimation.Dock = System.Windows.Forms.DockStyle.Fill;
            this.cFaceAnimation.Location = new System.Drawing.Point(0, 0);
            this.cFaceAnimation.Margin = new System.Windows.Forms.Padding(2);
            this.cFaceAnimation.Name = "cFaceAnimation";
            series3.BorderColor = System.Drawing.Color.DarkBlue;
            series3.ChartArea = "ChartArea1";
            series3.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.RangeBar;
            series3.Color = System.Drawing.Color.LightSkyBlue;
            series3.Font = new System.Drawing.Font("Microsoft Sans Serif", 5F);
            series3.LabelForeColor = System.Drawing.Color.DeepSkyBlue;
            series3.MarkerBorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(192)))), ((int)(((byte)(0)))));
            series3.MarkerBorderWidth = 0;
            series3.MarkerColor = System.Drawing.Color.FromArgb(((int)(((byte)(128)))), ((int)(((byte)(255)))), ((int)(((byte)(128)))));
            series3.MarkerSize = 8;
            series3.Name = "SeriesKeyframes";
            series3.YValuesPerPoint = 2;
            this.cFaceAnimation.Series.Add(series3);
            this.cFaceAnimation.Size = new System.Drawing.Size(1154, 134);
            this.cFaceAnimation.SuppressExceptions = true;
            this.cFaceAnimation.TabIndex = 4;
            this.cFaceAnimation.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Chart_MouseClick);
            this.cFaceAnimation.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDoubleClick);
            this.cFaceAnimation.MouseDown += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDown);
            this.cFaceAnimation.MouseMove += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseMove);
            // 
            // cProceduralFace
            // 
            this.cProceduralFace.BorderlineColor = System.Drawing.Color.Transparent;
            this.cProceduralFace.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
            chartArea7.AxisX.Interval = 1D;
            chartArea7.AxisX.IsLabelAutoFit = false;
            chartArea7.AxisX.LabelAutoFitMaxFontSize = 8;
            chartArea7.AxisX.LabelAutoFitMinFontSize = 8;
            chartArea7.AxisX.LabelStyle.Enabled = false;
            chartArea7.AxisX.LabelStyle.Interval = 1D;
            chartArea7.AxisX.MajorGrid.Interval = 1D;
            chartArea7.AxisX.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea7.AxisX.MajorGrid.LineWidth = 2;
            chartArea7.AxisX.MajorTickMark.Interval = 1D;
            chartArea7.AxisX.MajorTickMark.IntervalOffset = 0.5D;
            chartArea7.AxisX.Maximum = 1D;
            chartArea7.AxisX.Minimum = 0D;
            chartArea7.AxisX.MinorGrid.Interval = 0.1D;
            chartArea7.AxisX.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea7.AxisX.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea7.AxisX.Title = "Face\\nRig\\n\\n";
            chartArea7.AxisX.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea7.AxisX.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea7.AxisY.IntervalOffset = 1D;
            chartArea7.AxisY.IsLabelAutoFit = false;
            chartArea7.AxisY.LabelStyle.Interval = 0.5D;
            chartArea7.AxisY.LabelStyle.IntervalOffset = 0D;
            chartArea7.AxisY.MajorGrid.Interval = 1D;
            chartArea7.AxisY.MajorGrid.IntervalOffset = 0D;
            chartArea7.AxisY.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea7.AxisY.MajorGrid.LineWidth = 2;
            chartArea7.AxisY.MajorTickMark.Interval = 1D;
            chartArea7.AxisY.Maximum = 6D;
            chartArea7.AxisY.Minimum = 0D;
            chartArea7.AxisY.MinorGrid.Enabled = true;
            chartArea7.AxisY.MinorGrid.Interval = 0.1D;
            chartArea7.AxisY.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea7.AxisY.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea7.AxisY.MinorTickMark.Enabled = true;
            chartArea7.AxisY.MinorTickMark.Interval = 0.1D;
            chartArea7.AxisY.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea7.AxisY.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea7.Name = "ChartArea1";
            chartArea7.Position.Auto = false;
            chartArea7.Position.Height = 94F;
            chartArea7.Position.Width = 98F;
            chartArea7.Position.X = 2F;
            chartArea7.Position.Y = 3F;
            this.cProceduralFace.ChartAreas.Add(chartArea7);
            this.cProceduralFace.Dock = System.Windows.Forms.DockStyle.Fill;
            this.cProceduralFace.Location = new System.Drawing.Point(0, 0);
            this.cProceduralFace.Margin = new System.Windows.Forms.Padding(2);
            this.cProceduralFace.Name = "cProceduralFace";
            series7.BorderColor = System.Drawing.Color.DarkBlue;
            series7.ChartArea = "ChartArea1";
            series7.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.RangeBar;
            series7.Color = System.Drawing.Color.LightSkyBlue;
            series7.Font = new System.Drawing.Font("Microsoft Sans Serif", 5F);
            series7.LabelForeColor = System.Drawing.Color.DeepSkyBlue;
            series7.MarkerBorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(192)))), ((int)(((byte)(0)))));
            series7.MarkerBorderWidth = 0;
            series7.MarkerColor = System.Drawing.Color.FromArgb(((int)(((byte)(128)))), ((int)(((byte)(255)))), ((int)(((byte)(128)))));
            series7.MarkerSize = 8;
            series7.Name = "SeriesKeyframes";
            series7.YValuesPerPoint = 2;
            this.cProceduralFace.Series.Add(series7);
            this.cProceduralFace.Size = new System.Drawing.Size(1154, 134);
            this.cProceduralFace.SuppressExceptions = true;
            this.cProceduralFace.TabIndex = 4;
            this.cProceduralFace.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Chart_MouseClick);
            this.cProceduralFace.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDoubleClick);
            this.cProceduralFace.MouseDown += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDown);
            this.cProceduralFace.MouseMove += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseMove);
            // 
            // cLiftHeight
            // 
            this.cLiftHeight.BorderlineColor = System.Drawing.Color.Transparent;
            this.cLiftHeight.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
            chartArea4.AxisX.Interval = 0.1D;
            chartArea4.AxisX.IsLabelAutoFit = false;
            chartArea4.AxisX.LabelAutoFitMaxFontSize = 8;
            chartArea4.AxisX.LabelAutoFitMinFontSize = 8;
            chartArea4.AxisX.LabelStyle.Interval = 0.5D;
            chartArea4.AxisX.MajorGrid.Interval = 1D;
            chartArea4.AxisX.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea4.AxisX.MajorGrid.LineWidth = 2;
            chartArea4.AxisX.Maximum = 6D;
            chartArea4.AxisX.Minimum = 0D;
            chartArea4.AxisX.MinorGrid.Enabled = true;
            chartArea4.AxisX.MinorGrid.Interval = 0.1D;
            chartArea4.AxisX.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea4.AxisX.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea4.AxisX2.Maximum = 6D;
            chartArea4.AxisX2.Minimum = 0D;
            chartArea4.AxisY.Interval = 10D;
            chartArea4.AxisY.IntervalOffset = 5D;
            chartArea4.AxisY.LabelStyle.Interval = 10D;
            chartArea4.AxisY.LabelStyle.IntervalOffset = 5D;
            chartArea4.AxisY.MajorGrid.Interval = 5D;
            chartArea4.AxisY.MajorGrid.IntervalOffset = 0D;
            chartArea4.AxisY.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea4.AxisY.MajorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea4.AxisY.MajorTickMark.Interval = 10D;
            chartArea4.AxisY.Maximum = 95D;
            chartArea4.AxisY.Minimum = 35D;
            chartArea4.AxisY.Title = "Lift Height\\n[mm]";
            chartArea4.AxisY.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea4.AxisY.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea4.AxisY2.Maximum = 95D;
            chartArea4.AxisY2.Minimum = 35D;
            chartArea4.Name = "ChartArea1";
            chartArea4.Position.Auto = false;
            chartArea4.Position.Height = 94F;
            chartArea4.Position.Width = 100F;
            chartArea4.Position.Y = 3F;
            this.cLiftHeight.ChartAreas.Add(chartArea4);
            this.cLiftHeight.Dock = System.Windows.Forms.DockStyle.Fill;
            this.cLiftHeight.Location = new System.Drawing.Point(0, 0);
            this.cLiftHeight.Margin = new System.Windows.Forms.Padding(2);
            this.cLiftHeight.Name = "cLiftHeight";
            series4.BorderDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            series4.BorderWidth = 3;
            series4.ChartArea = "ChartArea1";
            series4.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Line;
            series4.Color = System.Drawing.Color.LightSkyBlue;
            series4.MarkerBorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(192)))), ((int)(((byte)(0)))));
            series4.MarkerBorderWidth = 0;
            series4.MarkerColor = System.Drawing.Color.FromArgb(((int)(((byte)(128)))), ((int)(((byte)(255)))), ((int)(((byte)(128)))));
            series4.MarkerSize = 8;
            series4.MarkerStyle = System.Windows.Forms.DataVisualization.Charting.MarkerStyle.Square;
            series4.Name = "SeriesKeyframes";
            this.cLiftHeight.Series.Add(series4);
            this.cLiftHeight.Size = new System.Drawing.Size(1154, 134);
            this.cLiftHeight.TabIndex = 4;
            this.cLiftHeight.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Chart_MouseClick);
            this.cLiftHeight.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.ChartXY_MouseDoubleClick);
            this.cLiftHeight.MouseDown += new System.Windows.Forms.MouseEventHandler(this.ChartXY_MouseDown);
            this.cLiftHeight.MouseMove += new System.Windows.Forms.MouseEventHandler(this.ChartXY_MouseMove);
            // 
            // cAudioRobot
            // 
            this.cAudioRobot.BorderlineColor = System.Drawing.Color.Transparent;
            this.cAudioRobot.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
            chartArea5.AxisX.Interval = 1D;
            chartArea5.AxisX.IsLabelAutoFit = false;
            chartArea5.AxisX.LabelAutoFitMaxFontSize = 8;
            chartArea5.AxisX.LabelAutoFitMinFontSize = 8;
            chartArea5.AxisX.LabelStyle.Enabled = false;
            chartArea5.AxisX.LabelStyle.Interval = 1D;
            chartArea5.AxisX.MajorGrid.Interval = 1D;
            chartArea5.AxisX.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea5.AxisX.MajorGrid.LineWidth = 2;
            chartArea5.AxisX.MajorTickMark.Interval = 1D;
            chartArea5.AxisX.MajorTickMark.IntervalOffset = 0.5D;
            chartArea5.AxisX.Maximum = 1D;
            chartArea5.AxisX.Minimum = 0D;
            chartArea5.AxisX.MinorGrid.Interval = 0.1D;
            chartArea5.AxisX.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea5.AxisX.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea5.AxisX.Title = "Audio\\nRobot\\n ";
            chartArea5.AxisX.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea5.AxisX.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea5.AxisY.IntervalOffset = 1D;
            chartArea5.AxisY.IsLabelAutoFit = false;
            chartArea5.AxisY.LabelStyle.Interval = 0.5D;
            chartArea5.AxisY.LabelStyle.IntervalOffset = 0D;
            chartArea5.AxisY.MajorGrid.Interval = 1D;
            chartArea5.AxisY.MajorGrid.IntervalOffset = 0D;
            chartArea5.AxisY.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea5.AxisY.MajorGrid.LineWidth = 2;
            chartArea5.AxisY.MajorTickMark.Interval = 1D;
            chartArea5.AxisY.Maximum = 6D;
            chartArea5.AxisY.Minimum = 0D;
            chartArea5.AxisY.MinorGrid.Enabled = true;
            chartArea5.AxisY.MinorGrid.Interval = 0.1D;
            chartArea5.AxisY.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea5.AxisY.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea5.AxisY.MinorTickMark.Enabled = true;
            chartArea5.AxisY.MinorTickMark.Interval = 0.1D;
            chartArea5.AxisY.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea5.AxisY.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea5.Name = "ChartArea1";
            chartArea5.Position.Auto = false;
            chartArea5.Position.Height = 94F;
            chartArea5.Position.Width = 98F;
            chartArea5.Position.X = 2F;
            chartArea5.Position.Y = 3F;
            this.cAudioRobot.ChartAreas.Add(chartArea5);
            this.cAudioRobot.Dock = System.Windows.Forms.DockStyle.Fill;
            this.cAudioRobot.Location = new System.Drawing.Point(0, 0);
            this.cAudioRobot.Margin = new System.Windows.Forms.Padding(2);
            this.cAudioRobot.Name = "cAudioRobot";
            series5.BorderColor = System.Drawing.Color.DarkBlue;
            series5.ChartArea = "ChartArea1";
            series5.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.RangeBar;
            series5.Color = System.Drawing.Color.LightSkyBlue;
            series5.Font = new System.Drawing.Font("Microsoft Sans Serif", 5F);
            series5.LabelForeColor = System.Drawing.Color.DeepSkyBlue;
            series5.MarkerBorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(192)))), ((int)(((byte)(0)))));
            series5.MarkerBorderWidth = 0;
            series5.MarkerColor = System.Drawing.Color.FromArgb(((int)(((byte)(128)))), ((int)(((byte)(255)))), ((int)(((byte)(128)))));
            series5.MarkerSize = 8;
            series5.Name = "SeriesKeyframes";
            series5.YValuesPerPoint = 2;
            this.cAudioRobot.Series.Add(series5);
            this.cAudioRobot.Size = new System.Drawing.Size(1264, 134);
            this.cAudioRobot.SuppressExceptions = true;
            this.cAudioRobot.TabIndex = 4;
            this.cAudioRobot.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Chart_MouseClick);
            this.cAudioRobot.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDoubleClick);
            this.cAudioRobot.MouseDown += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDown);
            this.cAudioRobot.MouseMove += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseMove);
            // 
            // cAudioDevice
            // 
            this.cAudioDevice.BorderlineColor = System.Drawing.Color.Transparent;
            this.cAudioDevice.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
            chartArea6.AxisX.Interval = 1D;
            chartArea6.AxisX.IsLabelAutoFit = false;
            chartArea6.AxisX.LabelAutoFitMaxFontSize = 8;
            chartArea6.AxisX.LabelAutoFitMinFontSize = 8;
            chartArea6.AxisX.LabelStyle.Enabled = false;
            chartArea6.AxisX.LabelStyle.Interval = 1D;
            chartArea6.AxisX.MajorGrid.Interval = 1D;
            chartArea6.AxisX.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea6.AxisX.MajorGrid.LineWidth = 2;
            chartArea6.AxisX.MajorTickMark.Interval = 1D;
            chartArea6.AxisX.MajorTickMark.IntervalOffset = 0.5D;
            chartArea6.AxisX.Maximum = 1D;
            chartArea6.AxisX.Minimum = 0D;
            chartArea6.AxisX.MinorGrid.Interval = 0.1D;
            chartArea6.AxisX.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea6.AxisX.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea6.AxisX.Title = "Audio\\nDevice\\n ";
            chartArea6.AxisX.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea6.AxisX.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea6.AxisY.IntervalOffset = 1D;
            chartArea6.AxisY.IsLabelAutoFit = false;
            chartArea6.AxisY.LabelStyle.Interval = 0.5D;
            chartArea6.AxisY.LabelStyle.IntervalOffset = 0D;
            chartArea6.AxisY.MajorGrid.Interval = 1D;
            chartArea6.AxisY.MajorGrid.IntervalOffset = 0D;
            chartArea6.AxisY.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea6.AxisY.MajorGrid.LineWidth = 2;
            chartArea6.AxisY.MajorTickMark.Interval = 1D;
            chartArea6.AxisY.Maximum = 6D;
            chartArea6.AxisY.Minimum = 0D;
            chartArea6.AxisY.MinorGrid.Enabled = true;
            chartArea6.AxisY.MinorGrid.Interval = 0.1D;
            chartArea6.AxisY.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea6.AxisY.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea6.AxisY.MinorTickMark.Enabled = true;
            chartArea6.AxisY.MinorTickMark.Interval = 0.1D;
            chartArea6.AxisY.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea6.AxisY.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea6.Name = "ChartArea1";
            chartArea6.Position.Auto = false;
            chartArea6.Position.Height = 94F;
            chartArea6.Position.Width = 98F;
            chartArea6.Position.X = 2F;
            chartArea6.Position.Y = 3F;
            this.cAudioDevice.ChartAreas.Add(chartArea6);
            this.cAudioDevice.Dock = System.Windows.Forms.DockStyle.Fill;
            this.cAudioDevice.Location = new System.Drawing.Point(0, 0);
            this.cAudioDevice.Margin = new System.Windows.Forms.Padding(2);
            this.cAudioDevice.Name = "cAudioDevice";
            series6.BorderColor = System.Drawing.Color.DarkBlue;
            series6.ChartArea = "ChartArea1";
            series6.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.RangeBar;
            series6.Color = System.Drawing.Color.LightSkyBlue;
            series6.Font = new System.Drawing.Font("Microsoft Sans Serif", 5F);
            series6.LabelForeColor = System.Drawing.Color.DeepSkyBlue;
            series6.MarkerBorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(192)))), ((int)(((byte)(0)))));
            series6.MarkerBorderWidth = 0;
            series6.MarkerColor = System.Drawing.Color.FromArgb(((int)(((byte)(128)))), ((int)(((byte)(255)))), ((int)(((byte)(128)))));
            series6.MarkerSize = 8;
            series6.Name = "SeriesKeyframes";
            series6.YValuesPerPoint = 2;
            this.cAudioDevice.Series.Add(series6);
            this.cAudioDevice.Size = new System.Drawing.Size(1264, 134);
            this.cAudioDevice.SuppressExceptions = true;
            this.cAudioDevice.TabIndex = 4;
            this.cAudioDevice.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Chart_MouseClick);
            this.cAudioDevice.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDoubleClick);
            this.cAudioDevice.MouseDown += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDown);
            this.cAudioDevice.MouseMove += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseMove);
            // 
            // headAngle
            // 
            this.headAngle.Location = new System.Drawing.Point(0, 27);
            this.headAngle.Name = "headAngle";
            this.headAngle.Size = new System.Drawing.Size(1280, 135);
            this.headAngle.TabIndex = 9;
            // 
            // liftHeight
            // 
            this.liftHeight.Location = new System.Drawing.Point(0, 168);
            this.liftHeight.Name = "liftHeight";
            this.liftHeight.Size = new System.Drawing.Size(1280, 135);
            this.liftHeight.TabIndex = 9;
            // 
            // bodyMotion
            // 
            this.bodyMotion.Location = new System.Drawing.Point(0, 309);
            this.bodyMotion.Name = "bodyMotion";
            this.bodyMotion.Size = new System.Drawing.Size(1280, 100);
            this.bodyMotion.TabIndex = 9;
            // 
            // proceduralFace
            // 
            this.proceduralFace.Location = new System.Drawing.Point(0, 415);
            this.proceduralFace.Name = "proceduralFace";
            this.proceduralFace.Size = new System.Drawing.Size(1280, 100);
            this.proceduralFace.TabIndex = 9;
            // 
            // faceAnimation
            // 
            this.faceAnimation.Location = new System.Drawing.Point(0, 521);
            this.faceAnimation.Name = "faceAnimation";
            this.faceAnimation.Size = new System.Drawing.Size(1280, 100);
            this.faceAnimation.TabIndex = 9;
            // 
            // audioRobot
            // 
            this.audioRobot.Location = new System.Drawing.Point(0, 627);
            this.audioRobot.Name = "audioRobot";
            this.audioRobot.Size = new System.Drawing.Size(1280, 100);
            this.audioRobot.TabIndex = 9;
            // 
            // audioDevice
            // 
            this.audioDevice.Location = new System.Drawing.Point(0, 733);
            this.audioDevice.Name = "audioDevice";
            this.audioDevice.Size = new System.Drawing.Size(1280, 100);
            this.audioDevice.TabIndex = 9;
            // 
            // MainForm
            // 
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(1281, 698);
            this.Controls.Add(this.menuStrip);
            this.Controls.Add(this.headAngle);
            this.Controls.Add(this.liftHeight);
            this.Controls.Add(this.bodyMotion);
            this.Controls.Add(this.proceduralFace);
            this.Controls.Add(this.faceAnimation);
            this.Controls.Add(this.audioRobot);
            this.Controls.Add(this.audioDevice);
            this.KeyPreview = true;
            this.MainMenuStrip = this.menuStrip;
            this.Margin = new System.Windows.Forms.Padding(2);
            this.MinimumSize = new System.Drawing.Size(1280, 720);
            this.Name = "MainForm";
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.menuStrip.ResumeLayout(false);
            this.menuStrip.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.cHeadAngle)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.cBodyMotion)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.cFaceAnimation)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.cLiftHeight)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.cAudioRobot)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.cAudioDevice)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private MenuStrip menuStrip;
        private ToolStripMenuItem fileToolStripMenuItem;
        private ToolStripMenuItem animationToolStripMenuItem;
        private ToolStripMenuItem newToolStripMenuItem;
        private ToolStripMenuItem openToolStripMenuItem;
        private ToolStripMenuItem saveToolStripMenuItem;
        private ToolStripMenuItem saveAsToolStripMenuItem;
        private ToolStripMenuItem editToolStripMenuItem;
        private ToolStripMenuItem undoToolStripMenuItem;
        private ToolStripMenuItem redoToolStripMenuItem;
        private ToolStripMenuItem deleteToolStripMenuItem;
        private ToolStripMenuItem settingsToolStripMenuItem;
        private ToolStripMenuItem durationToolStripMenuItem;
        private ToolStripMenuItem rootDirectoryToolStripMenuItem;
        private Chart cHeadAngle;
        private Chart cLiftHeight;
        private Chart cBodyMotion;
        private Chart cFaceAnimation;
        private Chart cProceduralFace;
        private Chart cAudioRobot;
        private Chart cAudioDevice;
        private ToolStripMenuItem playAnimationToolStripMenuItem;
        private ToolStripMenuItem setIPAddressToolStripMenuItem;
        private ToolStripMenuItem connectionToolStripMenuItem;
        private ChartForm headAngle;
        private ChartForm liftHeight;
        private ChartForm bodyMotion;
        private ChartForm proceduralFace;
        private ChartForm faceAnimation;
        private ChartForm audioRobot;
        private ChartForm audioDevice;
    }
}

