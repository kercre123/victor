using System;
using System.Windows.Forms;
using System.Drawing;
using System.Net;

namespace AnimationTool
{
    public class FaceForm : Form
    {
        private Label browLabel;
        private TextBox browLeftAngleTextBox;
        private TrackBar browLeftAngleTrackBar;
        private Label browLeftAngleLabel;
        private Label browRightAngleLabel;
        private TrackBar browRightAngleTrackBar;
        private TextBox browRightAngleTextBox;
        private Label leftLabel;
        private Label rightLabel;
        private Label label5;
        private TrackBar trackBar3;
        private TextBox textBox2;
        private Label label6;
        private TrackBar trackBar4;
        private TextBox textBox3;
        private TextBox textBox4;
        private Label label8;
        private TrackBar trackBar6;
        private TextBox textBox5;
        private Label label17;
        private TrackBar trackBar15;
        private TextBox textBox14;
        private Label label18;
        private TrackBar trackBar16;
        private TextBox textBox15;
        private Label label19;
        private TrackBar trackBar17;
        private TextBox textBox16;
        private Label label20;
        private TrackBar trackBar18;
        private TextBox textBox17;
        private Label rightLabel2;
        private Label leftLabel2;
        private Label label23;
        private TrackBar trackBar19;
        private TextBox textBox18;
        private Label label24;
        private TrackBar trackBar20;
        private Label label25;
        private TextBox textBox19;
        private Label label27;
        private TrackBar trackBar22;
        private TextBox textBox21;
        private Label label29;
        private TrackBar trackBar24;
        private Label label30;
        private TextBox textBox23;
        private Label label7;
        private TrackBar trackBar5;
        private Label label9;
        private TrackBar trackBar7;
        private TextBox textBox6;
        private Label label10;
        private TrackBar trackBar8;
        private TextBox textBox7;
        private Label label11;
        private TrackBar trackBar9;
        private TextBox textBox8;
        private Label label12;
        private TrackBar trackBar10;
        private TextBox textBox9;
        private Label label13;
        private TrackBar trackBar11;
        private TextBox textBox10;
        private Label label14;
        private TrackBar trackBar12;
        private Label label15;
        private TextBox textBox11;
        private Label label16;
        private TrackBar trackBar13;
        private TextBox textBox12;
        private Label label26;
        private TrackBar trackBar14;
        private Button button;
        private Label label;
        private TextBox textBox22;
        private TextBox textBox13;

        public FaceForm()
        {
            FormBorderStyle = FormBorderStyle.FixedToolWindow;
            MaximizeBox = false;
            MinimizeBox = false;

            InitializeComponent();
        }

        public DialogResult Open(Point location)
        {
            StartPosition = FormStartPosition.Manual;
            Location = location;

            if (Properties.Settings.Default.IPAddress != null)
            {
                this.browLeftAngleTextBox.Text = Properties.Settings.Default.IPAddress;
            }

            return ShowDialog();
        }

