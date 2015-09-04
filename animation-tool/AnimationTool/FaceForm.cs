using System;
using System.Windows.Forms;
using System.Drawing;
using System.Net;
using System.Collections.Generic;

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
        private Label timeLabel;
        private TextBox timeTextBox;
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

        public KeyValuePair<int, int> EyeHeight { get { return new KeyValuePair<int, int>(eyeHeight.LeftValue, eyeHeight.RightValue); } }
        public KeyValuePair<int, int> EyeWidth { get { return new KeyValuePair<int, int>(eyeWidth.LeftValue, eyeWidth.RightValue); } }
        public KeyValuePair<int, int> EyeY { get { return new KeyValuePair<int, int>(eyeY.LeftValue, eyeY.RightValue); } }
        public KeyValuePair<int, int> PupilX { get { return new KeyValuePair<int, int>(pupilX.LeftValue, pupilX.RightValue); } }
        public KeyValuePair<int, int> PupilY { get { return new KeyValuePair<int, int>(pupilY.LeftValue, pupilY.RightValue); } }
        public KeyValuePair<int, int> PupilSize { get { return new KeyValuePair<int, int>(pupilSize.LeftValue, pupilSize.RightValue); } }
        public KeyValuePair<int, int> BrowY { get { return new KeyValuePair<int, int>(browY.LeftValue, browY.RightValue); } }
        public KeyValuePair<int, int> BrowAngle { get { return new KeyValuePair<int, int>(browAngle.LeftValue, browAngle.RightValue); } }
        public int FaceAngle { get { return face.RightValue; } }
        public int FaceY { get { return face.LeftValue; } }

        public bool Changed
        {
            get
            {
                return browLength.Changed || eyeHeight.Changed || eyeWidth.Changed || eyeY.Changed || pupilX.Changed || pupilY.Changed || pupilSize.Changed || browY.Changed;
            }

            set
            {
                browLength.Changed = value;
                eyeHeight.Changed = value;
                eyeWidth.Changed = value;
                eyeY.Changed = value;
                pupilX.Changed = value;
                pupilY.Changed = value;
                pupilSize.Changed = value;
                browY.Changed = value;
            }
        }

        public FaceForm()
        {
            InitializeComponent();
        }

        public DialogResult Open()
        {
            button.Enabled = true;

            return ShowDialog();
        }

        private void InitializeComponent()
        {
            this.browsLabel = new System.Windows.Forms.Label();
            this.faceLabel = new System.Windows.Forms.Label();
            this.button = new System.Windows.Forms.Button();
            this.timeLabel = new System.Windows.Forms.Label();
            this.timeTextBox = new System.Windows.Forms.TextBox();
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
            this.browAngle.CheckBoxVisible = true;
            this.browAngle.LeftLabel = "Angle (deg)";
            this.browAngle.LeftMaximum = 100;
            this.browAngle.LeftMinimum = 0;
            this.browAngle.Location = new System.Drawing.Point(514, 24);
            this.browAngle.Name = "browAngle";
            this.browAngle.RightLabel = "Angle (deg)";
            this.browAngle.RightMaximum = 100;
            this.browAngle.RightMinimum = 0;
            this.browAngle.Size = new System.Drawing.Size(480, 50);
            this.browAngle.TabIndex = 55;
            // 
            // browX
            // 
            this.browX.CheckBoxVisible = true;
            this.browX.LeftLabel = "X";
            this.browX.LeftMaximum = 100;
            this.browX.LeftMinimum = 0;
            this.browX.Location = new System.Drawing.Point(514, 96);
            this.browX.Name = "browX";
            this.browX.RightLabel = "X";
            this.browX.RightMaximum = 100;
            this.browX.RightMinimum = 0;
            this.browX.Size = new System.Drawing.Size(480, 50);
            this.browX.TabIndex = 56;
            // 
            // browY
            // 
            this.browY.CheckBoxVisible = true;
            this.browY.LeftLabel = "Y";
            this.browY.LeftMaximum = 100;
            this.browY.LeftMinimum = 0;
            this.browY.Location = new System.Drawing.Point(514, 168);
            this.browY.Name = "browY";
            this.browY.RightLabel = "Y";
            this.browY.RightMaximum = 100;
            this.browY.RightMinimum = 0;
            this.browY.Size = new System.Drawing.Size(480, 50);
            this.browY.TabIndex = 57;
            // 
            // face
            // 
            this.face.CheckBoxVisible = false;
            this.face.LeftLabel = "Y";
            this.face.LeftMaximum = 100;
            this.face.LeftMinimum = 0;
            this.face.Location = new System.Drawing.Point(514, 335);
            this.face.Name = "face";
            this.face.RightLabel = "Angle (deg)";
            this.face.RightMaximum = 100;
            this.face.RightMinimum = 0;
            this.face.Size = new System.Drawing.Size(480, 50);
            this.face.TabIndex = 58;
            // 
            // browLength
            // 
            this.browLength.CheckBoxVisible = true;
            this.browLength.LeftLabel = "Length";
            this.browLength.LeftMaximum = 100;
            this.browLength.LeftMinimum = 0;
            this.browLength.Location = new System.Drawing.Point(514, 240);
            this.browLength.Name = "browLength";
            this.browLength.RightLabel = "Length";
            this.browLength.RightMaximum = 100;
            this.browLength.RightMinimum = 0;
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
            this.eyeHeight.CheckBoxVisible = true;
            this.eyeHeight.LeftLabel = "Height";
            this.eyeHeight.LeftMaximum = 100;
            this.eyeHeight.LeftMinimum = 0;
            this.eyeHeight.Location = new System.Drawing.Point(0, 24);
            this.eyeHeight.Name = "eyeHeight";
            this.eyeHeight.RightLabel = "Height";
            this.eyeHeight.RightMaximum = 100;
            this.eyeHeight.RightMinimum = 0;
            this.eyeHeight.Size = new System.Drawing.Size(480, 50);
            this.eyeHeight.TabIndex = 60;
            // 
            // eyeWidth
            // 
            this.eyeWidth.CheckBoxVisible = true;
            this.eyeWidth.LeftLabel = "Width";
            this.eyeWidth.LeftMaximum = 100;
            this.eyeWidth.LeftMinimum = 0;
            this.eyeWidth.Location = new System.Drawing.Point(0, 96);
            this.eyeWidth.Name = "eyeWidth";
            this.eyeWidth.RightLabel = "Width";
            this.eyeWidth.RightMaximum = 100;
            this.eyeWidth.RightMinimum = 0;
            this.eyeWidth.Size = new System.Drawing.Size(480, 50);
            this.eyeWidth.TabIndex = 61;
            // 
            // eyeY
            // 
            this.eyeY.CheckBoxVisible = true;
            this.eyeY.LeftLabel = "Y";
            this.eyeY.LeftMaximum = 100;
            this.eyeY.LeftMinimum = 0;
            this.eyeY.Location = new System.Drawing.Point(0, 168);
            this.eyeY.Name = "eyeY";
            this.eyeY.RightLabel = "Y";
            this.eyeY.RightMaximum = 100;
            this.eyeY.RightMinimum = 0;
            this.eyeY.Size = new System.Drawing.Size(480, 50);
            this.eyeY.TabIndex = 62;
            // 
            // pupilX
            // 
            this.pupilX.CheckBoxVisible = true;
            this.pupilX.LeftLabel = "X";
            this.pupilX.LeftMaximum = 100;
            this.pupilX.LeftMinimum = 0;
            this.pupilX.Location = new System.Drawing.Point(0, 263);
            this.pupilX.Name = "pupilX";
            this.pupilX.RightLabel = "X";
            this.pupilX.RightMaximum = 100;
            this.pupilX.RightMinimum = 0;
            this.pupilX.Size = new System.Drawing.Size(480, 50);
            this.pupilX.TabIndex = 63;
            // 
            // pupilY
            // 
            this.pupilY.CheckBoxVisible = true;
            this.pupilY.LeftLabel = "Y";
            this.pupilY.LeftMaximum = 100;
            this.pupilY.LeftMinimum = 0;
            this.pupilY.Location = new System.Drawing.Point(0, 335);
            this.pupilY.Name = "pupilY";
            this.pupilY.RightLabel = "Y";
            this.pupilY.RightMaximum = 100;
            this.pupilY.RightMinimum = 0;
            this.pupilY.Size = new System.Drawing.Size(480, 50);
            this.pupilY.TabIndex = 64;
            // 
            // pupilSize
            // 
            this.pupilSize.CheckBoxVisible = true;
            this.pupilSize.LeftLabel = "Size";
            this.pupilSize.LeftMaximum = 100;
            this.pupilSize.LeftMinimum = 0;
            this.pupilSize.Location = new System.Drawing.Point(0, 407);
            this.pupilSize.Name = "pupilSize";
            this.pupilSize.RightLabel = "Size";
            this.pupilSize.RightMaximum = 100;
            this.pupilSize.RightMinimum = 0;
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
