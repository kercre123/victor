using System.Windows.Forms;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using System;

namespace AnimationTool
{
    public class FaceForm : Form
    {

        public readonly FaceTrackBarForm[] eyes;
        private TabControl tabControl;
        private TabPage eyeTab;
        private Button button;
        private Label eyeCenterLabel;
        private FaceTrackBarForm eyeCenterY;
        private FaceTrackBarForm eyeCenterX;
        private FaceTrackBarForm eyeScaleY;
        private FaceTrackBarForm eyeScaleX;
        private Label eyeScaleLabel;
        private FaceTrackBarForm eyeAngle;
        private Label eyeAngleLabel;
        private TabPage radiusTab;
        private FaceTrackBarForm upperOuterRadiusY;
        private FaceTrackBarForm upperOuterRadiusX;
        private Label radiiUpperOuterLabel;
        private FaceTrackBarForm upperInnerRadiusY;
        private FaceTrackBarForm upperInnerRadiusX;
        private Label radiiUpperInnerLabel;
        private FaceTrackBarForm lowerOuterRadiusY;
        private FaceTrackBarForm lowerOuterRadiusX;
        private Label radiiLowerOuterLabel;
        private FaceTrackBarForm lowerInnerRadiusY;
        private FaceTrackBarForm lowerInnerRadiusX;
        private Label radiiLowerInnerLabel;
        private TabPage lidTab;
        private FaceTrackBarForm lowerLidBend;
        private FaceTrackBarForm lowerLidAngle;
        private FaceTrackBarForm lowerLidY;
        private Label lidLowerLabel;
        private FaceTrackBarForm upperLidBend;
        private FaceTrackBarForm upperLidAngle;
        private FaceTrackBarForm upperLidY;
        private Label lidUpperLabel;
        private TabPage faceTab;
        private FaceTrackBarForm faceAngleNew;
        private Label label1;
        private FaceTrackBarForm faceScale;
        private Label faceScaleLabel;
        private FaceTrackBarForm faceCenter;
        private Label faceCenterLabel;

        public float faceAngle { get { return faceAngleNew.LeftValue; } }
        public int faceCenterX { get { return (int)faceCenter.LeftValue; } }
        public int faceCenterY { get { return (int)faceCenter.RightValue; } }
        public float faceScaleX { get { return faceScale.LeftValue; } }
        public float faceScaleY { get { return faceScale.RightValue; } }

        private bool called;

        public FaceForm()
        {
            InitializeComponent();

            called = false;

            eyes = new FaceTrackBarForm[(int)ProceduralEyeParameter.NumParameters];

            eyes[(int)ProceduralEyeParameter.EyeCenterX] = eyeCenterX;
            eyes[(int)ProceduralEyeParameter.EyeCenterY] = eyeCenterY;
            eyes[(int)ProceduralEyeParameter.EyeScaleX] = eyeScaleX;
            eyes[(int)ProceduralEyeParameter.EyeScaleY] = eyeScaleY;
            eyes[(int)ProceduralEyeParameter.EyeAngle] = eyeAngle;

            eyes[(int)ProceduralEyeParameter.LowerInnerRadiusX] = lowerInnerRadiusX;
            eyes[(int)ProceduralEyeParameter.LowerInnerRadiusY] = lowerInnerRadiusY;
            eyes[(int)ProceduralEyeParameter.UpperInnerRadiusX] = upperInnerRadiusX;
            eyes[(int)ProceduralEyeParameter.UpperInnerRadiusY] = upperInnerRadiusY;

            eyes[(int)ProceduralEyeParameter.LowerOuterRadiusX] = lowerOuterRadiusX;
            eyes[(int)ProceduralEyeParameter.LowerOuterRadiusY] = lowerOuterRadiusY;
            eyes[(int)ProceduralEyeParameter.UpperOuterRadiusX] = upperOuterRadiusX;
            eyes[(int)ProceduralEyeParameter.UpperOuterRadiusY] = upperOuterRadiusY;

            eyes[(int)ProceduralEyeParameter.UpperLidY] = upperLidY;
            eyes[(int)ProceduralEyeParameter.UpperLidAngle] = upperLidAngle;
            eyes[(int)ProceduralEyeParameter.UpperLidBend] = upperLidBend;
            eyes[(int)ProceduralEyeParameter.LowerLidY] = lowerLidY;
            eyes[(int)ProceduralEyeParameter.LowerLidAngle] = lowerLidAngle;
            eyes[(int)ProceduralEyeParameter.LowerLidBend] = lowerLidBend;

            for (int i = 0; i < eyes.Length; ++i)
            {
                if(eyes[i] != null)
                {
                    eyes[i].OnChanged += OnChanged;
                }
            }

            Reset();
        }

        public void Reset()
        {
            faceAngleNew.LeftValue = 0;

            for (int i = 0; i < eyes.Length; ++i)
            {
                if (eyes[i] != null)
                {
                    eyes[i].LeftValue = 0;
                    eyes[i].RightValue = 0;
                }
            }

            OnChanged(null, null);
        }

        public DialogResult Open(Sequencer.ExtraProceduralFaceData extraData)
        {
            button.Enabled = true;

            faceAngleNew.LeftValue = extraData.faceAngle;
            faceCenter.LeftValue = extraData.faceCenterX;
            faceCenter.RightValue = extraData.faceCenterY;
            faceScale.LeftValue = extraData.faceScaleX;
            faceScale.RightValue = extraData.faceScaleY;

            for (int i = 0; i < eyes.Length && i < extraData.leftEye.Length; ++i)
            {
                if (eyes[i] != null)
                {
                    eyes[i].LeftValue = extraData.leftEye[i];
                    eyes[i].RightValue = extraData.rightEye[i];
                }
            }

            return ShowDialog();
        }

