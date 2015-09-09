using System.Windows.Forms;
using Anki.Cozmo;

namespace AnimationTool
{
    public class FaceForm : Form
    {
        private Label browsLabel;
        private FaceTrackBarForm browAngle;
        private FaceTrackBarForm browX;
        private Label faceLabel;
        private FaceTrackBarForm face;
        private Button button;
        private FaceTrackBarForm browLength;
        private Label pupilLabel;
        private Label eyesLabel;
        private FaceTrackBarForm eyeHeight;
        private FaceTrackBarForm eyeWidth;
        private FaceTrackBarForm eyeY;
        private FaceTrackBarForm pupilX;
        private FaceTrackBarForm pupilY;
        private FaceTrackBarForm pupilSize;
        private FaceTrackBarForm browY;

        public FaceTrackBarForm[] Eyes { get; private set; }

        public float FaceAngle_deg { get { return face.RightValue; } }
        public float FaceY { get { return face.LeftValue; } }

        public bool Changed
        {
            get
            {
                for(int i = 0; i < Eyes.Length; ++i)
                {
                    if (Eyes[i].Changed)
                    {
                        return true;
                    }
                }

                return face.Changed;
            }

            set
            {
                face.Changed = value;

                for(int i = 0; i < Eyes.Length; ++i)
                {
                    Eyes[i].Changed = value;
                }
            }
        }

        public float LastFaceAngle { get; private set; }
        public float[] LastLeftEye { get; private set; }
        public float[] LastRightEye { get; private set; }

        public FaceForm()
        {
            InitializeComponent();

            Eyes = new FaceTrackBarForm[(int)ProceduralEyeParameter.NumParameters];
            LastLeftEye = new float[(int)ProceduralEyeParameter.NumParameters];
            LastRightEye = new float[(int)ProceduralEyeParameter.NumParameters];

            Eyes[(int)ProceduralEyeParameter.BrowAngle] = browAngle;
            Eyes[(int)ProceduralEyeParameter.BrowCenX] = browX;
            Eyes[(int)ProceduralEyeParameter.BrowCenY] = browY;

            Eyes[(int)ProceduralEyeParameter.EyeHeight] = eyeHeight;
            Eyes[(int)ProceduralEyeParameter.EyeWidth] = eyeWidth;

            Eyes[(int)ProceduralEyeParameter.PupilCenY] = pupilY;
            Eyes[(int)ProceduralEyeParameter.PupilCenX] = pupilX;
            Eyes[(int)ProceduralEyeParameter.PupilHeight] = pupilSize;
            Eyes[(int)ProceduralEyeParameter.PupilWidth] = pupilSize;
        }

        public DialogResult Open(Sequencer.ExtraProceduralFaceData extraData)
        {
            button.Enabled = true;
            Changed = true;

            face.RightValue = extraData.faceAngle_deg;

            browAngle.LeftValue = extraData.leftBrowAngle;
            browAngle.RightValue = extraData.rightBrowAngle;
            browY.LeftValue = extraData.leftBrowCenY;
            browY.RightValue = extraData.rightBrowCenY;
            browX.LeftValue = extraData.leftBrowCenX;
            browX.RightValue = extraData.rightBrowCenX;

            eyeHeight.LeftValue = extraData.leftEyeHeight;
            eyeHeight.RightValue = extraData.rightEyeHeight;
            eyeWidth.LeftValue = extraData.leftEyeWidth;
            eyeWidth.RightValue = extraData.rightEyeWidth;

            pupilY.LeftValue = extraData.leftPupilCenY;
            pupilY.RightValue = extraData.rightPupilCenY;
            pupilX.LeftValue = extraData.leftPupilCenX;
            pupilX.RightValue = extraData.rightPupilCenX;
            pupilSize.LeftValue = extraData.leftPupilHeight;
            pupilSize.RightValue = extraData.rightPupilHeight;

            for (int i = 0; i < Eyes.Length; ++i)
            {
                if (Eyes[i] != null)
                {
                    LastLeftEye[i] = Eyes[i].LeftValue;
                    LastRightEye[i] = Eyes[i].RightValue;
                }
            }
            LastFaceAngle = FaceAngle_deg;

            return ShowDialog();
        }