        private void InitializeComponent()
        {
            this.browLeftAngleTextBox = new System.Windows.Forms.TextBox();
            this.browLabel = new System.Windows.Forms.Label();
            this.browLeftAngleTrackBar = new System.Windows.Forms.TrackBar();
            this.browLeftAngleLabel = new System.Windows.Forms.Label();
            this.browRightAngleLabel = new System.Windows.Forms.Label();
            this.browRightAngleTrackBar = new System.Windows.Forms.TrackBar();
            this.browRightAngleTextBox = new System.Windows.Forms.TextBox();
            this.leftLabel = new System.Windows.Forms.Label();
            this.rightLabel = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.trackBar3 = new System.Windows.Forms.TrackBar();
            this.textBox2 = new System.Windows.Forms.TextBox();
            this.label6 = new System.Windows.Forms.Label();
            this.trackBar4 = new System.Windows.Forms.TrackBar();
            this.textBox3 = new System.Windows.Forms.TextBox();
            this.textBox4 = new System.Windows.Forms.TextBox();
            this.label8 = new System.Windows.Forms.Label();
            this.trackBar6 = new System.Windows.Forms.TrackBar();
            this.textBox5 = new System.Windows.Forms.TextBox();
            this.label17 = new System.Windows.Forms.Label();
            this.trackBar15 = new System.Windows.Forms.TrackBar();
            this.textBox14 = new System.Windows.Forms.TextBox();
            this.label18 = new System.Windows.Forms.Label();
            this.trackBar16 = new System.Windows.Forms.TrackBar();
            this.textBox15 = new System.Windows.Forms.TextBox();
            this.label19 = new System.Windows.Forms.Label();
            this.trackBar17 = new System.Windows.Forms.TrackBar();
            this.textBox16 = new System.Windows.Forms.TextBox();
            this.label20 = new System.Windows.Forms.Label();
            this.trackBar18 = new System.Windows.Forms.TrackBar();
            this.textBox17 = new System.Windows.Forms.TextBox();
            this.rightLabel2 = new System.Windows.Forms.Label();
            this.leftLabel2 = new System.Windows.Forms.Label();
            this.label23 = new System.Windows.Forms.Label();
            this.trackBar19 = new System.Windows.Forms.TrackBar();
            this.textBox18 = new System.Windows.Forms.TextBox();
            this.label24 = new System.Windows.Forms.Label();
            this.trackBar20 = new System.Windows.Forms.TrackBar();
            this.label25 = new System.Windows.Forms.Label();
            this.textBox19 = new System.Windows.Forms.TextBox();
            this.label27 = new System.Windows.Forms.Label();
            this.trackBar22 = new System.Windows.Forms.TrackBar();
            this.textBox21 = new System.Windows.Forms.TextBox();
            this.label29 = new System.Windows.Forms.Label();
            this.trackBar24 = new System.Windows.Forms.TrackBar();
            this.label30 = new System.Windows.Forms.Label();
            this.textBox23 = new System.Windows.Forms.TextBox();
            this.label7 = new System.Windows.Forms.Label();
            this.trackBar5 = new System.Windows.Forms.TrackBar();
            this.label9 = new System.Windows.Forms.Label();
            this.trackBar7 = new System.Windows.Forms.TrackBar();
            this.textBox6 = new System.Windows.Forms.TextBox();
            this.label10 = new System.Windows.Forms.Label();
            this.trackBar8 = new System.Windows.Forms.TrackBar();
            this.textBox7 = new System.Windows.Forms.TextBox();
            this.label11 = new System.Windows.Forms.Label();
            this.trackBar9 = new System.Windows.Forms.TrackBar();
            this.textBox8 = new System.Windows.Forms.TextBox();
            this.label12 = new System.Windows.Forms.Label();
            this.trackBar10 = new System.Windows.Forms.TrackBar();
            this.textBox9 = new System.Windows.Forms.TextBox();
            this.label13 = new System.Windows.Forms.Label();
            this.trackBar11 = new System.Windows.Forms.TrackBar();
            this.textBox10 = new System.Windows.Forms.TextBox();
            this.label14 = new System.Windows.Forms.Label();
            this.trackBar12 = new System.Windows.Forms.TrackBar();
            this.label15 = new System.Windows.Forms.Label();
            this.textBox11 = new System.Windows.Forms.TextBox();
            this.label16 = new System.Windows.Forms.Label();
            this.trackBar13 = new System.Windows.Forms.TrackBar();
            this.textBox12 = new System.Windows.Forms.TextBox();
            this.label26 = new System.Windows.Forms.Label();
            this.trackBar14 = new System.Windows.Forms.TrackBar();
            this.textBox13 = new System.Windows.Forms.TextBox();
            this.button = new System.Windows.Forms.Button();
            this.label = new System.Windows.Forms.Label();
            this.textBox22 = new System.Windows.Forms.TextBox();
            ((System.ComponentModel.ISupportInitialize)(this.browLeftAngleTrackBar)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.browRightAngleTrackBar)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar3)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar4)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar6)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar15)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar16)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar17)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar18)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar19)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar20)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar22)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar24)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar5)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar7)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar8)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar9)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar10)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar11)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar12)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar13)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar14)).BeginInit();
            this.SuspendLayout();
            // 
            // browLeftAngleTextBox
            // 
            this.browLeftAngleTextBox.Location = new System.Drawing.Point(87, 75);
            this.browLeftAngleTextBox.Name = "browLeftAngleTextBox";
            this.browLeftAngleTextBox.Size = new System.Drawing.Size(27, 20);
            this.browLeftAngleTextBox.TabIndex = 0;
            this.browLeftAngleTextBox.Text = "000";
            this.browLeftAngleTextBox.TextChanged += new System.EventHandler(this.textBox_TextChanged);
            // 
            // browLabel
            // 
            this.browLabel.AutoSize = true;
            this.browLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.browLabel.Location = new System.Drawing.Point(12, 48);
            this.browLabel.Name = "browLabel";
            this.browLabel.Size = new System.Drawing.Size(41, 13);
            this.browLabel.TabIndex = 1;
            this.browLabel.Text = "Brows";
            // 
            // browLeftAngleTrackBar
            // 
            this.browLeftAngleTrackBar.LargeChange = 10;
            this.browLeftAngleTrackBar.Location = new System.Drawing.Point(10, 100);
            this.browLeftAngleTrackBar.Margin = new System.Windows.Forms.Padding(0);
            this.browLeftAngleTrackBar.Maximum = 90;
            this.browLeftAngleTrackBar.Name = "browLeftAngleTrackBar";
            this.browLeftAngleTrackBar.Size = new System.Drawing.Size(300, 45);
            this.browLeftAngleTrackBar.TabIndex = 0;
            this.browLeftAngleTrackBar.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // browLeftAngleLabel
            // 
            this.browLeftAngleLabel.AutoSize = true;
            this.browLeftAngleLabel.Location = new System.Drawing.Point(20, 75);
            this.browLeftAngleLabel.Name = "browLeftAngleLabel";
            this.browLeftAngleLabel.Size = new System.Drawing.Size(61, 13);
            this.browLeftAngleLabel.TabIndex = 3;
            this.browLeftAngleLabel.Text = "Angle (deg)";
            // 
            // browRightAngleLabel
            // 
            this.browRightAngleLabel.AutoSize = true;
            this.browRightAngleLabel.Location = new System.Drawing.Point(320, 75);
            this.browRightAngleLabel.Name = "browRightAngleLabel";
            this.browRightAngleLabel.Size = new System.Drawing.Size(61, 13);
            this.browRightAngleLabel.TabIndex = 6;
            this.browRightAngleLabel.Text = "Angle (deg)";
            // 
            // browRightAngleTrackBar
            // 
            this.browRightAngleTrackBar.LargeChange = 10;
            this.browRightAngleTrackBar.Location = new System.Drawing.Point(310, 100);
            this.browRightAngleTrackBar.Margin = new System.Windows.Forms.Padding(0);
            this.browRightAngleTrackBar.Maximum = 90;
            this.browRightAngleTrackBar.Name = "browRightAngleTrackBar";
            this.browRightAngleTrackBar.Size = new System.Drawing.Size(300, 45);
            this.browRightAngleTrackBar.TabIndex = 0;
            this.browRightAngleTrackBar.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // browRightAngleTextBox
            // 
            this.browRightAngleTextBox.Location = new System.Drawing.Point(387, 75);
            this.browRightAngleTextBox.Name = "browRightAngleTextBox";
            this.browRightAngleTextBox.Size = new System.Drawing.Size(27, 20);
            this.browRightAngleTextBox.TabIndex = 4;
            this.browRightAngleTextBox.Text = "000";
            // 
            // leftLabel
            // 
            this.leftLabel.AutoSize = true;
            this.leftLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.leftLabel.Location = new System.Drawing.Point(160, 10);
            this.leftLabel.Name = "leftLabel";
            this.leftLabel.Size = new System.Drawing.Size(29, 13);
            this.leftLabel.TabIndex = 7;
            this.leftLabel.Text = "Left";
            // 
            // rightLabel
            // 
            this.rightLabel.AutoSize = true;
            this.rightLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.rightLabel.Location = new System.Drawing.Point(443, 10);
            this.rightLabel.Name = "rightLabel";
            this.rightLabel.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.rightLabel.Size = new System.Drawing.Size(37, 13);
            this.rightLabel.TabIndex = 8;
            this.rightLabel.Text = "Right";
            this.rightLabel.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Location = new System.Drawing.Point(320, 150);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(14, 13);
            this.label5.TabIndex = 14;
            this.label5.Text = "Y";
            // 
            // trackBar3
            // 
            this.trackBar3.LargeChange = 10;
            this.trackBar3.Location = new System.Drawing.Point(310, 175);
            this.trackBar3.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar3.Maximum = 90;
            this.trackBar3.Name = "trackBar3";
            this.trackBar3.Size = new System.Drawing.Size(300, 45);
            this.trackBar3.TabIndex = 9;
            this.trackBar3.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox2
            // 
            this.textBox2.Location = new System.Drawing.Point(340, 150);
            this.textBox2.Name = "textBox2";
            this.textBox2.Size = new System.Drawing.Size(27, 20);
            this.textBox2.TabIndex = 13;
            this.textBox2.Text = "000";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Location = new System.Drawing.Point(20, 150);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(14, 13);
            this.label6.TabIndex = 12;
            this.label6.Text = "Y";
            // 
            // trackBar4
            // 
            this.trackBar4.LargeChange = 10;
            this.trackBar4.Location = new System.Drawing.Point(10, 175);
            this.trackBar4.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar4.Maximum = 90;
            this.trackBar4.Name = "trackBar4";
            this.trackBar4.Size = new System.Drawing.Size(300, 45);
            this.trackBar4.TabIndex = 10;
            this.trackBar4.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox3
            // 
            this.textBox3.Location = new System.Drawing.Point(40, 150);
            this.textBox3.Name = "textBox3";
            this.textBox3.Size = new System.Drawing.Size(27, 20);
            this.textBox3.TabIndex = 11;
            this.textBox3.Text = "000";
            // 
            // textBox4
            // 
            this.textBox4.Location = new System.Drawing.Point(366, 225);
            this.textBox4.Name = "textBox4";
            this.textBox4.Size = new System.Drawing.Size(27, 20);
            this.textBox4.TabIndex = 19;
            this.textBox4.Text = "000";
            // 
            // label8
            // 
            this.label8.AutoSize = true;
            this.label8.Location = new System.Drawing.Point(20, 225);
            this.label8.Name = "label8";
            this.label8.Size = new System.Drawing.Size(40, 13);
            this.label8.TabIndex = 18;
            this.label8.Text = "Length";
            // 
            // trackBar6
            // 
            this.trackBar6.LargeChange = 10;
            this.trackBar6.Location = new System.Drawing.Point(10, 250);
            this.trackBar6.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar6.Maximum = 90;
            this.trackBar6.Name = "trackBar6";
            this.trackBar6.Size = new System.Drawing.Size(300, 45);
            this.trackBar6.TabIndex = 16;
            this.trackBar6.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox5
            // 
            this.textBox5.Location = new System.Drawing.Point(66, 225);
            this.textBox5.Name = "textBox5";
            this.textBox5.Size = new System.Drawing.Size(27, 20);
            this.textBox5.TabIndex = 17;
            this.textBox5.Text = "000";
            // 
            // label17
            // 
            this.label17.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label17.AutoSize = true;
            this.label17.Location = new System.Drawing.Point(960, 225);
            this.label17.Name = "label17";
            this.label17.Size = new System.Drawing.Size(14, 13);
            this.label17.TabIndex = 53;
            this.label17.Text = "Y";
            // 
            // trackBar15
            // 
            this.trackBar15.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar15.LargeChange = 10;
            this.trackBar15.Location = new System.Drawing.Point(950, 250);
            this.trackBar15.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar15.Maximum = 90;
            this.trackBar15.Name = "trackBar15";
            this.trackBar15.Size = new System.Drawing.Size(300, 45);
            this.trackBar15.TabIndex = 48;
            this.trackBar15.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox14
            // 
            this.textBox14.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox14.Location = new System.Drawing.Point(980, 225);
            this.textBox14.Name = "textBox14";
            this.textBox14.Size = new System.Drawing.Size(27, 20);
            this.textBox14.TabIndex = 52;
            this.textBox14.Text = "000";
            // 
            // label18
            // 
            this.label18.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label18.AutoSize = true;
            this.label18.Location = new System.Drawing.Point(660, 225);
            this.label18.Name = "label18";
            this.label18.Size = new System.Drawing.Size(14, 13);
            this.label18.TabIndex = 51;
            this.label18.Text = "Y";
            // 
            // trackBar16
            // 
            this.trackBar16.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar16.LargeChange = 10;
            this.trackBar16.Location = new System.Drawing.Point(650, 250);
            this.trackBar16.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar16.Maximum = 90;
            this.trackBar16.Name = "trackBar16";
            this.trackBar16.Size = new System.Drawing.Size(300, 45);
            this.trackBar16.TabIndex = 49;
            this.trackBar16.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox15
            // 
            this.textBox15.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox15.Location = new System.Drawing.Point(680, 225);
            this.textBox15.Name = "textBox15";
            this.textBox15.Size = new System.Drawing.Size(27, 20);
            this.textBox15.TabIndex = 50;
            this.textBox15.Text = "000";
            // 
            // label19
            // 
            this.label19.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label19.AutoSize = true;
            this.label19.Location = new System.Drawing.Point(960, 150);
            this.label19.Name = "label19";
            this.label19.Size = new System.Drawing.Size(35, 13);
            this.label19.TabIndex = 47;
            this.label19.Text = "Width";
            // 
            // trackBar17
            // 
            this.trackBar17.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar17.LargeChange = 10;
            this.trackBar17.Location = new System.Drawing.Point(950, 175);
            this.trackBar17.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar17.Maximum = 90;
            this.trackBar17.Name = "trackBar17";
            this.trackBar17.Size = new System.Drawing.Size(300, 45);
            this.trackBar17.TabIndex = 42;
            this.trackBar17.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox16
            // 
            this.textBox16.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox16.Location = new System.Drawing.Point(1001, 150);
            this.textBox16.Name = "textBox16";
            this.textBox16.Size = new System.Drawing.Size(27, 20);
            this.textBox16.TabIndex = 46;
            this.textBox16.Text = "000";
            // 
            // label20
            // 
            this.label20.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label20.AutoSize = true;
            this.label20.Location = new System.Drawing.Point(660, 150);
            this.label20.Name = "label20";
            this.label20.Size = new System.Drawing.Size(35, 13);
            this.label20.TabIndex = 45;
            this.label20.Text = "Width";
            // 
            // trackBar18
            // 
            this.trackBar18.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar18.LargeChange = 10;
            this.trackBar18.Location = new System.Drawing.Point(650, 175);
            this.trackBar18.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar18.Maximum = 90;
            this.trackBar18.Name = "trackBar18";
            this.trackBar18.Size = new System.Drawing.Size(300, 45);
            this.trackBar18.TabIndex = 43;
            this.trackBar18.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox17
            // 
            this.textBox17.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox17.Location = new System.Drawing.Point(701, 150);
            this.textBox17.Name = "textBox17";
            this.textBox17.Size = new System.Drawing.Size(27, 20);
            this.textBox17.TabIndex = 44;
            this.textBox17.Text = "000";
            // 
            // rightLabel2
            // 
            this.rightLabel2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.rightLabel2.AutoSize = true;
            this.rightLabel2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.rightLabel2.Location = new System.Drawing.Point(1083, 10);
            this.rightLabel2.Name = "rightLabel2";
            this.rightLabel2.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.rightLabel2.Size = new System.Drawing.Size(37, 13);
            this.rightLabel2.TabIndex = 41;
            this.rightLabel2.Text = "Right";
            this.rightLabel2.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // leftLabel2
            // 
            this.leftLabel2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.leftLabel2.AutoSize = true;
            this.leftLabel2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.leftLabel2.Location = new System.Drawing.Point(800, 10);
            this.leftLabel2.Name = "leftLabel2";
            this.leftLabel2.Size = new System.Drawing.Size(29, 13);
            this.leftLabel2.TabIndex = 40;
            this.leftLabel2.Text = "Left";
            // 
            // label23
            // 
            this.label23.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label23.AutoSize = true;
            this.label23.Location = new System.Drawing.Point(960, 75);
            this.label23.Name = "label23";
            this.label23.Size = new System.Drawing.Size(38, 13);
            this.label23.TabIndex = 39;
            this.label23.Text = "Height";
            // 
            // trackBar19
            // 
            this.trackBar19.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar19.LargeChange = 10;
            this.trackBar19.Location = new System.Drawing.Point(950, 100);
            this.trackBar19.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar19.Maximum = 90;
            this.trackBar19.Name = "trackBar19";
            this.trackBar19.Size = new System.Drawing.Size(300, 45);
            this.trackBar19.TabIndex = 35;
            this.trackBar19.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox18
            // 
            this.textBox18.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox18.Location = new System.Drawing.Point(1004, 75);
            this.textBox18.Name = "textBox18";
            this.textBox18.Size = new System.Drawing.Size(27, 20);
            this.textBox18.TabIndex = 38;
            this.textBox18.Text = "000";
            // 
            // label24
            // 
            this.label24.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label24.AutoSize = true;
            this.label24.Location = new System.Drawing.Point(660, 75);
            this.label24.Name = "label24";
            this.label24.Size = new System.Drawing.Size(38, 13);
            this.label24.TabIndex = 37;
            this.label24.Text = "Height";
            // 
            // trackBar20
            // 
            this.trackBar20.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar20.LargeChange = 10;
            this.trackBar20.Location = new System.Drawing.Point(650, 100);
            this.trackBar20.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar20.Maximum = 90;
            this.trackBar20.Name = "trackBar20";
            this.trackBar20.Size = new System.Drawing.Size(300, 45);
            this.trackBar20.TabIndex = 34;
            this.trackBar20.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // label25
            // 
            this.label25.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label25.AutoSize = true;
            this.label25.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label25.Location = new System.Drawing.Point(652, 48);
            this.label25.Name = "label25";
            this.label25.Size = new System.Drawing.Size(59, 13);
            this.label25.TabIndex = 36;
            this.label25.Text = "Eye Balls";
            // 
            // textBox19
            // 
            this.textBox19.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox19.Location = new System.Drawing.Point(701, 75);
            this.textBox19.Name = "textBox19";
            this.textBox19.Size = new System.Drawing.Size(27, 20);
            this.textBox19.TabIndex = 33;
            this.textBox19.Text = "000";
            this.textBox19.TextChanged += new System.EventHandler(this.textBox19_TextChanged);
            // 
            // label27
            // 
            this.label27.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label27.AutoSize = true;
            this.label27.Location = new System.Drawing.Point(20, 410);
            this.label27.Name = "label27";
            this.label27.Size = new System.Drawing.Size(14, 13);
            this.label27.TabIndex = 76;
            this.label27.Text = "Y";
            // 
            // trackBar22
            // 
            this.trackBar22.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.trackBar22.LargeChange = 10;
            this.trackBar22.Location = new System.Drawing.Point(10, 435);
            this.trackBar22.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar22.Maximum = 90;
            this.trackBar22.Name = "trackBar22";
            this.trackBar22.Size = new System.Drawing.Size(300, 45);
            this.trackBar22.TabIndex = 74;
            this.trackBar22.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox21
            // 
            this.textBox21.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.textBox21.Location = new System.Drawing.Point(40, 410);
            this.textBox21.Name = "textBox21";
            this.textBox21.Size = new System.Drawing.Size(27, 20);
            this.textBox21.TabIndex = 75;
            this.textBox21.Text = "000";
            // 
            // label29
            // 
            this.label29.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label29.AutoSize = true;
            this.label29.Location = new System.Drawing.Point(20, 335);
            this.label29.Name = "label29";
            this.label29.Size = new System.Drawing.Size(61, 13);
            this.label29.TabIndex = 70;
            this.label29.Text = "Angle [deg]";
            // 
            // trackBar24
            // 
            this.trackBar24.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.trackBar24.LargeChange = 10;
            this.trackBar24.Location = new System.Drawing.Point(10, 360);
            this.trackBar24.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar24.Maximum = 90;
            this.trackBar24.Name = "trackBar24";
            this.trackBar24.Size = new System.Drawing.Size(300, 45);
            this.trackBar24.TabIndex = 67;
            this.trackBar24.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // label30
            // 
            this.label30.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label30.AutoSize = true;
            this.label30.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label30.Location = new System.Drawing.Point(12, 308);
            this.label30.Name = "label30";
            this.label30.Size = new System.Drawing.Size(35, 13);
            this.label30.TabIndex = 69;
            this.label30.Text = "Face";
            // 
            // textBox23
            // 
            this.textBox23.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.textBox23.Location = new System.Drawing.Point(87, 335);
            this.textBox23.Name = "textBox23";
            this.textBox23.Size = new System.Drawing.Size(27, 20);
            this.textBox23.TabIndex = 68;
            this.textBox23.Text = "000";
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Location = new System.Drawing.Point(320, 225);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(40, 13);
            this.label7.TabIndex = 20;
            this.label7.Text = "Length";
            // 
            // trackBar5
            // 
            this.trackBar5.LargeChange = 10;
            this.trackBar5.Location = new System.Drawing.Point(310, 250);
            this.trackBar5.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar5.Maximum = 90;
            this.trackBar5.Name = "trackBar5";
            this.trackBar5.Size = new System.Drawing.Size(300, 45);
            this.trackBar5.TabIndex = 15;
            this.trackBar5.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // label9
            // 
            this.label9.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.label9.AutoSize = true;
            this.label9.Location = new System.Drawing.Point(960, 485);
            this.label9.Name = "label9";
            this.label9.Size = new System.Drawing.Size(14, 13);
            this.label9.TabIndex = 95;
            this.label9.Text = "X";
            // 
            // trackBar7
            // 
            this.trackBar7.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar7.LargeChange = 10;
            this.trackBar7.Location = new System.Drawing.Point(950, 510);
            this.trackBar7.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar7.Maximum = 90;
            this.trackBar7.Name = "trackBar7";
            this.trackBar7.Size = new System.Drawing.Size(300, 45);
            this.trackBar7.TabIndex = 90;
            this.trackBar7.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox6
            // 
            this.textBox6.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox6.Location = new System.Drawing.Point(980, 485);
            this.textBox6.Name = "textBox6";
            this.textBox6.Size = new System.Drawing.Size(27, 20);
            this.textBox6.TabIndex = 94;
            this.textBox6.Text = "000";
            // 
            // label10
            // 
            this.label10.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.label10.AutoSize = true;
            this.label10.Location = new System.Drawing.Point(660, 485);
            this.label10.Name = "label10";
            this.label10.Size = new System.Drawing.Size(14, 13);
            this.label10.TabIndex = 93;
            this.label10.Text = "X";
            // 
            // trackBar8
            // 
            this.trackBar8.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar8.LargeChange = 10;
            this.trackBar8.Location = new System.Drawing.Point(650, 510);
            this.trackBar8.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar8.Maximum = 90;
            this.trackBar8.Name = "trackBar8";
            this.trackBar8.Size = new System.Drawing.Size(300, 45);
            this.trackBar8.TabIndex = 91;
            this.trackBar8.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox7
            // 
            this.textBox7.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox7.Location = new System.Drawing.Point(680, 485);
            this.textBox7.Name = "textBox7";
            this.textBox7.Size = new System.Drawing.Size(27, 20);
            this.textBox7.TabIndex = 92;
            this.textBox7.Text = "000";
            // 
            // label11
            // 
            this.label11.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.label11.AutoSize = true;
            this.label11.Location = new System.Drawing.Point(960, 410);
            this.label11.Name = "label11";
            this.label11.Size = new System.Drawing.Size(35, 13);
            this.label11.TabIndex = 89;
            this.label11.Text = "Width";
            // 
            // trackBar9
            // 
            this.trackBar9.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar9.LargeChange = 10;
            this.trackBar9.Location = new System.Drawing.Point(950, 435);
            this.trackBar9.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar9.Maximum = 90;
            this.trackBar9.Name = "trackBar9";
            this.trackBar9.Size = new System.Drawing.Size(300, 45);
            this.trackBar9.TabIndex = 84;
            this.trackBar9.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox8
            // 
            this.textBox8.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox8.Location = new System.Drawing.Point(1001, 410);
            this.textBox8.Name = "textBox8";
            this.textBox8.Size = new System.Drawing.Size(27, 20);
            this.textBox8.TabIndex = 88;
            this.textBox8.Text = "000";
            // 
            // label12
            // 
            this.label12.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.label12.AutoSize = true;
            this.label12.Location = new System.Drawing.Point(660, 410);
            this.label12.Name = "label12";
            this.label12.Size = new System.Drawing.Size(35, 13);
            this.label12.TabIndex = 87;
            this.label12.Text = "Width";
            // 
            // trackBar10
            // 
            this.trackBar10.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar10.LargeChange = 10;
            this.trackBar10.Location = new System.Drawing.Point(650, 435);
            this.trackBar10.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar10.Maximum = 90;
            this.trackBar10.Name = "trackBar10";
            this.trackBar10.Size = new System.Drawing.Size(300, 45);
            this.trackBar10.TabIndex = 85;
            this.trackBar10.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox9
            // 
            this.textBox9.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox9.Location = new System.Drawing.Point(701, 410);
            this.textBox9.Name = "textBox9";
            this.textBox9.Size = new System.Drawing.Size(27, 20);
            this.textBox9.TabIndex = 86;
            this.textBox9.Text = "000";
            // 
            // label13
            // 
            this.label13.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.label13.AutoSize = true;
            this.label13.Location = new System.Drawing.Point(960, 335);
            this.label13.Name = "label13";
            this.label13.Size = new System.Drawing.Size(38, 13);
            this.label13.TabIndex = 83;
            this.label13.Text = "Height";
            // 
            // trackBar11
            // 
            this.trackBar11.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar11.LargeChange = 10;
            this.trackBar11.Location = new System.Drawing.Point(950, 360);
            this.trackBar11.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar11.Maximum = 90;
            this.trackBar11.Name = "trackBar11";
            this.trackBar11.Size = new System.Drawing.Size(300, 45);
            this.trackBar11.TabIndex = 79;
            this.trackBar11.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox10
            // 
            this.textBox10.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox10.Location = new System.Drawing.Point(1004, 335);
            this.textBox10.Name = "textBox10";
            this.textBox10.Size = new System.Drawing.Size(27, 20);
            this.textBox10.TabIndex = 82;
            this.textBox10.Text = "000";
            // 
            // label14
            // 
            this.label14.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.label14.AutoSize = true;
            this.label14.Location = new System.Drawing.Point(660, 335);
            this.label14.Name = "label14";
            this.label14.Size = new System.Drawing.Size(38, 13);
            this.label14.TabIndex = 81;
            this.label14.Text = "Height";
            // 
            // trackBar12
            // 
            this.trackBar12.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar12.LargeChange = 10;
            this.trackBar12.Location = new System.Drawing.Point(650, 360);
            this.trackBar12.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar12.Maximum = 90;
            this.trackBar12.Name = "trackBar12";
            this.trackBar12.Size = new System.Drawing.Size(300, 45);
            this.trackBar12.TabIndex = 78;
            this.trackBar12.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // label15
            // 
            this.label15.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.label15.AutoSize = true;
            this.label15.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label15.Location = new System.Drawing.Point(652, 308);
            this.label15.Name = "label15";
            this.label15.Size = new System.Drawing.Size(41, 13);
            this.label15.TabIndex = 80;
            this.label15.Text = "Pupils";
            // 
            // textBox11
            // 
            this.textBox11.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox11.Location = new System.Drawing.Point(701, 335);
            this.textBox11.Name = "textBox11";
            this.textBox11.Size = new System.Drawing.Size(27, 20);
            this.textBox11.TabIndex = 77;
            this.textBox11.Text = "000";
            // 
            // label16
            // 
            this.label16.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.label16.AutoSize = true;
            this.label16.Location = new System.Drawing.Point(960, 560);
            this.label16.Name = "label16";
            this.label16.Size = new System.Drawing.Size(14, 13);
            this.label16.TabIndex = 101;
            this.label16.Text = "Y";
            // 
            // trackBar13
            // 
            this.trackBar13.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar13.LargeChange = 10;
            this.trackBar13.Location = new System.Drawing.Point(950, 585);
            this.trackBar13.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar13.Maximum = 90;
            this.trackBar13.Name = "trackBar13";
            this.trackBar13.Size = new System.Drawing.Size(300, 45);
            this.trackBar13.TabIndex = 96;
            this.trackBar13.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox12
            // 
            this.textBox12.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox12.Location = new System.Drawing.Point(980, 560);
            this.textBox12.Name = "textBox12";
            this.textBox12.Size = new System.Drawing.Size(27, 20);
            this.textBox12.TabIndex = 100;
            this.textBox12.Text = "000";
            // 
            // label26
            // 
            this.label26.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.label26.AutoSize = true;
            this.label26.Location = new System.Drawing.Point(660, 560);
            this.label26.Name = "label26";
            this.label26.Size = new System.Drawing.Size(14, 13);
            this.label26.TabIndex = 99;
            this.label26.Text = "Y";
            // 
            // trackBar14
            // 
            this.trackBar14.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.trackBar14.LargeChange = 10;
            this.trackBar14.Location = new System.Drawing.Point(650, 585);
            this.trackBar14.Margin = new System.Windows.Forms.Padding(0);
            this.trackBar14.Maximum = 90;
            this.trackBar14.Name = "trackBar14";
            this.trackBar14.Size = new System.Drawing.Size(300, 45);
            this.trackBar14.TabIndex = 97;
            this.trackBar14.TickStyle = System.Windows.Forms.TickStyle.None;
            // 
            // textBox13
            // 
            this.textBox13.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.textBox13.Location = new System.Drawing.Point(680, 560);
            this.textBox13.Name = "textBox13";
            this.textBox13.Size = new System.Drawing.Size(27, 20);
            this.textBox13.TabIndex = 98;
            this.textBox13.Text = "000";
            // 
            // button
            // 
            this.button.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.button.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.button.Enabled = false;
            this.button.Location = new System.Drawing.Point(10, 608);
            this.button.Name = "button";
            this.button.Size = new System.Drawing.Size(100, 23);
            this.button.TabIndex = 104;
            this.button.Text = "OK";
            this.button.UseVisualStyleBackColor = true;
            // 
            // label
            // 
            this.label.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.label.AutoSize = true;
            this.label.Location = new System.Drawing.Point(12, 585);
            this.label.Name = "label";
            this.label.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.label.Size = new System.Drawing.Size(44, 13);
            this.label.TabIndex = 103;
            this.label.Text = "Time (s)";
            this.label.TextAlign = System.Drawing.ContentAlignment.BottomLeft;
            // 
            // textBox22
            // 
            this.textBox22.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.textBox22.Location = new System.Drawing.Point(62, 582);
            this.textBox22.Name = "textBox22";
            this.textBox22.Size = new System.Drawing.Size(27, 20);
            this.textBox22.TabIndex = 107;
            this.textBox22.Text = "000";
            // 
            // FaceForm
            // 
            this.ClientSize = new System.Drawing.Size(1264, 639);
            this.Controls.Add(this.textBox22);
            this.Controls.Add(this.button);
            this.Controls.Add(this.label);
            this.Controls.Add(this.label16);
            this.Controls.Add(this.trackBar13);
            this.Controls.Add(this.textBox12);
            this.Controls.Add(this.label26);
            this.Controls.Add(this.trackBar14);
            this.Controls.Add(this.textBox13);
            this.Controls.Add(this.label9);
            this.Controls.Add(this.trackBar7);
            this.Controls.Add(this.textBox6);
            this.Controls.Add(this.label10);
            this.Controls.Add(this.trackBar8);
            this.Controls.Add(this.textBox7);
            this.Controls.Add(this.label11);
            this.Controls.Add(this.trackBar9);
            this.Controls.Add(this.textBox8);
            this.Controls.Add(this.label12);
            this.Controls.Add(this.trackBar10);
            this.Controls.Add(this.textBox9);
            this.Controls.Add(this.label13);
            this.Controls.Add(this.trackBar11);
            this.Controls.Add(this.textBox10);
            this.Controls.Add(this.label14);
            this.Controls.Add(this.trackBar12);
            this.Controls.Add(this.label15);
            this.Controls.Add(this.textBox11);
            this.Controls.Add(this.label27);
            this.Controls.Add(this.trackBar22);
            this.Controls.Add(this.textBox21);
            this.Controls.Add(this.label29);
            this.Controls.Add(this.trackBar24);
            this.Controls.Add(this.label30);
            this.Controls.Add(this.textBox23);
            this.Controls.Add(this.label17);
            this.Controls.Add(this.trackBar15);
            this.Controls.Add(this.textBox14);
            this.Controls.Add(this.label18);
            this.Controls.Add(this.trackBar16);
            this.Controls.Add(this.textBox15);
            this.Controls.Add(this.label19);
            this.Controls.Add(this.trackBar17);
            this.Controls.Add(this.textBox16);
            this.Controls.Add(this.label20);
            this.Controls.Add(this.trackBar18);
            this.Controls.Add(this.textBox17);
            this.Controls.Add(this.rightLabel2);
            this.Controls.Add(this.leftLabel2);
            this.Controls.Add(this.label23);
            this.Controls.Add(this.trackBar19);
            this.Controls.Add(this.textBox18);
            this.Controls.Add(this.label24);
            this.Controls.Add(this.trackBar20);
            this.Controls.Add(this.label25);
            this.Controls.Add(this.textBox19);
            this.Controls.Add(this.label7);
            this.Controls.Add(this.trackBar5);
            this.Controls.Add(this.textBox4);
            this.Controls.Add(this.label8);
            this.Controls.Add(this.trackBar6);
            this.Controls.Add(this.textBox5);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.trackBar3);
            this.Controls.Add(this.textBox2);
            this.Controls.Add(this.label6);
            this.Controls.Add(this.trackBar4);
            this.Controls.Add(this.textBox3);
            this.Controls.Add(this.rightLabel);
            this.Controls.Add(this.leftLabel);
            this.Controls.Add(this.browRightAngleLabel);
            this.Controls.Add(this.browRightAngleTrackBar);
            this.Controls.Add(this.browRightAngleTextBox);
            this.Controls.Add(this.browLeftAngleLabel);
            this.Controls.Add(this.browLeftAngleTrackBar);
            this.Controls.Add(this.browLabel);
            this.Controls.Add(this.browLeftAngleTextBox);
            this.Name = "FaceForm";
            this.Load += new System.EventHandler(this.FaceForm_Load);
            ((System.ComponentModel.ISupportInitialize)(this.browLeftAngleTrackBar)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.browRightAngleTrackBar)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar3)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar4)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar6)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar15)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar16)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar17)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar18)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar19)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar20)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar22)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar24)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar5)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar7)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar8)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar9)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar10)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar11)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar12)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar13)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackBar14)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        private void textBox_TextChanged(object sender, EventArgs e)
        {
            try
            {
                IPAddress.Parse(this.browLeftAngleTextBox.Text);
            }
            catch (Exception) { }
        }

        private void textBox19_TextChanged(object sender, EventArgs e)
        {

        }

        private void FaceForm_Load(object sender, EventArgs e)
        {

        }
    }
}