        private void OnChanged(object sender, EventArgs e)
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

            for (int i = 0; i < displayProceduralFaceMessage.leftEye.Length && i < eyes.Length; ++i)
            {
                if (eyes[i] != null)
                {
                    displayProceduralFaceMessage.leftEye[i] = eyes[i].LeftValue;
                    displayProceduralFaceMessage.rightEye[i] = eyes[i].RightValue;
                }
                else
                {
                    //MessageBox.Show("Missing " + (ProceduralEyeParameter)i + " in message.");
                    displayProceduralFaceMessage.leftEye[i] = 0;
                    displayProceduralFaceMessage.rightEye[i] = 0;
                }
            }

            MessageGameToEngine message = new MessageGameToEngine();
            message.DisplayProceduralFace = displayProceduralFaceMessage;

            if (called)
            {
                RobotEngineMessenger.instance.ProceduralFaceQueue.Send(message);
            }
            else
            {
                for (int i = 0; i < 2; ++i) // HACK: send message twice because first one fails
                {
                    RobotEngineMessenger.instance.ConnectionManager.SendMessage(message);
                    called = true;
                }
            }
        }

        private void InitializeComponent()
        {
            this.tabControl = new System.Windows.Forms.TabControl();
            this.eyeTab = new System.Windows.Forms.TabPage();
            this.eyeAngle = new AnimationTool.FaceTrackBarForm();
            this.eyeAngleLabel = new System.Windows.Forms.Label();
            this.eyeScaleY = new AnimationTool.FaceTrackBarForm();
            this.eyeScaleX = new AnimationTool.FaceTrackBarForm();
            this.eyeScaleLabel = new System.Windows.Forms.Label();
            this.eyeCenterY = new AnimationTool.FaceTrackBarForm();
            this.eyeCenterX = new AnimationTool.FaceTrackBarForm();
            this.eyeCenterLabel = new System.Windows.Forms.Label();
            this.radiusTab = new System.Windows.Forms.TabPage();
            this.upperOuterRadiusY = new AnimationTool.FaceTrackBarForm();
            this.upperOuterRadiusX = new AnimationTool.FaceTrackBarForm();
            this.radiiUpperOuterLabel = new System.Windows.Forms.Label();
            this.upperInnerRadiusY = new AnimationTool.FaceTrackBarForm();
            this.upperInnerRadiusX = new AnimationTool.FaceTrackBarForm();
            this.radiiUpperInnerLabel = new System.Windows.Forms.Label();
            this.lowerOuterRadiusY = new AnimationTool.FaceTrackBarForm();
            this.lowerOuterRadiusX = new AnimationTool.FaceTrackBarForm();
            this.radiiLowerOuterLabel = new System.Windows.Forms.Label();
            this.lowerInnerRadiusY = new AnimationTool.FaceTrackBarForm();
            this.lowerInnerRadiusX = new AnimationTool.FaceTrackBarForm();
            this.radiiLowerInnerLabel = new System.Windows.Forms.Label();
            this.lidTab = new System.Windows.Forms.TabPage();
            this.lowerLidBend = new AnimationTool.FaceTrackBarForm();
            this.lowerLidAngle = new AnimationTool.FaceTrackBarForm();
            this.lowerLidY = new AnimationTool.FaceTrackBarForm();
            this.lidLowerLabel = new System.Windows.Forms.Label();
            this.upperLidBend = new AnimationTool.FaceTrackBarForm();
            this.upperLidAngle = new AnimationTool.FaceTrackBarForm();
            this.upperLidY = new AnimationTool.FaceTrackBarForm();
            this.lidUpperLabel = new System.Windows.Forms.Label();
            this.faceTab = new System.Windows.Forms.TabPage();
            this.faceAngleNew = new AnimationTool.FaceTrackBarForm();
            this.label1 = new System.Windows.Forms.Label();
            this.faceScale = new AnimationTool.FaceTrackBarForm();
            this.faceScaleLabel = new System.Windows.Forms.Label();
            this.faceCenter = new AnimationTool.FaceTrackBarForm();
            this.faceCenterLabel = new System.Windows.Forms.Label();
            this.button = new System.Windows.Forms.Button();
            this.tabControl.SuspendLayout();
            this.eyeTab.SuspendLayout();
            this.radiusTab.SuspendLayout();
            this.lidTab.SuspendLayout();
            this.faceTab.SuspendLayout();
            this.SuspendLayout();
            // 
            // tabControl
            // 
            this.tabControl.Appearance = System.Windows.Forms.TabAppearance.FlatButtons;
            this.tabControl.Controls.Add(this.eyeTab);
            this.tabControl.Controls.Add(this.radiusTab);
            this.tabControl.Controls.Add(this.lidTab);
            this.tabControl.Controls.Add(this.faceTab);
            this.tabControl.Location = new System.Drawing.Point(12, 12);
            this.tabControl.Name = "tabControl";
            this.tabControl.SelectedIndex = 0;
            this.tabControl.Size = new System.Drawing.Size(989, 389);
            this.tabControl.TabIndex = 66;
            // 
            // eyeTab
            // 
            this.eyeTab.Controls.Add(this.eyeAngle);
            this.eyeTab.Controls.Add(this.eyeAngleLabel);
            this.eyeTab.Controls.Add(this.eyeScaleY);
            this.eyeTab.Controls.Add(this.eyeScaleX);
            this.eyeTab.Controls.Add(this.eyeScaleLabel);
            this.eyeTab.Controls.Add(this.eyeCenterY);
            this.eyeTab.Controls.Add(this.eyeCenterX);
            this.eyeTab.Controls.Add(this.eyeCenterLabel);
            this.eyeTab.Location = new System.Drawing.Point(4, 25);
            this.eyeTab.Name = "eyeTab";
            this.eyeTab.Padding = new System.Windows.Forms.Padding(3);
            this.eyeTab.Size = new System.Drawing.Size(981, 360);
            this.eyeTab.TabIndex = 1;
            this.eyeTab.Text = "Eye";
            this.eyeTab.UseVisualStyleBackColor = true;
            // 
            // eyeAngle
            // 
            this.eyeAngle.BackColor = System.Drawing.SystemColors.Control;
            this.eyeAngle.CheckBoxVisible = true;
            this.eyeAngle.FakeFloat = false;
            this.eyeAngle.LeftLabel = "Deg(real)";
            this.eyeAngle.LeftMaximum = 180;
            this.eyeAngle.LeftMinimum = -180;
            this.eyeAngle.LeftValue = 0F;
            this.eyeAngle.Location = new System.Drawing.Point(6, 289);
            this.eyeAngle.Name = "eyeAngle";
            this.eyeAngle.RightLabel = "Deg(real)";
            this.eyeAngle.RightMaximum = 180;
            this.eyeAngle.RightMinimum = -180;
            this.eyeAngle.RightValue = 0F;
            this.eyeAngle.Size = new System.Drawing.Size(480, 50);
            this.eyeAngle.TabIndex = 85;
            // 
            // eyeAngleLabel
            // 
            this.eyeAngleLabel.AutoSize = true;
            this.eyeAngleLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.eyeAngleLabel.Location = new System.Drawing.Point(2, 266);
            this.eyeAngleLabel.Name = "eyeAngleLabel";
            this.eyeAngleLabel.Size = new System.Drawing.Size(55, 20);
            this.eyeAngleLabel.TabIndex = 84;
            this.eyeAngleLabel.Text = "Angle";
            // 
            // eyeScaleY
            // 
            this.eyeScaleY.BackColor = System.Drawing.SystemColors.Control;
            this.eyeScaleY.CheckBoxVisible = true;
            this.eyeScaleY.FakeFloat = true;
            this.eyeScaleY.LeftLabel = "Y";
            this.eyeScaleY.LeftMaximum = 1000;
            this.eyeScaleY.LeftMinimum = 0;
            this.eyeScaleY.LeftValue = 1F;
            this.eyeScaleY.Location = new System.Drawing.Point(6, 213);
            this.eyeScaleY.Name = "eyeScaleY";
            this.eyeScaleY.RightLabel = "Y";
            this.eyeScaleY.RightMaximum = 1000;
            this.eyeScaleY.RightMinimum = 0;
            this.eyeScaleY.RightValue = 1F;
            this.eyeScaleY.Size = new System.Drawing.Size(480, 50);
            this.eyeScaleY.TabIndex = 83;
            // 
            // eyeScaleX
            // 
            this.eyeScaleX.BackColor = System.Drawing.SystemColors.Control;
            this.eyeScaleX.CheckBoxVisible = true;
            this.eyeScaleX.FakeFloat = true;
            this.eyeScaleX.LeftLabel = "X";
            this.eyeScaleX.LeftMaximum = 1000;
            this.eyeScaleX.LeftMinimum = 0;
            this.eyeScaleX.LeftValue = 1F;
            this.eyeScaleX.Location = new System.Drawing.Point(6, 157);
            this.eyeScaleX.Name = "eyeScaleX";
            this.eyeScaleX.RightLabel = "X";
            this.eyeScaleX.RightMaximum = 1000;
            this.eyeScaleX.RightMinimum = 0;
            this.eyeScaleX.RightValue = 1F;
            this.eyeScaleX.Size = new System.Drawing.Size(480, 50);
            this.eyeScaleX.TabIndex = 82;
            // 
            // eyeScaleLabel
            // 
            this.eyeScaleLabel.AutoSize = true;
            this.eyeScaleLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.eyeScaleLabel.Location = new System.Drawing.Point(2, 134);
            this.eyeScaleLabel.Name = "eyeScaleLabel";
            this.eyeScaleLabel.Size = new System.Drawing.Size(54, 20);
            this.eyeScaleLabel.TabIndex = 81;
            this.eyeScaleLabel.Text = "Scale";
            // 
            // eyeCenterY
            // 
            this.eyeCenterY.BackColor = System.Drawing.SystemColors.Control;
            this.eyeCenterY.CheckBoxVisible = true;
            this.eyeCenterY.FakeFloat = false;
            this.eyeCenterY.LeftLabel = "Y(real)";
            this.eyeCenterY.LeftMaximum = 64;
            this.eyeCenterY.LeftMinimum = 0;
            this.eyeCenterY.LeftValue = 32F;
            this.eyeCenterY.Location = new System.Drawing.Point(6, 81);
            this.eyeCenterY.Name = "eyeCenterY";
            this.eyeCenterY.RightLabel = "Y(real)";
            this.eyeCenterY.RightMaximum = 64;
            this.eyeCenterY.RightMinimum = 0;
            this.eyeCenterY.RightValue = 32F;
            this.eyeCenterY.Size = new System.Drawing.Size(480, 50);
            this.eyeCenterY.TabIndex = 80;
            // 
            // eyeCenterX
            // 
            this.eyeCenterX.BackColor = System.Drawing.SystemColors.Control;
            this.eyeCenterX.CheckBoxVisible = false;
            this.eyeCenterX.FakeFloat = false;
            this.eyeCenterX.LeftLabel = "X(real)";
            this.eyeCenterX.LeftMaximum = 128;
            this.eyeCenterX.LeftMinimum = 0;
            this.eyeCenterX.LeftValue = 32F;
            this.eyeCenterX.Location = new System.Drawing.Point(6, 25);
            this.eyeCenterX.Name = "eyeCenterX";
            this.eyeCenterX.RightLabel = "X(real)";
            this.eyeCenterX.RightMaximum = 128;
            this.eyeCenterX.RightMinimum = 0;
            this.eyeCenterX.RightValue = 96F;
            this.eyeCenterX.Size = new System.Drawing.Size(480, 50);
            this.eyeCenterX.TabIndex = 79;
            // 
            // eyeCenterLabel
            // 
            this.eyeCenterLabel.AutoSize = true;
            this.eyeCenterLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.eyeCenterLabel.Location = new System.Drawing.Point(2, 2);
            this.eyeCenterLabel.Name = "eyeCenterLabel";
            this.eyeCenterLabel.Size = new System.Drawing.Size(63, 20);
            this.eyeCenterLabel.TabIndex = 74;
            this.eyeCenterLabel.Text = "Center";
            // 
            // radiusTab
            // 
            this.radiusTab.Controls.Add(this.upperOuterRadiusY);
            this.radiusTab.Controls.Add(this.upperOuterRadiusX);
            this.radiusTab.Controls.Add(this.radiiUpperOuterLabel);
            this.radiusTab.Controls.Add(this.upperInnerRadiusY);
            this.radiusTab.Controls.Add(this.upperInnerRadiusX);
            this.radiusTab.Controls.Add(this.radiiUpperInnerLabel);
            this.radiusTab.Controls.Add(this.lowerOuterRadiusY);
            this.radiusTab.Controls.Add(this.lowerOuterRadiusX);
            this.radiusTab.Controls.Add(this.radiiLowerOuterLabel);
            this.radiusTab.Controls.Add(this.lowerInnerRadiusY);
            this.radiusTab.Controls.Add(this.lowerInnerRadiusX);
            this.radiusTab.Controls.Add(this.radiiLowerInnerLabel);
            this.radiusTab.Location = new System.Drawing.Point(4, 25);
            this.radiusTab.Name = "radiusTab";
            this.radiusTab.Padding = new System.Windows.Forms.Padding(3);
            this.radiusTab.Size = new System.Drawing.Size(981, 360);
            this.radiusTab.TabIndex = 2;
            this.radiusTab.Text = "Radius";
            this.radiusTab.UseVisualStyleBackColor = true;
            // 
            // upperOuterRadiusY
            // 
            this.upperOuterRadiusY.BackColor = System.Drawing.SystemColors.Control;
            this.upperOuterRadiusY.CheckBoxVisible = true;
            this.upperOuterRadiusY.FakeFloat = true;
            this.upperOuterRadiusY.LeftLabel = "Y";
            this.upperOuterRadiusY.LeftMaximum = 100;
            this.upperOuterRadiusY.LeftMinimum = 0;
            this.upperOuterRadiusY.LeftValue = 0.5F;
            this.upperOuterRadiusY.Location = new System.Drawing.Point(496, 214);
            this.upperOuterRadiusY.Name = "upperOuterRadiusY";
            this.upperOuterRadiusY.RightLabel = "Y";
            this.upperOuterRadiusY.RightMaximum = 100;
            this.upperOuterRadiusY.RightMinimum = 0;
            this.upperOuterRadiusY.RightValue = 0.5F;
            this.upperOuterRadiusY.Size = new System.Drawing.Size(480, 50);
            this.upperOuterRadiusY.TabIndex = 92;
            // 
            // upperOuterRadiusX
            // 
            this.upperOuterRadiusX.BackColor = System.Drawing.SystemColors.Control;
            this.upperOuterRadiusX.CheckBoxVisible = true;
            this.upperOuterRadiusX.FakeFloat = true;
            this.upperOuterRadiusX.LeftLabel = "X";
            this.upperOuterRadiusX.LeftMaximum = 100;
            this.upperOuterRadiusX.LeftMinimum = 0;
            this.upperOuterRadiusX.LeftValue = 0.5F;
            this.upperOuterRadiusX.Location = new System.Drawing.Point(496, 158);
            this.upperOuterRadiusX.Name = "upperOuterRadiusX";
            this.upperOuterRadiusX.RightLabel = "X";
            this.upperOuterRadiusX.RightMaximum = 100;
            this.upperOuterRadiusX.RightMinimum = 0;
            this.upperOuterRadiusX.RightValue = 0.5F;
            this.upperOuterRadiusX.Size = new System.Drawing.Size(480, 50);
            this.upperOuterRadiusX.TabIndex = 91;
            // 
            // radiiUpperOuterLabel
            // 
            this.radiiUpperOuterLabel.AutoSize = true;
            this.radiiUpperOuterLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.radiiUpperOuterLabel.Location = new System.Drawing.Point(492, 135);
            this.radiiUpperOuterLabel.Name = "radiiUpperOuterLabel";
            this.radiiUpperOuterLabel.Size = new System.Drawing.Size(108, 20);
            this.radiiUpperOuterLabel.TabIndex = 90;
            this.radiiUpperOuterLabel.Text = "Upper Outer";
            // 
            // upperInnerRadiusY
            // 
            this.upperInnerRadiusY.BackColor = System.Drawing.SystemColors.Control;
            this.upperInnerRadiusY.CheckBoxVisible = true;
            this.upperInnerRadiusY.FakeFloat = true;
            this.upperInnerRadiusY.LeftLabel = "Y";
            this.upperInnerRadiusY.LeftMaximum = 100;
            this.upperInnerRadiusY.LeftMinimum = 0;
            this.upperInnerRadiusY.LeftValue = 0.5F;
            this.upperInnerRadiusY.Location = new System.Drawing.Point(496, 82);
            this.upperInnerRadiusY.Name = "upperInnerRadiusY";
            this.upperInnerRadiusY.RightLabel = "Y";
            this.upperInnerRadiusY.RightMaximum = 100;
            this.upperInnerRadiusY.RightMinimum = 0;
            this.upperInnerRadiusY.RightValue = 0.5F;
            this.upperInnerRadiusY.Size = new System.Drawing.Size(480, 50);
            this.upperInnerRadiusY.TabIndex = 89;
            // 
            // upperInnerRadiusX
            // 
            this.upperInnerRadiusX.BackColor = System.Drawing.SystemColors.Control;
            this.upperInnerRadiusX.CheckBoxVisible = true;
            this.upperInnerRadiusX.FakeFloat = true;
            this.upperInnerRadiusX.LeftLabel = "X";
            this.upperInnerRadiusX.LeftMaximum = 100;
            this.upperInnerRadiusX.LeftMinimum = 0;
            this.upperInnerRadiusX.LeftValue = 0.5F;
            this.upperInnerRadiusX.Location = new System.Drawing.Point(496, 26);
            this.upperInnerRadiusX.Name = "upperInnerRadiusX";
            this.upperInnerRadiusX.RightLabel = "X";
            this.upperInnerRadiusX.RightMaximum = 100;
            this.upperInnerRadiusX.RightMinimum = 0;
            this.upperInnerRadiusX.RightValue = 0.5F;
            this.upperInnerRadiusX.Size = new System.Drawing.Size(480, 50);
            this.upperInnerRadiusX.TabIndex = 88;
            // 
            // radiiUpperInnerLabel
            // 
            this.radiiUpperInnerLabel.AutoSize = true;
            this.radiiUpperInnerLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.radiiUpperInnerLabel.Location = new System.Drawing.Point(492, 3);
            this.radiiUpperInnerLabel.Name = "radiiUpperInnerLabel";
            this.radiiUpperInnerLabel.Size = new System.Drawing.Size(105, 20);
            this.radiiUpperInnerLabel.TabIndex = 87;
            this.radiiUpperInnerLabel.Text = "Upper Inner";
            // 
            // lowerOuterRadiusY
            // 
            this.lowerOuterRadiusY.BackColor = System.Drawing.SystemColors.Control;
            this.lowerOuterRadiusY.CheckBoxVisible = true;
            this.lowerOuterRadiusY.FakeFloat = true;
            this.lowerOuterRadiusY.LeftLabel = "Y";
            this.lowerOuterRadiusY.LeftMaximum = 100;
            this.lowerOuterRadiusY.LeftMinimum = 0;
            this.lowerOuterRadiusY.LeftValue = 0.5F;
            this.lowerOuterRadiusY.Location = new System.Drawing.Point(6, 213);
            this.lowerOuterRadiusY.Name = "lowerOuterRadiusY";
            this.lowerOuterRadiusY.RightLabel = "Y";
            this.lowerOuterRadiusY.RightMaximum = 100;
            this.lowerOuterRadiusY.RightMinimum = 0;
            this.lowerOuterRadiusY.RightValue = 0.5F;
            this.lowerOuterRadiusY.Size = new System.Drawing.Size(480, 50);
            this.lowerOuterRadiusY.TabIndex = 86;
            // 
            // lowerOuterRadiusX
            // 
            this.lowerOuterRadiusX.BackColor = System.Drawing.SystemColors.Control;
            this.lowerOuterRadiusX.CheckBoxVisible = true;
            this.lowerOuterRadiusX.FakeFloat = true;
            this.lowerOuterRadiusX.LeftLabel = "X";
            this.lowerOuterRadiusX.LeftMaximum = 100;
            this.lowerOuterRadiusX.LeftMinimum = 0;
            this.lowerOuterRadiusX.LeftValue = 0.5F;
            this.lowerOuterRadiusX.Location = new System.Drawing.Point(6, 157);
            this.lowerOuterRadiusX.Name = "lowerOuterRadiusX";
            this.lowerOuterRadiusX.RightLabel = "X";
            this.lowerOuterRadiusX.RightMaximum = 100;
            this.lowerOuterRadiusX.RightMinimum = 0;
            this.lowerOuterRadiusX.RightValue = 0.5F;
            this.lowerOuterRadiusX.Size = new System.Drawing.Size(480, 50);
            this.lowerOuterRadiusX.TabIndex = 85;
            // 
            // radiiLowerOuterLabel
            // 
            this.radiiLowerOuterLabel.AutoSize = true;
            this.radiiLowerOuterLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.radiiLowerOuterLabel.Location = new System.Drawing.Point(2, 134);
            this.radiiLowerOuterLabel.Name = "radiiLowerOuterLabel";
            this.radiiLowerOuterLabel.Size = new System.Drawing.Size(107, 20);
            this.radiiLowerOuterLabel.TabIndex = 84;
            this.radiiLowerOuterLabel.Text = "Lower Outer";
            // 
            // lowerInnerRadiusY
            // 
            this.lowerInnerRadiusY.BackColor = System.Drawing.SystemColors.Control;
            this.lowerInnerRadiusY.CheckBoxVisible = true;
            this.lowerInnerRadiusY.FakeFloat = true;
            this.lowerInnerRadiusY.LeftLabel = "Y";
            this.lowerInnerRadiusY.LeftMaximum = 100;
            this.lowerInnerRadiusY.LeftMinimum = 0;
            this.lowerInnerRadiusY.LeftValue = 0.5F;
            this.lowerInnerRadiusY.Location = new System.Drawing.Point(6, 81);
            this.lowerInnerRadiusY.Name = "lowerInnerRadiusY";
            this.lowerInnerRadiusY.RightLabel = "Y";
            this.lowerInnerRadiusY.RightMaximum = 100;
            this.lowerInnerRadiusY.RightMinimum = 0;
            this.lowerInnerRadiusY.RightValue = 0.5F;
            this.lowerInnerRadiusY.Size = new System.Drawing.Size(480, 50);
            this.lowerInnerRadiusY.TabIndex = 83;
            // 
            // lowerInnerRadiusX
            // 
            this.lowerInnerRadiusX.BackColor = System.Drawing.SystemColors.Control;
            this.lowerInnerRadiusX.CheckBoxVisible = true;
            this.lowerInnerRadiusX.FakeFloat = true;
            this.lowerInnerRadiusX.LeftLabel = "X";
            this.lowerInnerRadiusX.LeftMaximum = 100;
            this.lowerInnerRadiusX.LeftMinimum = 0;
            this.lowerInnerRadiusX.LeftValue = 0.5F;
            this.lowerInnerRadiusX.Location = new System.Drawing.Point(6, 25);
            this.lowerInnerRadiusX.Name = "lowerInnerRadiusX";
            this.lowerInnerRadiusX.RightLabel = "X";
            this.lowerInnerRadiusX.RightMaximum = 100;
            this.lowerInnerRadiusX.RightMinimum = 0;
            this.lowerInnerRadiusX.RightValue = 0.5F;
            this.lowerInnerRadiusX.Size = new System.Drawing.Size(480, 50);
            this.lowerInnerRadiusX.TabIndex = 82;
            // 
            // radiiLowerInnerLabel
            // 
            this.radiiLowerInnerLabel.AutoSize = true;
            this.radiiLowerInnerLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.radiiLowerInnerLabel.Location = new System.Drawing.Point(2, 2);
            this.radiiLowerInnerLabel.Name = "radiiLowerInnerLabel";
            this.radiiLowerInnerLabel.Size = new System.Drawing.Size(104, 20);
            this.radiiLowerInnerLabel.TabIndex = 81;
            this.radiiLowerInnerLabel.Text = "Lower Inner";
            // 
            // lidTab
            // 
            this.lidTab.Controls.Add(this.lowerLidBend);
            this.lidTab.Controls.Add(this.lowerLidAngle);
            this.lidTab.Controls.Add(this.lowerLidY);
            this.lidTab.Controls.Add(this.lidLowerLabel);
            this.lidTab.Controls.Add(this.upperLidBend);
            this.lidTab.Controls.Add(this.upperLidAngle);
            this.lidTab.Controls.Add(this.upperLidY);
            this.lidTab.Controls.Add(this.lidUpperLabel);
            this.lidTab.Location = new System.Drawing.Point(4, 25);
            this.lidTab.Name = "lidTab";
            this.lidTab.Padding = new System.Windows.Forms.Padding(3);
            this.lidTab.Size = new System.Drawing.Size(981, 360);
            this.lidTab.TabIndex = 3;
            this.lidTab.Text = "Lid";
            this.lidTab.UseVisualStyleBackColor = true;
            // 
            // lowerLidBend
            // 
            this.lowerLidBend.BackColor = System.Drawing.SystemColors.Control;
            this.lowerLidBend.CheckBoxVisible = true;
            this.lowerLidBend.FakeFloat = true;
            this.lowerLidBend.LeftLabel = "Bend";
            this.lowerLidBend.LeftMaximum = 100;
            this.lowerLidBend.LeftMinimum = 0;
            this.lowerLidBend.LeftValue = 0F;
            this.lowerLidBend.Location = new System.Drawing.Point(6, 137);
            this.lowerLidBend.Name = "lowerLidBend";
            this.lowerLidBend.RightLabel = "Bend";
            this.lowerLidBend.RightMaximum = 100;
            this.lowerLidBend.RightMinimum = 0;
            this.lowerLidBend.RightValue = 0F;
            this.lowerLidBend.Size = new System.Drawing.Size(480, 50);
            this.lowerLidBend.TabIndex = 100;
            // 
            // lowerLidAngle
            // 
            this.lowerLidAngle.BackColor = System.Drawing.SystemColors.Control;
            this.lowerLidAngle.CheckBoxVisible = true;
            this.lowerLidAngle.FakeFloat = false;
            this.lowerLidAngle.LeftLabel = "Angle(real)";
            this.lowerLidAngle.LeftMaximum = 180;
            this.lowerLidAngle.LeftMinimum = -180;
            this.lowerLidAngle.LeftValue = 0F;
            this.lowerLidAngle.Location = new System.Drawing.Point(6, 81);
            this.lowerLidAngle.Name = "lowerLidAngle";
            this.lowerLidAngle.RightLabel = "Angle(real)";
            this.lowerLidAngle.RightMaximum = 180;
            this.lowerLidAngle.RightMinimum = -180;
            this.lowerLidAngle.RightValue = 0F;
            this.lowerLidAngle.Size = new System.Drawing.Size(480, 50);
            this.lowerLidAngle.TabIndex = 99;
            // 
            // lowerLidY
            // 
            this.lowerLidY.BackColor = System.Drawing.SystemColors.Control;
            this.lowerLidY.CheckBoxVisible = true;
            this.lowerLidY.FakeFloat = true;
            this.lowerLidY.LeftLabel = "Y";
            this.lowerLidY.LeftMaximum = 100;
            this.lowerLidY.LeftMinimum = 0;
            this.lowerLidY.LeftValue = 0F;
            this.lowerLidY.Location = new System.Drawing.Point(6, 25);
            this.lowerLidY.Name = "lowerLidY";
            this.lowerLidY.RightLabel = "Y";
            this.lowerLidY.RightMaximum = 100;
            this.lowerLidY.RightMinimum = 0;
            this.lowerLidY.RightValue = 0F;
            this.lowerLidY.Size = new System.Drawing.Size(480, 50);
            this.lowerLidY.TabIndex = 98;
            // 
            // lidLowerLabel
            // 
            this.lidLowerLabel.AutoSize = true;
            this.lidLowerLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lidLowerLabel.Location = new System.Drawing.Point(2, 2);
            this.lidLowerLabel.Name = "lidLowerLabel";
            this.lidLowerLabel.Size = new System.Drawing.Size(57, 20);
            this.lidLowerLabel.TabIndex = 97;
            this.lidLowerLabel.Text = "Lower";
            // 
            // upperLidBend
            // 
            this.upperLidBend.BackColor = System.Drawing.SystemColors.Control;
            this.upperLidBend.CheckBoxVisible = true;
            this.upperLidBend.FakeFloat = true;
            this.upperLidBend.LeftLabel = "Bend";
            this.upperLidBend.LeftMaximum = 100;
            this.upperLidBend.LeftMinimum = 0;
            this.upperLidBend.LeftValue = 0F;
            this.upperLidBend.Location = new System.Drawing.Point(496, 138);
            this.upperLidBend.Name = "upperLidBend";
            this.upperLidBend.RightLabel = "Bend";
            this.upperLidBend.RightMaximum = 100;
            this.upperLidBend.RightMinimum = 0;
            this.upperLidBend.RightValue = 0F;
            this.upperLidBend.Size = new System.Drawing.Size(480, 50);
            this.upperLidBend.TabIndex = 96;
            // 
            // upperLidAngle
            // 
            this.upperLidAngle.BackColor = System.Drawing.SystemColors.Control;
            this.upperLidAngle.CheckBoxVisible = true;
            this.upperLidAngle.FakeFloat = false;
            this.upperLidAngle.LeftLabel = "Angle(real)";
            this.upperLidAngle.LeftMaximum = 180;
            this.upperLidAngle.LeftMinimum = -180;
            this.upperLidAngle.LeftValue = 0F;
            this.upperLidAngle.Location = new System.Drawing.Point(496, 82);
            this.upperLidAngle.Name = "upperLidAngle";
            this.upperLidAngle.RightLabel = "Angle(real)";
            this.upperLidAngle.RightMaximum = 180;
            this.upperLidAngle.RightMinimum = -180;
            this.upperLidAngle.RightValue = 0F;
            this.upperLidAngle.Size = new System.Drawing.Size(480, 50);
            this.upperLidAngle.TabIndex = 95;
            // 
            // upperLidY
            // 
            this.upperLidY.BackColor = System.Drawing.SystemColors.Control;
            this.upperLidY.CheckBoxVisible = true;
            this.upperLidY.FakeFloat = true;
            this.upperLidY.LeftLabel = "Y";
            this.upperLidY.LeftMaximum = 100;
            this.upperLidY.LeftMinimum = 0;
            this.upperLidY.LeftValue = 0F;
            this.upperLidY.Location = new System.Drawing.Point(496, 26);
            this.upperLidY.Name = "upperLidY";
            this.upperLidY.RightLabel = "Y";
            this.upperLidY.RightMaximum = 100;
            this.upperLidY.RightMinimum = 0;
            this.upperLidY.RightValue = 0F;
            this.upperLidY.Size = new System.Drawing.Size(480, 50);
            this.upperLidY.TabIndex = 94;
            // 
            // lidUpperLabel
            // 
            this.lidUpperLabel.AutoSize = true;
            this.lidUpperLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lidUpperLabel.Location = new System.Drawing.Point(492, 3);
            this.lidUpperLabel.Name = "lidUpperLabel";
            this.lidUpperLabel.Size = new System.Drawing.Size(58, 20);
            this.lidUpperLabel.TabIndex = 93;
            this.lidUpperLabel.Text = "Upper";
            // 
            // faceTab
            // 
            this.faceTab.Controls.Add(this.faceAngleNew);
            this.faceTab.Controls.Add(this.label1);
            this.faceTab.Controls.Add(this.faceScale);
            this.faceTab.Controls.Add(this.faceScaleLabel);
            this.faceTab.Controls.Add(this.faceCenter);
            this.faceTab.Controls.Add(this.faceCenterLabel);
            this.faceTab.Location = new System.Drawing.Point(4, 25);
            this.faceTab.Name = "faceTab";
            this.faceTab.Padding = new System.Windows.Forms.Padding(3);
            this.faceTab.Size = new System.Drawing.Size(981, 360);
            this.faceTab.TabIndex = 4;
            this.faceTab.Text = "Face";
            this.faceTab.UseVisualStyleBackColor = true;
            // 
            // faceAngleNew
            // 
            this.faceAngleNew.BackColor = System.Drawing.SystemColors.Control;
            this.faceAngleNew.CheckBoxVisible = false;
            this.faceAngleNew.FakeFloat = false;
            this.faceAngleNew.LeftLabel = "Deg(real)";
            this.faceAngleNew.LeftMaximum = 180;
            this.faceAngleNew.LeftMinimum = -180;
            this.faceAngleNew.LeftValue = 0F;
            this.faceAngleNew.Location = new System.Drawing.Point(6, 177);
            this.faceAngleNew.Name = "faceAngleNew";
            this.faceAngleNew.RightLabel = "Unused";
            this.faceAngleNew.RightMaximum = 0;
            this.faceAngleNew.RightMinimum = 0;
            this.faceAngleNew.RightValue = 0F;
            this.faceAngleNew.Size = new System.Drawing.Size(480, 50);
            this.faceAngleNew.TabIndex = 88;
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(2, 154);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(55, 20);
            this.label1.TabIndex = 87;
            this.label1.Text = "Angle";
            // 
            // faceScale
            // 
            this.faceScale.BackColor = System.Drawing.SystemColors.Control;
            this.faceScale.CheckBoxVisible = true;
            this.faceScale.FakeFloat = true;
            this.faceScale.LeftLabel = "X";
            this.faceScale.LeftMaximum = 1000;
            this.faceScale.LeftMinimum = 0;
            this.faceScale.LeftValue = 1F;
            this.faceScale.Location = new System.Drawing.Point(6, 101);
            this.faceScale.Name = "faceScale";
            this.faceScale.RightLabel = "Y";
            this.faceScale.RightMaximum = 1000;
            this.faceScale.RightMinimum = 0;
            this.faceScale.RightValue = 1F;
            this.faceScale.Size = new System.Drawing.Size(480, 50);
            this.faceScale.TabIndex = 86;
            // 
            // faceScaleLabel
            // 
            this.faceScaleLabel.AutoSize = true;
            this.faceScaleLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.faceScaleLabel.Location = new System.Drawing.Point(2, 78);
            this.faceScaleLabel.Name = "faceScaleLabel";
            this.faceScaleLabel.Size = new System.Drawing.Size(54, 20);
            this.faceScaleLabel.TabIndex = 85;
            this.faceScaleLabel.Text = "Scale";
            // 
            // faceCenter
            // 
            this.faceCenter.BackColor = System.Drawing.SystemColors.Control;
            this.faceCenter.CheckBoxVisible = false;
            this.faceCenter.FakeFloat = false;
            this.faceCenter.LeftLabel = "X(real)";
            this.faceCenter.LeftMaximum = 64;
            this.faceCenter.LeftMinimum = -64;
            this.faceCenter.LeftValue = 0F;
            this.faceCenter.Location = new System.Drawing.Point(6, 25);
            this.faceCenter.Name = "faceCenter";
            this.faceCenter.RightLabel = "Y(real)";
            this.faceCenter.RightMaximum = 32;
            this.faceCenter.RightMinimum = -32;
            this.faceCenter.RightValue = 0F;
            this.faceCenter.Size = new System.Drawing.Size(480, 50);
            this.faceCenter.TabIndex = 84;
            // 
            // faceCenterLabel
            // 
            this.faceCenterLabel.AutoSize = true;
            this.faceCenterLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.faceCenterLabel.Location = new System.Drawing.Point(2, 2);
            this.faceCenterLabel.Name = "faceCenterLabel";
            this.faceCenterLabel.Size = new System.Drawing.Size(63, 20);
            this.faceCenterLabel.TabIndex = 83;
            this.faceCenterLabel.Text = "Center";
            // 
            // button
            // 
            this.button.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.button.Enabled = false;
            this.button.Location = new System.Drawing.Point(402, 407);
            this.button.Name = "button";
            this.button.Size = new System.Drawing.Size(100, 23);
            this.button.TabIndex = 32;
            this.button.Text = "OK";
            this.button.UseVisualStyleBackColor = true;
            // 
            // FaceForm
            // 
            this.AutoScroll = true;
            this.ClientSize = new System.Drawing.Size(1008, 445);
            this.Controls.Add(this.tabControl);
            this.Controls.Add(this.button);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.SizableToolWindow;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "FaceForm";
            this.Text = "Face Animation";
            this.tabControl.ResumeLayout(false);
            this.eyeTab.ResumeLayout(false);
            this.eyeTab.PerformLayout();
            this.radiusTab.ResumeLayout(false);
            this.radiusTab.PerformLayout();
            this.lidTab.ResumeLayout(false);
            this.lidTab.PerformLayout();
            this.faceTab.ResumeLayout(false);
            this.faceTab.PerformLayout();
            this.ResumeLayout(false);

        }
    }
}