        private void InitializeComponent()
        {
            this.browsLabel = new System.Windows.Forms.Label();
            this.faceLabel = new System.Windows.Forms.Label();
            this.button = new System.Windows.Forms.Button();
            this.browAngle = new AnimationTool.FaceTrackBarForm();
            this.browX = new AnimationTool.FaceTrackBarForm();
            this.browY = new AnimationTool.FaceTrackBarForm();
            this.face = new AnimationTool.FaceTrackBarForm();
            this.browLength = new AnimationTool.FaceTrackBarForm();
            this.pupilLabel = new System.Windows.Forms.Label();
            this.eyesLabel = new System.Windows.Forms.Label();
            this.eyeHeight = new AnimationTool.FaceTrackBarForm();
            this.eyeWidth = new AnimationTool.FaceTrackBarForm();
            this.eyeY = new AnimationTool.FaceTrackBarForm();
            this.pupilX = new AnimationTool.FaceTrackBarForm();
            this.pupilY = new AnimationTool.FaceTrackBarForm();
            this.pupilSize = new AnimationTool.FaceTrackBarForm();
            this.SuspendLayout();
            // 
            // browsLabel
            // 
            this.browsLabel.AutoSize = true;
            this.browsLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.browsLabel.Location = new System.Drawing.Point(510, -1);
            this.browsLabel.Name = "browsLabel";
            this.browsLabel.Size = new System.Drawing.Size(58, 20);
            this.browsLabel.TabIndex = 1;
            this.browsLabel.Text = "Brows";
            // 
            // faceLabel
            // 
            this.faceLabel.AutoSize = true;
            this.faceLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.faceLabel.Location = new System.Drawing.Point(510, 309);
            this.faceLabel.Name = "faceLabel";
            this.faceLabel.Size = new System.Drawing.Size(49, 20);
            this.faceLabel.TabIndex = 18;
            this.faceLabel.Text = "Face";
            // 
            // button
            // 
            this.button.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.button.Enabled = false;
            this.button.Location = new System.Drawing.Point(899, 438);
            this.button.Name = "button";
            this.button.Size = new System.Drawing.Size(100, 23);
            this.button.TabIndex = 32;
            this.button.Text = "OK";
            this.button.UseVisualStyleBackColor = true;
            // 
            // browAngle
            // 
            this.browAngle.BackColor = System.Drawing.SystemColors.Control;
            this.browAngle.CheckBoxVisible = true;
            this.browAngle.LeftLabel = "Angle (deg)";
            this.browAngle.LeftMaximum = 100;
            this.browAngle.LeftMinimum = -100;
            this.browAngle.Location = new System.Drawing.Point(514, 24);
            this.browAngle.Name = "browAngle";
            this.browAngle.RightLabel = "Angle (deg)";
            this.browAngle.RightMaximum = 100;
            this.browAngle.RightMinimum = -100;
            this.browAngle.Size = new System.Drawing.Size(480, 50);
            this.browAngle.TabIndex = 55;
            // 
            // browX
            // 
            this.browX.BackColor = System.Drawing.SystemColors.Control;
            this.browX.CheckBoxVisible = true;
            this.browX.LeftLabel = "X";
            this.browX.LeftMaximum = 100;
            this.browX.LeftMinimum = -100;
            this.browX.Location = new System.Drawing.Point(514, 96);
            this.browX.Name = "browX";
            this.browX.RightLabel = "X";
            this.browX.RightMaximum = 100;
            this.browX.RightMinimum = -100;
            this.browX.Size = new System.Drawing.Size(480, 50);
            this.browX.TabIndex = 56;
            // 
            // browY
            // 
            this.browY.BackColor = System.Drawing.SystemColors.Control;
            this.browY.CheckBoxVisible = true;
            this.browY.LeftLabel = "Y";
            this.browY.LeftMaximum = 100;
            this.browY.LeftMinimum = -100;
            this.browY.Location = new System.Drawing.Point(514, 168);
            this.browY.Name = "browY";
            this.browY.RightLabel = "Y";
            this.browY.RightMaximum = 100;
            this.browY.RightMinimum = -100;
            this.browY.Size = new System.Drawing.Size(480, 50);
            this.browY.TabIndex = 57;
            // 
            // face
            // 
            this.face.BackColor = System.Drawing.SystemColors.Control;
            this.face.CheckBoxVisible = false;
            this.face.LeftLabel = "Y";
            this.face.LeftMaximum = 100;
            this.face.LeftMinimum = -100;
            this.face.Location = new System.Drawing.Point(514, 335);
            this.face.Name = "face";
            this.face.RightLabel = "Angle (deg)";
            this.face.RightMaximum = 100;
            this.face.RightMinimum = -100;
            this.face.Size = new System.Drawing.Size(480, 50);
            this.face.TabIndex = 58;
            // 
            // browLength
            // 
            this.browLength.BackColor = System.Drawing.SystemColors.Control;
            this.browLength.CheckBoxVisible = true;
            this.browLength.LeftLabel = "Length";
            this.browLength.LeftMaximum = 100;
            this.browLength.LeftMinimum = -100;
            this.browLength.Location = new System.Drawing.Point(514, 240);
            this.browLength.Name = "browLength";
            this.browLength.RightLabel = "Length";
            this.browLength.RightMaximum = 100;
            this.browLength.RightMinimum = -100;
            this.browLength.Size = new System.Drawing.Size(480, 50);
            this.browLength.TabIndex = 59;
            // 
            // pupilLabel
            // 
            this.pupilLabel.AutoSize = true;
            this.pupilLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.pupilLabel.Location = new System.Drawing.Point(-4, 240);
            this.pupilLabel.Name = "pupilLabel";
            this.pupilLabel.Size = new System.Drawing.Size(57, 20);
            this.pupilLabel.TabIndex = 54;
            this.pupilLabel.Text = "Pupils";
            // 
            // eyesLabel
            // 
            this.eyesLabel.AutoSize = true;
            this.eyesLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.eyesLabel.Location = new System.Drawing.Point(-4, -1);
            this.eyesLabel.Name = "eyesLabel";
            this.eyesLabel.Size = new System.Drawing.Size(48, 20);
            this.eyesLabel.TabIndex = 53;
            this.eyesLabel.Text = "Eyes";
            // 
            // eyeHeight
            // 
            this.eyeHeight.BackColor = System.Drawing.SystemColors.Control;
            this.eyeHeight.CheckBoxVisible = true;
            this.eyeHeight.LeftLabel = "Height";
            this.eyeHeight.LeftMaximum = 100;
            this.eyeHeight.LeftMinimum = -100;
            this.eyeHeight.Location = new System.Drawing.Point(0, 24);
            this.eyeHeight.Name = "eyeHeight";
            this.eyeHeight.RightLabel = "Height";
            this.eyeHeight.RightMaximum = 100;
            this.eyeHeight.RightMinimum = -100;
            this.eyeHeight.Size = new System.Drawing.Size(480, 50);
            this.eyeHeight.TabIndex = 60;
            // 
            // eyeWidth
            // 
            this.eyeWidth.BackColor = System.Drawing.SystemColors.Control;
            this.eyeWidth.CheckBoxVisible = true;
            this.eyeWidth.LeftLabel = "Width";
            this.eyeWidth.LeftMaximum = 100;
            this.eyeWidth.LeftMinimum = -100;
            this.eyeWidth.Location = new System.Drawing.Point(0, 96);
            this.eyeWidth.Name = "eyeWidth";
            this.eyeWidth.RightLabel = "Width";
            this.eyeWidth.RightMaximum = 100;
            this.eyeWidth.RightMinimum = -100;
            this.eyeWidth.Size = new System.Drawing.Size(480, 50);
            this.eyeWidth.TabIndex = 61;
            // 
            // eyeY
            // 
            this.eyeY.BackColor = System.Drawing.SystemColors.Control;
            this.eyeY.CheckBoxVisible = true;
            this.eyeY.LeftLabel = "Y";
            this.eyeY.LeftMaximum = 100;
            this.eyeY.LeftMinimum = -100;
            this.eyeY.Location = new System.Drawing.Point(0, 168);
            this.eyeY.Name = "eyeY";
            this.eyeY.RightLabel = "Y";
            this.eyeY.RightMaximum = 100;
            this.eyeY.RightMinimum = -100;
            this.eyeY.Size = new System.Drawing.Size(480, 50);
            this.eyeY.TabIndex = 62;
            // 
            // pupilX
            // 
            this.pupilX.BackColor = System.Drawing.SystemColors.Control;
            this.pupilX.CheckBoxVisible = true;
            this.pupilX.LeftLabel = "X";
            this.pupilX.LeftMaximum = 100;
            this.pupilX.LeftMinimum = -100;
            this.pupilX.Location = new System.Drawing.Point(0, 263);
            this.pupilX.Name = "pupilX";
            this.pupilX.RightLabel = "X";
            this.pupilX.RightMaximum = 100;
            this.pupilX.RightMinimum = -100;
            this.pupilX.Size = new System.Drawing.Size(480, 50);
            this.pupilX.TabIndex = 63;
            // 
            // pupilY
            // 
            this.pupilY.BackColor = System.Drawing.SystemColors.Control;
            this.pupilY.CheckBoxVisible = true;
            this.pupilY.LeftLabel = "Y";
            this.pupilY.LeftMaximum = 100;
            this.pupilY.LeftMinimum = -100;
            this.pupilY.Location = new System.Drawing.Point(0, 335);
            this.pupilY.Name = "pupilY";
            this.pupilY.RightLabel = "Y";
            this.pupilY.RightMaximum = 100;
            this.pupilY.RightMinimum = -100;
            this.pupilY.Size = new System.Drawing.Size(480, 50);
            this.pupilY.TabIndex = 64;
            // 
            // pupilSize
            // 
            this.pupilSize.BackColor = System.Drawing.SystemColors.Control;
            this.pupilSize.CheckBoxVisible = true;
            this.pupilSize.LeftLabel = "Size";
            this.pupilSize.LeftMaximum = 100;
            this.pupilSize.LeftMinimum = -100;
            this.pupilSize.Location = new System.Drawing.Point(0, 407);
            this.pupilSize.Name = "pupilSize";
            this.pupilSize.RightLabel = "Size";
            this.pupilSize.RightMaximum = 100;
            this.pupilSize.RightMinimum = -100;
            this.pupilSize.Size = new System.Drawing.Size(480, 50);
            this.pupilSize.TabIndex = 65;
            // 
            // FaceForm
            // 
            this.ClientSize = new System.Drawing.Size(1011, 473);
            this.Controls.Add(this.pupilLabel);
            this.Controls.Add(this.eyesLabel);
            this.Controls.Add(this.button);
            this.Controls.Add(this.faceLabel);
            this.Controls.Add(this.browsLabel);
            this.Controls.Add(this.browAngle);
            this.Controls.Add(this.browX);
            this.Controls.Add(this.browY);
            this.Controls.Add(this.face);
            this.Controls.Add(this.browLength);
            this.Controls.Add(this.eyeHeight);
            this.Controls.Add(this.eyeWidth);
            this.Controls.Add(this.eyeY);
            this.Controls.Add(this.pupilX);
            this.Controls.Add(this.pupilY);
            this.Controls.Add(this.pupilSize);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedToolWindow;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "FaceForm";
            this.Text = "Face Animation";
            this.ResumeLayout(false);
            this.PerformLayout();

        }
    }
}
