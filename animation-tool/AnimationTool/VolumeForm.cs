using System;
using System.Windows.Forms;

namespace AnimationTool
{
    public class VolumeForm : Form
    {
        private TextBox textBox;
        private Label label;
        private Button button;

        private double previousVolume;
        public double Volume { get; private set; }

        public VolumeForm()
        {
            FormBorderStyle = FormBorderStyle.FixedToolWindow;
            MaximizeBox = false;
            MinimizeBox = false;

            InitializeComponent();

            Volume = 1.0;
            button.Enabled = false;
        }

        private void InitializeComponent()
        {
            this.textBox = new System.Windows.Forms.TextBox();
            this.label = new System.Windows.Forms.Label();
            this.button = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // textBox
            // 
            this.textBox.Location = new System.Drawing.Point(91, 12);
            this.textBox.Name = "textBox";
            this.textBox.Size = new System.Drawing.Size(100, 20);
            this.textBox.TabIndex = 0;
            this.textBox.Text = "1";
            // 
            // label
            // 
            this.label.AutoSize = true;
            this.label.Location = new System.Drawing.Point(7, 15);
            this.label.Name = "label";
            this.label.RightToLeft = System.Windows.Forms.RightToLeft.No;
            this.label.Size = new System.Drawing.Size(78, 13);
            this.label.TabIndex = 2;
            this.label.Text = "Volume (0 to 1)";
            this.label.TextAlign = System.Drawing.ContentAlignment.BottomLeft;
            // 
            // button
            // 
            this.button.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.button.Enabled = false;
            this.button.Location = new System.Drawing.Point(91, 38);
            this.button.Name = "button";
            this.button.Size = new System.Drawing.Size(100, 23);
            this.button.TabIndex = 4;
            this.button.Text = "OK";
            this.button.UseVisualStyleBackColor = true;
            // 
            // VolumeForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(203, 72);
            this.Controls.Add(this.button);
            this.Controls.Add(this.label);
            this.Controls.Add(this.textBox);
            this.Name = "VolumeForm";
            this.ResumeLayout(false);
            this.PerformLayout();
        }

        public DialogResult Open(double volume, IWin32Window parent)
        {
            textBox.Text = volume.ToString();
            previousVolume = volume;

            StartPosition = FormStartPosition.CenterParent;

            return ShowDialog(parent);
        }

        private void OnTextChange(object o, EventArgs e)
        {
            button.Enabled = false;

            try
            {
                double newVolume = Math.Round(Convert.ToDouble(textBox.Text), 1);

                if (newVolume >= MoveSelectedDataPoints.DELTA_X && newVolume <= 1 && newVolume != previousVolume)
                {
                    Volume = newVolume;
                    button.Enabled = true;
                }
            }
            catch (Exception) { }
        }
    }
}

