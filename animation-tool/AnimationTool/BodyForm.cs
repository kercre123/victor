using System;
using System.Windows.Forms;

namespace AnimationTool
{
    public class BodyForm : Form
    {
        private Button straightButton;
        private Button arcButton;
        private Button turnInPlaceButton;

        public BodyForm()
        {
            this.FormBorderStyle = FormBorderStyle.FixedToolWindow;
            this.MaximizeBox = false;
            this.MinimizeBox = false;

            InitializeComponent();
        }

        private void InitializeComponent()
        {
            this.straightButton = new System.Windows.Forms.Button();
            this.arcButton = new System.Windows.Forms.Button();
            this.turnInPlaceButton = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // straightButton
            // 
            this.straightButton.Location = new System.Drawing.Point(12, 12);
            this.straightButton.Name = "straightButton";
            this.straightButton.Size = new System.Drawing.Size(100, 23);
            this.straightButton.TabIndex = 2;
            this.straightButton.Text = "Straight";
            this.straightButton.UseVisualStyleBackColor = true;
            this.straightButton.DialogResult = DialogResult.OK;
            // 
            // arcButton
            // 
            this.arcButton.Location = new System.Drawing.Point(12, 41);
            this.arcButton.Name = "arcButton";
            this.arcButton.Size = new System.Drawing.Size(100, 23);
            this.arcButton.TabIndex = 3;
            this.arcButton.Text = "Arc";
            this.arcButton.UseVisualStyleBackColor = true;
            this.arcButton.DialogResult = DialogResult.No;
            // 
            // turnInPlaceButton
            // 
            this.turnInPlaceButton.Location = new System.Drawing.Point(12, 70);
            this.turnInPlaceButton.Name = "turnInPlaceButton";
            this.turnInPlaceButton.Size = new System.Drawing.Size(100, 23);
            this.turnInPlaceButton.TabIndex = 3;
            this.turnInPlaceButton.Text = "Turn In Place";
            this.turnInPlaceButton.UseVisualStyleBackColor = true;
            this.turnInPlaceButton.DialogResult = DialogResult.Yes;
            // 
            // BodyForm
            // 
            this.ClientSize = new System.Drawing.Size(124, 106);
            this.Controls.Add(this.straightButton);
            this.Controls.Add(this.arcButton);
            this.Controls.Add(this.turnInPlaceButton);
            this.Name = "BodyForm";
            this.ResumeLayout(false);
        }
    }
}
