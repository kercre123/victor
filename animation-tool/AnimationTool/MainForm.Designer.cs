using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using System.ComponentModel;

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
            this.animationToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.setIPAddressToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.playAnimationToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.pHeadAngle = new System.Windows.Forms.Panel();
            this.cHeadAngle = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.pHeadAngleSide = new System.Windows.Forms.Panel();
            this.cbHeadAngle = new System.Windows.Forms.CheckBox();
            this.pLift = new System.Windows.Forms.Panel();
            this.cLiftHeight = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.pLiftSide = new System.Windows.Forms.Panel();
            this.cbLift = new System.Windows.Forms.CheckBox();
            this.pBodyMotion = new System.Windows.Forms.Panel();
            this.cBodyMotion = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.pBodyMotionSide = new System.Windows.Forms.Panel();
            this.cbBodyMotion = new System.Windows.Forms.CheckBox();
            this.pFaceAnimation = new System.Windows.Forms.Panel();
            this.cFaceAnimation = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.pFaceAnimationSide = new System.Windows.Forms.Panel();
            this.pbFaceAnimation = new System.Windows.Forms.PictureBox();
            this.cbFaceAnimation = new System.Windows.Forms.CheckBox();
            this.pAudioRobot = new System.Windows.Forms.Panel();
            this.cAudioRobot = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.pAudioRobotSide = new System.Windows.Forms.Panel();
            this.cbAudioRobot = new System.Windows.Forms.CheckBox();
            this.pAudioDevice = new System.Windows.Forms.Panel();
            this.cAudioDevice = new System.Windows.Forms.DataVisualization.Charting.Chart();
            this.pAudioDeviceSide = new System.Windows.Forms.Panel();
            this.cbAudioDevice = new System.Windows.Forms.CheckBox();
            this.menuStrip.SuspendLayout();
            this.pHeadAngle.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.cHeadAngle)).BeginInit();
            this.pHeadAngleSide.SuspendLayout();
            this.pLift.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.cLiftHeight)).BeginInit();
            this.pLiftSide.SuspendLayout();
            this.pBodyMotion.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.cBodyMotion)).BeginInit();
            this.pBodyMotionSide.SuspendLayout();
            this.pFaceAnimation.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.cFaceAnimation)).BeginInit();
            this.pFaceAnimationSide.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pbFaceAnimation)).BeginInit();
            this.pAudioRobot.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.cAudioRobot)).BeginInit();
            this.pAudioRobotSide.SuspendLayout();
            this.pAudioDevice.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.cAudioDevice)).BeginInit();
            this.pAudioDeviceSide.SuspendLayout();
            this.SuspendLayout();
            // 
            // menuStrip
            // 
            this.menuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.fileToolStripMenuItem,
            this.editToolStripMenuItem,
            this.settingsToolStripMenuItem,
            this.animationToolStripMenuItem});
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
            this.durationToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
            this.durationToolStripMenuItem.Text = "Duration";
            this.durationToolStripMenuItem.Click += new System.EventHandler(this.ChangeDuration);
            // 
            // rootDirectoryToolStripMenuItem
            // 
            this.rootDirectoryToolStripMenuItem.Name = "rootDirectoryToolStripMenuItem";
            this.rootDirectoryToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
            this.rootDirectoryToolStripMenuItem.Text = "Root Directory";
            this.rootDirectoryToolStripMenuItem.Click += new System.EventHandler(this.SetRootDirectory);
            // 
            // setIPAddressToolStripMenuItem
            // 
            this.setIPAddressToolStripMenuItem.Name = "setIPAddressToolStripMenuItem";
            this.setIPAddressToolStripMenuItem.Size = new System.Drawing.Size(193, 22);
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
            this.playAnimationToolStripMenuItem.ShortcutKeyDisplayString = System.Windows.Forms.Keys.Space.ToString();
            this.playAnimationToolStripMenuItem.Size = new System.Drawing.Size(193, 22);
            this.playAnimationToolStripMenuItem.Text = "Play Animation";
            this.playAnimationToolStripMenuItem.Click += new System.EventHandler(this.PlayAnimation);
            // 
            // pHeadAngle
            // 
            this.pHeadAngle.Controls.Add(this.cHeadAngle);
            this.pHeadAngle.Controls.Add(this.pHeadAngleSide);
            this.pHeadAngle.Dock = System.Windows.Forms.DockStyle.Top;
            this.pHeadAngle.Location = new System.Drawing.Point(0, 24);
            this.pHeadAngle.Margin = new System.Windows.Forms.Padding(2);
            this.pHeadAngle.Name = "pHeadAngle";
            this.pHeadAngle.Size = new System.Drawing.Size(1280, 130);
            this.pHeadAngle.TabIndex = 2;
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
            this.cHeadAngle.Size = new System.Drawing.Size(1170, 130);
            this.cHeadAngle.TabIndex = 4;
            this.cHeadAngle.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Chart_MouseClick);
            this.cHeadAngle.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.ChartXY_MouseDoubleClick);
            this.cHeadAngle.MouseDown += new System.Windows.Forms.MouseEventHandler(this.ChartXY_MouseDown);
            this.cHeadAngle.MouseMove += new System.Windows.Forms.MouseEventHandler(this.ChartXY_MouseMove);
            // 
            // pHeadAngleSide
            // 
            this.pHeadAngleSide.BackColor = System.Drawing.SystemColors.AppWorkspace;
            this.pHeadAngleSide.Controls.Add(this.cbHeadAngle);
            this.pHeadAngleSide.Dock = System.Windows.Forms.DockStyle.Right;
            this.pHeadAngleSide.Location = new System.Drawing.Point(1170, 0);
            this.pHeadAngleSide.Margin = new System.Windows.Forms.Padding(2);
            this.pHeadAngleSide.Name = "pHeadAngleSide";
            this.pHeadAngleSide.Size = new System.Drawing.Size(110, 130);
            this.pHeadAngleSide.TabIndex = 3;
            // 
            // cbHeadAngle
            // 
            this.cbHeadAngle.AutoSize = true;
            this.cbHeadAngle.Location = new System.Drawing.Point(11, 11);
            this.cbHeadAngle.Margin = new System.Windows.Forms.Padding(2);
            this.cbHeadAngle.Name = "cbHeadAngle";
            this.cbHeadAngle.Size = new System.Drawing.Size(103, 17);
            this.cbHeadAngle.TabIndex = 4;
            this.cbHeadAngle.Text = "Disable Channel";
            this.cbHeadAngle.UseVisualStyleBackColor = true;
            this.cbHeadAngle.CheckStateChanged += new System.EventHandler(this.HeadAngleCheckBox);
            // 
            // pLift
            // 
            this.pLift.Controls.Add(this.cLiftHeight);
            this.pLift.Controls.Add(this.pLiftSide);
            this.pLift.Dock = System.Windows.Forms.DockStyle.Top;
            this.pLift.Location = new System.Drawing.Point(0, 154);
            this.pLift.Margin = new System.Windows.Forms.Padding(2);
            this.pLift.Name = "pLift";
            this.pLift.Size = new System.Drawing.Size(1280, 130);
            this.pLift.TabIndex = 3;
            // 
            // cLiftHeight
            // 
            this.cLiftHeight.BorderlineColor = System.Drawing.Color.Transparent;
            this.cLiftHeight.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
            chartArea2.AxisX.Interval = 0.1D;
            chartArea2.AxisX.IsLabelAutoFit = false;
            chartArea2.AxisX.LabelAutoFitMaxFontSize = 8;
            chartArea2.AxisX.LabelAutoFitMinFontSize = 8;
            chartArea2.AxisX.LabelStyle.Interval = 0.5D;
            chartArea2.AxisX.MajorGrid.Interval = 1D;
            chartArea2.AxisX.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea2.AxisX.MajorGrid.LineWidth = 2;
            chartArea2.AxisX.Maximum = 6D;
            chartArea2.AxisX.Minimum = 0D;
            chartArea2.AxisX.MinorGrid.Enabled = true;
            chartArea2.AxisX.MinorGrid.Interval = 0.1D;
            chartArea2.AxisX.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea2.AxisX.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea2.AxisX2.Maximum = 6D;
            chartArea2.AxisX2.Minimum = 0D;
            chartArea2.AxisY.Interval = 10D;
            chartArea2.AxisY.IntervalOffset = 5D;
            chartArea2.AxisY.LabelStyle.Interval = 10D;
            chartArea2.AxisY.LabelStyle.IntervalOffset = 5D;
            chartArea2.AxisY.MajorGrid.Interval = 5D;
            chartArea2.AxisY.MajorGrid.IntervalOffset = 0D;
            chartArea2.AxisY.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea2.AxisY.MajorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea2.AxisY.MajorTickMark.Interval = 10D;
            chartArea2.AxisY.Maximum = 95D;
            chartArea2.AxisY.Minimum = 35D;
            chartArea2.AxisY.Title = "Lift Height\\n[mm]";
            chartArea2.AxisY.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea2.AxisY.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea2.AxisY2.Maximum = 95D;
            chartArea2.AxisY2.Minimum = 35D;
            chartArea2.Name = "ChartArea1";
            chartArea2.Position.Auto = false;
            chartArea2.Position.Height = 94F;
            chartArea2.Position.Width = 100F;
            chartArea2.Position.Y = 3F;
            this.cLiftHeight.ChartAreas.Add(chartArea2);
            this.cLiftHeight.Dock = System.Windows.Forms.DockStyle.Fill;
            this.cLiftHeight.Location = new System.Drawing.Point(0, 0);
            this.cLiftHeight.Margin = new System.Windows.Forms.Padding(2);
            this.cLiftHeight.Name = "cLiftHeight";
            series2.BorderDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            series2.BorderWidth = 3;
            series2.ChartArea = "ChartArea1";
            series2.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.Line;
            series2.Color = System.Drawing.Color.LightSkyBlue;
            series2.MarkerBorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(192)))), ((int)(((byte)(0)))));
            series2.MarkerBorderWidth = 0;
            series2.MarkerColor = System.Drawing.Color.FromArgb(((int)(((byte)(128)))), ((int)(((byte)(255)))), ((int)(((byte)(128)))));
            series2.MarkerSize = 8;
            series2.MarkerStyle = System.Windows.Forms.DataVisualization.Charting.MarkerStyle.Square;
            series2.Name = "SeriesKeyframes";
            this.cLiftHeight.Series.Add(series2);
            this.cLiftHeight.Size = new System.Drawing.Size(1170, 130);
            this.cLiftHeight.TabIndex = 4;
            this.cLiftHeight.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Chart_MouseClick);
            this.cLiftHeight.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.ChartXY_MouseDoubleClick);
            this.cLiftHeight.MouseDown += new System.Windows.Forms.MouseEventHandler(this.ChartXY_MouseDown);
            this.cLiftHeight.MouseMove += new System.Windows.Forms.MouseEventHandler(this.ChartXY_MouseMove);
            // 
            // pLiftSide
            // 
            this.pLiftSide.Controls.Add(this.cbLift);
            this.pLiftSide.Dock = System.Windows.Forms.DockStyle.Right;
            this.pLiftSide.Location = new System.Drawing.Point(1170, 0);
            this.pLiftSide.Margin = new System.Windows.Forms.Padding(2);
            this.pLiftSide.Name = "pLiftSide";
            this.pLiftSide.Size = new System.Drawing.Size(110, 130);
            this.pLiftSide.TabIndex = 3;
            // 
            // cbLift
            // 
            this.cbLift.AutoSize = true;
            this.cbLift.Location = new System.Drawing.Point(11, 11);
            this.cbLift.Margin = new System.Windows.Forms.Padding(2);
            this.cbLift.Name = "cbLift";
            this.cbLift.Size = new System.Drawing.Size(103, 17);
            this.cbLift.TabIndex = 4;
            this.cbLift.Text = "Disable Channel";
            this.cbLift.UseVisualStyleBackColor = true;
            this.cbLift.CheckStateChanged += new System.EventHandler(this.LiftHeightCheckBox);
            // 
            // pBodyMotion
            // 
            this.pBodyMotion.Controls.Add(this.cBodyMotion);
            this.pBodyMotion.Controls.Add(this.pBodyMotionSide);
            this.pBodyMotion.Dock = System.Windows.Forms.DockStyle.Top;
            this.pBodyMotion.Location = new System.Drawing.Point(0, 284);
            this.pBodyMotion.Margin = new System.Windows.Forms.Padding(2);
            this.pBodyMotion.Name = "pBodyMotion";
            this.pBodyMotion.Size = new System.Drawing.Size(1280, 94);
            this.pBodyMotion.TabIndex = 4;
            // 
            // cBodyMotion
            // 
            this.cBodyMotion.BorderlineColor = System.Drawing.Color.Transparent;
            this.cBodyMotion.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
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
            chartArea3.AxisX.Title = "Body\\nMotion\\n ";
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
            this.cBodyMotion.ChartAreas.Add(chartArea3);
            this.cBodyMotion.Dock = System.Windows.Forms.DockStyle.Fill;
            this.cBodyMotion.Location = new System.Drawing.Point(0, 0);
            this.cBodyMotion.Margin = new System.Windows.Forms.Padding(2);
            this.cBodyMotion.Name = "cBodyMotion";
            series3.BorderColor = System.Drawing.Color.DarkBlue;
            series3.ChartArea = "ChartArea1";
            series3.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.RangeBar;
            series3.Color = System.Drawing.Color.LightSkyBlue;
            series3.MarkerBorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(192)))), ((int)(((byte)(0)))));
            series3.MarkerBorderWidth = 0;
            series3.MarkerColor = System.Drawing.Color.FromArgb(((int)(((byte)(128)))), ((int)(((byte)(255)))), ((int)(((byte)(128)))));
            series3.MarkerSize = 8;
            series3.Name = "SeriesKeyframes";
            series3.YValuesPerPoint = 2;
            this.cBodyMotion.Series.Add(series3);
            this.cBodyMotion.Size = new System.Drawing.Size(1170, 94);
            this.cBodyMotion.SuppressExceptions = true;
            this.cBodyMotion.TabIndex = 4;
            this.cBodyMotion.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Chart_MouseClick);
            this.cBodyMotion.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDoubleClick);
            this.cBodyMotion.MouseDown += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDown);
            this.cBodyMotion.MouseMove += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseMove);
            // 
            // pBodyMotionSide
            // 
            this.pBodyMotionSide.BackColor = System.Drawing.SystemColors.AppWorkspace;
            this.pBodyMotionSide.Controls.Add(this.cbBodyMotion);
            this.pBodyMotionSide.Dock = System.Windows.Forms.DockStyle.Right;
            this.pBodyMotionSide.Location = new System.Drawing.Point(1170, 0);
            this.pBodyMotionSide.Margin = new System.Windows.Forms.Padding(2);
            this.pBodyMotionSide.Name = "pBodyMotionSide";
            this.pBodyMotionSide.Size = new System.Drawing.Size(110, 94);
            this.pBodyMotionSide.TabIndex = 3;
            // 
            // cbBodyMotion
            // 
            this.cbBodyMotion.AutoSize = true;
            this.cbBodyMotion.Location = new System.Drawing.Point(11, 11);
            this.cbBodyMotion.Margin = new System.Windows.Forms.Padding(2);
            this.cbBodyMotion.Name = "cbBodyMotion";
            this.cbBodyMotion.Size = new System.Drawing.Size(103, 17);
            this.cbBodyMotion.TabIndex = 4;
            this.cbBodyMotion.Text = "Disable Channel";
            this.cbBodyMotion.UseVisualStyleBackColor = true;
            this.cbBodyMotion.CheckStateChanged += new System.EventHandler(this.BodyMotionCheckBox);
            // 
            // pFaceAnimation
            // 
            this.pFaceAnimation.Controls.Add(this.cFaceAnimation);
            this.pFaceAnimation.Controls.Add(this.pFaceAnimationSide);
            this.pFaceAnimation.Dock = System.Windows.Forms.DockStyle.Top;
            this.pFaceAnimation.Location = new System.Drawing.Point(0, 378);
            this.pFaceAnimation.Margin = new System.Windows.Forms.Padding(2);
            this.pFaceAnimation.Name = "pFaceAnimation";
            this.pFaceAnimation.Size = new System.Drawing.Size(1280, 94);
            this.pFaceAnimation.TabIndex = 6;
            // 
            // cFaceAnimation
            // 
            this.cFaceAnimation.BorderlineColor = System.Drawing.Color.Transparent;
            this.cFaceAnimation.BorderlineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Solid;
            chartArea4.AxisX.Interval = 1D;
            chartArea4.AxisX.IsLabelAutoFit = false;
            chartArea4.AxisX.LabelAutoFitMaxFontSize = 8;
            chartArea4.AxisX.LabelAutoFitMinFontSize = 8;
            chartArea4.AxisX.LabelStyle.Enabled = false;
            chartArea4.AxisX.LabelStyle.Interval = 1D;
            chartArea4.AxisX.MajorGrid.Interval = 1D;
            chartArea4.AxisX.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea4.AxisX.MajorGrid.LineWidth = 2;
            chartArea4.AxisX.MajorTickMark.Interval = 1D;
            chartArea4.AxisX.MajorTickMark.IntervalOffset = 0.5D;
            chartArea4.AxisX.Maximum = 1D;
            chartArea4.AxisX.Minimum = 0D;
            chartArea4.AxisX.MinorGrid.Interval = 0.1D;
            chartArea4.AxisX.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea4.AxisX.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea4.AxisX.Title = "Face\\nDisplay\\n ";
            chartArea4.AxisX.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea4.AxisX.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea4.AxisY.IntervalOffset = 1D;
            chartArea4.AxisY.IsLabelAutoFit = false;
            chartArea4.AxisY.LabelStyle.Interval = 0.5D;
            chartArea4.AxisY.LabelStyle.IntervalOffset = 0D;
            chartArea4.AxisY.MajorGrid.Interval = 1D;
            chartArea4.AxisY.MajorGrid.IntervalOffset = 0D;
            chartArea4.AxisY.MajorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea4.AxisY.MajorGrid.LineWidth = 2;
            chartArea4.AxisY.MajorTickMark.Interval = 1D;
            chartArea4.AxisY.Maximum = 6D;
            chartArea4.AxisY.Minimum = 0D;
            chartArea4.AxisY.MinorGrid.Enabled = true;
            chartArea4.AxisY.MinorGrid.Interval = 0.1D;
            chartArea4.AxisY.MinorGrid.LineColor = System.Drawing.Color.LightGray;
            chartArea4.AxisY.MinorGrid.LineDashStyle = System.Windows.Forms.DataVisualization.Charting.ChartDashStyle.Dash;
            chartArea4.AxisY.MinorTickMark.Enabled = true;
            chartArea4.AxisY.MinorTickMark.Interval = 0.1D;
            chartArea4.AxisY.TitleFont = new System.Drawing.Font("Verdana", 11F);
            chartArea4.AxisY.TitleForeColor = System.Drawing.Color.LightGray;
            chartArea4.Name = "ChartArea1";
            chartArea4.Position.Auto = false;
            chartArea4.Position.Height = 94F;
            chartArea4.Position.Width = 98F;
            chartArea4.Position.X = 2F;
            chartArea4.Position.Y = 3F;
            this.cFaceAnimation.ChartAreas.Add(chartArea4);
            this.cFaceAnimation.Dock = System.Windows.Forms.DockStyle.Fill;
            this.cFaceAnimation.Location = new System.Drawing.Point(0, 0);
            this.cFaceAnimation.Margin = new System.Windows.Forms.Padding(2);
            this.cFaceAnimation.Name = "cFaceAnimation";
            series4.BorderColor = System.Drawing.Color.DarkBlue;
            series4.ChartArea = "ChartArea1";
            series4.ChartType = System.Windows.Forms.DataVisualization.Charting.SeriesChartType.RangeBar;
            series4.Color = System.Drawing.Color.LightSkyBlue;
            series4.Font = new System.Drawing.Font("Microsoft Sans Serif", 5F);
            series4.LabelForeColor = System.Drawing.Color.DeepSkyBlue;
            series4.MarkerBorderColor = System.Drawing.Color.FromArgb(((int)(((byte)(0)))), ((int)(((byte)(192)))), ((int)(((byte)(0)))));
            series4.MarkerBorderWidth = 0;
            series4.MarkerColor = System.Drawing.Color.FromArgb(((int)(((byte)(128)))), ((int)(((byte)(255)))), ((int)(((byte)(128)))));
            series4.MarkerSize = 8;
            series4.Name = "SeriesKeyframes";
            series4.YValuesPerPoint = 2;
            this.cFaceAnimation.Series.Add(series4);
            this.cFaceAnimation.Size = new System.Drawing.Size(1170, 94);
            this.cFaceAnimation.SuppressExceptions = true;
            this.cFaceAnimation.TabIndex = 4;
            this.cFaceAnimation.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Chart_MouseClick);
            this.cFaceAnimation.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDoubleClick);
            this.cFaceAnimation.MouseDown += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDown);
            this.cFaceAnimation.MouseMove += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseMove);
            // 
            // pFaceAnimationSide
            // 
            this.pFaceAnimationSide.Controls.Add(this.pbFaceAnimation);
            this.pFaceAnimationSide.Controls.Add(this.cbFaceAnimation);
            this.pFaceAnimationSide.Dock = System.Windows.Forms.DockStyle.Right;
            this.pFaceAnimationSide.Location = new System.Drawing.Point(1170, 0);
            this.pFaceAnimationSide.Margin = new System.Windows.Forms.Padding(2);
            this.pFaceAnimationSide.Name = "pFaceAnimationSide";
            this.pFaceAnimationSide.Size = new System.Drawing.Size(110, 94);
            this.pFaceAnimationSide.TabIndex = 3;
            // 
            // pbFaceAnimation
            // 
            this.pbFaceAnimation.Location = new System.Drawing.Point(5, 39);
            this.pbFaceAnimation.Name = "pbFaceAnimation";
            this.pbFaceAnimation.Size = new System.Drawing.Size(100, 50);
            this.pbFaceAnimation.TabIndex = 5;
            this.pbFaceAnimation.TabStop = false;
            // 
            // cbFaceAnimation
            // 
            this.cbFaceAnimation.AutoSize = true;
            this.cbFaceAnimation.Location = new System.Drawing.Point(11, 11);
            this.cbFaceAnimation.Margin = new System.Windows.Forms.Padding(2);
            this.cbFaceAnimation.Name = "cbFaceAnimation";
            this.cbFaceAnimation.Size = new System.Drawing.Size(103, 17);
            this.cbFaceAnimation.TabIndex = 4;
            this.cbFaceAnimation.Text = "Disable Channel";
            this.cbFaceAnimation.UseVisualStyleBackColor = true;
            this.cbFaceAnimation.CheckStateChanged += new System.EventHandler(this.FaceCheckBox);
            // 
            // pAudioRobot
            // 
            this.pAudioRobot.Controls.Add(this.cAudioRobot);
            this.pAudioRobot.Controls.Add(this.pAudioRobotSide);
            this.pAudioRobot.Dock = System.Windows.Forms.DockStyle.Top;
            this.pAudioRobot.Location = new System.Drawing.Point(0, 472);
            this.pAudioRobot.Margin = new System.Windows.Forms.Padding(2);
            this.pAudioRobot.Name = "pAudioRobot";
            this.pAudioRobot.Size = new System.Drawing.Size(1280, 94);
            this.pAudioRobot.TabIndex = 7;
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
            this.cAudioRobot.Size = new System.Drawing.Size(1170, 94);
            this.cAudioRobot.SuppressExceptions = true;
            this.cAudioRobot.TabIndex = 4;
            this.cAudioRobot.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Chart_MouseClick);
            this.cAudioRobot.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDoubleClick);
            this.cAudioRobot.MouseDown += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDown);
            this.cAudioRobot.MouseMove += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseMove);
            // 
            // pAudioRobotSide
            // 
            this.pAudioRobotSide.BackColor = System.Drawing.SystemColors.AppWorkspace;
            this.pAudioRobotSide.Controls.Add(this.cbAudioRobot);
            this.pAudioRobotSide.Dock = System.Windows.Forms.DockStyle.Right;
            this.pAudioRobotSide.Location = new System.Drawing.Point(1170, 0);
            this.pAudioRobotSide.Margin = new System.Windows.Forms.Padding(2);
            this.pAudioRobotSide.Name = "pAudioRobotSide";
            this.pAudioRobotSide.Size = new System.Drawing.Size(110, 94);
            this.pAudioRobotSide.TabIndex = 3;
            // 
            // cbAudioRobot
            // 
            this.cbAudioRobot.AutoSize = true;
            this.cbAudioRobot.Location = new System.Drawing.Point(11, 11);
            this.cbAudioRobot.Margin = new System.Windows.Forms.Padding(2);
            this.cbAudioRobot.Name = "cbAudioRobot";
            this.cbAudioRobot.Size = new System.Drawing.Size(103, 17);
            this.cbAudioRobot.TabIndex = 4;
            this.cbAudioRobot.Text = "Disable Channel";
            this.cbAudioRobot.UseVisualStyleBackColor = true;
            this.cbAudioRobot.CheckStateChanged += new System.EventHandler(this.AudioRobotCheckBox);
            // 
            // pAudioDevice
            // 
            this.pAudioDevice.Controls.Add(this.cAudioDevice);
            this.pAudioDevice.Controls.Add(this.pAudioDeviceSide);
            this.pAudioDevice.Dock = System.Windows.Forms.DockStyle.Top;
            this.pAudioDevice.Location = new System.Drawing.Point(0, 566);
            this.pAudioDevice.Margin = new System.Windows.Forms.Padding(2);
            this.pAudioDevice.Name = "pAudioDevice";
            this.pAudioDevice.Size = new System.Drawing.Size(1280, 94);
            this.pAudioDevice.TabIndex = 8;
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
            this.cAudioDevice.Size = new System.Drawing.Size(1170, 94);
            this.cAudioDevice.SuppressExceptions = true;
            this.cAudioDevice.TabIndex = 4;
            this.cAudioDevice.MouseClick += new System.Windows.Forms.MouseEventHandler(this.Chart_MouseClick);
            this.cAudioDevice.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDoubleClick);
            this.cAudioDevice.MouseDown += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseDown);
            this.cAudioDevice.MouseMove += new System.Windows.Forms.MouseEventHandler(this.Sequencer_MouseMove);
            // 
            // pAudioDeviceSide
            // 
            this.pAudioDeviceSide.BackColor = System.Drawing.SystemColors.AppWorkspace;
            this.pAudioDeviceSide.Controls.Add(this.cbAudioDevice);
            this.pAudioDeviceSide.Dock = System.Windows.Forms.DockStyle.Right;
            this.pAudioDeviceSide.Location = new System.Drawing.Point(1170, 0);
            this.pAudioDeviceSide.Margin = new System.Windows.Forms.Padding(2);
            this.pAudioDeviceSide.Name = "pAudioDeviceSide";
            this.pAudioDeviceSide.Size = new System.Drawing.Size(110, 94);
            this.pAudioDeviceSide.TabIndex = 3;
            // 
            // cbAudioDevice
            // 
            this.cbAudioDevice.AutoSize = true;
            this.cbAudioDevice.Location = new System.Drawing.Point(11, 11);
            this.cbAudioDevice.Margin = new System.Windows.Forms.Padding(2);
            this.cbAudioDevice.Name = "cbAudioDevice";
            this.cbAudioDevice.Size = new System.Drawing.Size(103, 17);
            this.cbAudioDevice.TabIndex = 4;
            this.cbAudioDevice.Text = "Disable Channel";
            this.cbAudioDevice.UseVisualStyleBackColor = true;
            this.cbAudioDevice.CheckStateChanged += new System.EventHandler(this.AudioDeviceCheckBox);
            // 
            // MainForm
            // 
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(1280, 720);
            this.Controls.Add(this.pAudioDevice);
            this.Controls.Add(this.pAudioRobot);
            this.Controls.Add(this.pFaceAnimation);
            this.Controls.Add(this.pBodyMotion);
            this.Controls.Add(this.pLift);
            this.Controls.Add(this.pHeadAngle);
            this.Controls.Add(this.menuStrip);
            this.KeyPreview = true;
            this.MainMenuStrip = this.menuStrip;
            this.Margin = new System.Windows.Forms.Padding(2);
            this.MinimumSize = new System.Drawing.Size(853, 480);
            this.Name = "MainForm";
            this.Text = "CBD";
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.menuStrip.ResumeLayout(false);
            this.menuStrip.PerformLayout();
            this.pHeadAngle.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.cHeadAngle)).EndInit();
            this.pHeadAngleSide.ResumeLayout(false);
            this.pHeadAngleSide.PerformLayout();
            this.pLift.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.cLiftHeight)).EndInit();
            this.pLiftSide.ResumeLayout(false);
            this.pLiftSide.PerformLayout();
            this.pBodyMotion.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.cBodyMotion)).EndInit();
            this.pBodyMotionSide.ResumeLayout(false);
            this.pBodyMotionSide.PerformLayout();
            this.pFaceAnimation.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.cFaceAnimation)).EndInit();
            this.pFaceAnimationSide.ResumeLayout(false);
            this.pFaceAnimationSide.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pbFaceAnimation)).EndInit();
            this.pAudioRobot.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.cAudioRobot)).EndInit();
            this.pAudioRobotSide.ResumeLayout(false);
            this.pAudioRobotSide.PerformLayout();
            this.pAudioDevice.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.cAudioDevice)).EndInit();
            this.pAudioDeviceSide.ResumeLayout(false);
            this.pAudioDeviceSide.PerformLayout();
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
        private Panel pHeadAngle;
        private Panel pHeadAngleSide;
        private Chart cHeadAngle;
        private CheckBox cbHeadAngle;
        private Panel pLift;
        private Chart cLiftHeight;
        private Panel pLiftSide;
        private CheckBox cbLift;
        private Panel pBodyMotion;
        private Chart cBodyMotion;
        private Panel pBodyMotionSide;
        private CheckBox cbBodyMotion;
        private Panel pFaceAnimation;
        private Chart cFaceAnimation;
        private Panel pFaceAnimationSide;
        private CheckBox cbFaceAnimation;
        private Panel pAudioRobot;
        private Chart cAudioRobot;
        private Panel pAudioRobotSide;
        private CheckBox cbAudioRobot;
        private Panel pAudioDevice;
        private Chart cAudioDevice;
        private Panel pAudioDeviceSide;
        private CheckBox cbAudioDevice;
        private PictureBox pbFaceAnimation;
        private ToolStripMenuItem playAnimationToolStripMenuItem;
        private ToolStripMenuItem setIPAddressToolStripMenuItem;
    }
}

