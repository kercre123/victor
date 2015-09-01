using System;
using System.Windows.Forms;
using System.Drawing;
using System.Net;

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

        public FaceForm()
        {
            InitializeComponent();
        }

        public DialogResult Open(Point location)
        {
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
            // timeLabel
            // 
            this.timeLabel.AutoSize = true;
            this.timeLabel.Location = new System.Drawing.Point(871, 415);
            this.timeLabel.Name = "timeLabel";
            this.timeLabel.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.timeLabel.Size = new System.Drawing.Size(69, 13);
            this.timeLabel.TabIndex = 33;
            this.timeLabel.Text = "Hold Time (s)";
            this.timeLabel.TextAlign = System.Drawing.ContentAlignment.BottomLeft;
            // 
            // timeTextBox
            // 
            this.timeTextBox.Location = new System.Drawing.Point(946, 412);
            this.timeTextBox.Name = "timeTextBox";
            this.timeTextBox.Size = new System.Drawing.Size(53, 20);
            this.timeTextBox.TabIndex = 34;
            this.timeTextBox.Text = "0.1";
            // 
            // browAngle
            // 
            this.browAngle.CheckBoxVisible = true;
            this.browAngle.ClientSize = new System.Drawing.Size(480, 50);
            this.browAngle.ControlBox = false;
            this.browAngle.LeftLabel = "Angle (deg)";
            this.browAngle.LeftMaximum = 90;
            this.browAngle.LeftMinimum = 0;
            this.browAngle.Location = new System.Drawing.Point(514, 24);
            this.browAngle.Name = "browAngle";
            this.browAngle.RightLabel = "Angle (deg)";
            this.browAngle.RightMaximum = 90;
            this.browAngle.RightMinimum = 0;
            this.browAngle.ShowInTaskbar = false;
            this.browAngle.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.browAngle.Visible = false;
            // 
            // browX
            // 
            this.browX.CheckBoxVisible = true;
            this.browX.ClientSize = new System.Drawing.Size(480, 50);
            this.browX.ControlBox = false;
            this.browX.LeftLabel = "X";
            this.browX.LeftMaximum = 90;
            this.browX.LeftMinimum = 0;
            this.browX.Location = new System.Drawing.Point(514, 96);
            this.browX.Name = "browX";
            this.browX.RightLabel = "X";
            this.browX.RightMaximum = 100;
            this.browX.RightMinimum = 0;
            this.browX.ShowInTaskbar = false;
            this.browX.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.browX.Visible = false;
            // 
            // browY
            // 
            this.browY.CheckBoxVisible = true;
            this.browY.ClientSize = new System.Drawing.Size(480, 50);
            this.browY.ControlBox = false;
            this.browY.LeftLabel = "Y";
            this.browY.LeftMaximum = 90;
            this.browY.LeftMinimum = 0;
            this.browY.Location = new System.Drawing.Point(514, 168);
            this.browY.Name = "browY";
            this.browY.RightLabel = "Y";
            this.browY.RightMaximum = 100;
            this.browY.RightMinimum = 0;
            this.browY.ShowInTaskbar = false;
            this.browY.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.browY.Visible = false;
            // 
            // face
            // 
            this.face.CheckBoxVisible = false;
            this.face.ClientSize = new System.Drawing.Size(480, 50);
            this.face.ControlBox = false;
            this.face.LeftLabel = "Y";
            this.face.LeftMaximum = 90;
            this.face.LeftMinimum = 0;
            this.face.Location = new System.Drawing.Point(514, 335);
            this.face.Name = "face";
            this.face.RightLabel = "Angle (deg)";
            this.face.RightMaximum = 90;
            this.face.RightMinimum = 0;
            this.face.ShowInTaskbar = false;
            this.face.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.face.Visible = false;
            // 
            // browLength
            // 
            this.browLength.CheckBoxVisible = true;
            this.browLength.ClientSize = new System.Drawing.Size(480, 50);
            this.browLength.ControlBox = false;
            this.browLength.LeftLabel = "Length";
            this.browLength.LeftMaximum = 90;
            this.browLength.LeftMinimum = 0;
            this.browLength.Location = new System.Drawing.Point(514, 240);
            this.browLength.Name = "browLength";
            this.browLength.RightLabel = "Length";
            this.browLength.RightMaximum = 100;
            this.browLength.RightMinimum = 0;
            this.browLength.ShowInTaskbar = false;
            this.browLength.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.browLength.Visible = false;
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
            this.eyeHeight.ClientSize = new System.Drawing.Size(480, 50);
            this.eyeHeight.ControlBox = false;
            this.eyeHeight.LeftLabel = "Height";
            this.eyeHeight.LeftMaximum = 90;
            this.eyeHeight.LeftMinimum = 0;
            this.eyeHeight.Location = new System.Drawing.Point(0, 24);
            this.eyeHeight.Name = "eyeHeight";
            this.eyeHeight.RightLabel = "Height";
            this.eyeHeight.RightMaximum = 90;
            this.eyeHeight.RightMinimum = 0;
            this.eyeHeight.ShowInTaskbar = false;
            this.eyeHeight.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.eyeHeight.Visible = false;
            // 
            // eyeWidth
            // 
            this.eyeWidth.CheckBoxVisible = true;
            this.eyeWidth.ClientSize = new System.Drawing.Size(480, 50);
            this.eyeWidth.ControlBox = false;
            this.eyeWidth.LeftLabel = "Width";
            this.eyeWidth.LeftMaximum = 90;
            this.eyeWidth.LeftMinimum = 0;
            this.eyeWidth.Location = new System.Drawing.Point(0, 96);
            this.eyeWidth.Name = "eyeWidth";
            this.eyeWidth.RightLabel = "Width";
            this.eyeWidth.RightMaximum = 90;
            this.eyeWidth.RightMinimum = 0;
            this.eyeWidth.ShowInTaskbar = false;
            this.eyeWidth.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.eyeWidth.Visible = false;
            // 
            // eyeY
            // 
            this.eyeY.CheckBoxVisible = true;
            this.eyeY.ClientSize = new System.Drawing.Size(480, 50);
            this.eyeY.ControlBox = false;
            this.eyeY.LeftLabel = "Y";
            this.eyeY.LeftMaximum = 90;
            this.eyeY.LeftMinimum = 0;
            this.eyeY.Location = new System.Drawing.Point(0, 168);
            this.eyeY.Name = "eyeY";
            this.eyeY.RightLabel = "Y";
            this.eyeY.RightMaximum = 90;
            this.eyeY.RightMinimum = 0;
            this.eyeY.ShowInTaskbar = false;
            this.eyeY.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.eyeY.Visible = false;
            // 
            // pupilX
            // 
            this.pupilX.CheckBoxVisible = true;
            this.pupilX.ClientSize = new System.Drawing.Size(480, 50);
            this.pupilX.ControlBox = false;
            this.pupilX.LeftLabel = "X";
            this.pupilX.LeftMaximum = 90;
            this.pupilX.LeftMinimum = 0;
            this.pupilX.Location = new System.Drawing.Point(0, 263);
            this.pupilX.Name = "pupilX";
            this.pupilX.RightLabel = "X";
            this.pupilX.RightMaximum = 90;
            this.pupilX.RightMinimum = 0;
            this.pupilX.ShowInTaskbar = false;
            this.pupilX.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.pupilX.Visible = false;
            // 
            // pupilY
            // 
            this.pupilY.CheckBoxVisible = true;
            this.pupilY.ClientSize = new System.Drawing.Size(480, 50);
            this.pupilY.ControlBox = false;
            this.pupilY.LeftLabel = "Y";
            this.pupilY.LeftMaximum = 90;
            this.pupilY.LeftMinimum = 0;
            this.pupilY.Location = new System.Drawing.Point(0, 335);
            this.pupilY.Name = "pupilY";
            this.pupilY.RightLabel = "Y";
            this.pupilY.RightMaximum = 90;
            this.pupilY.RightMinimum = 0;
            this.pupilY.ShowInTaskbar = false;
            this.pupilY.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.pupilY.Visible = false;
            // 
            // pupilSize
            // 
            this.pupilSize.CheckBoxVisible = true;
            this.pupilSize.ClientSize = new System.Drawing.Size(480, 50);
            this.pupilSize.ControlBox = false;
            this.pupilSize.LeftLabel = "Size";
            this.pupilSize.LeftMaximum = 90;
            this.pupilSize.LeftMinimum = 0;
            this.pupilSize.Location = new System.Drawing.Point(0, 407);
            this.pupilSize.Name = "pupilSize";
            this.pupilSize.RightLabel = "Size";
            this.pupilSize.RightMaximum = 90;
            this.pupilSize.RightMinimum = 0;
            this.pupilSize.ShowInTaskbar = false;
            this.pupilSize.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.pupilSize.Visible = false;
            // 
            // FaceForm
            // 
            this.ClientSize = new System.Drawing.Size(1011, 473);
            this.Controls.Add(this.pupilLabel);
            this.Controls.Add(this.eyesLabel);
            this.Controls.Add(this.timeTextBox);
            this.Controls.Add(this.timeLabel);
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
