using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool
{
    public class ChartForm : ContainerControl
    {
        private Panel sidePanel;
        private CheckBox checkBox;
        private Panel panel;
        private PictureBox pictureBox;

        public Panel SidePanel { get { return sidePanel; } private set { sidePanel = value; } }
        public CheckBox CheckBox { get { return checkBox; } private set { checkBox = value; } }
        public Panel Panel { get { return panel; } private set { panel = value; } }
        public PictureBox PictureBox { get { return pictureBox; } private set { pictureBox = value; } }
        public Chart chart;

        public ChartForm()
        {
            InitializeComponent();
        }

        private void InitializeComponent()
        {
            this.sidePanel = new System.Windows.Forms.Panel();
            this.pictureBox = new System.Windows.Forms.PictureBox();
            this.checkBox = new System.Windows.Forms.CheckBox();
            this.panel = new System.Windows.Forms.Panel();
            this.sidePanel.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox)).BeginInit();
            this.SuspendLayout();
            // 
            // sidePanel
            // 
            this.sidePanel.BackColor = System.Drawing.SystemColors.AppWorkspace;
            this.sidePanel.Controls.Add(this.pictureBox);
            this.sidePanel.Controls.Add(this.checkBox);
            this.sidePanel.Dock = System.Windows.Forms.DockStyle.Right;
            this.sidePanel.Location = new System.Drawing.Point(1154, 0);
            this.sidePanel.Margin = new System.Windows.Forms.Padding(2);
            this.sidePanel.Name = "sidePanel";
            this.sidePanel.Size = new System.Drawing.Size(110, 134);
            this.sidePanel.TabIndex = 3;
            // 
            // pictureBox
            // 
            this.pictureBox.Location = new System.Drawing.Point(5, 42);
            this.pictureBox.Name = "pictureBox";
            this.pictureBox.Size = new System.Drawing.Size(100, 50);
            this.pictureBox.TabIndex = 6;
            this.pictureBox.TabStop = false;
            // 
            // checkBox
            // 
            this.checkBox.AutoSize = true;
            this.checkBox.Location = new System.Drawing.Point(11, 11);
            this.checkBox.Margin = new System.Windows.Forms.Padding(2);
            this.checkBox.Name = "checkBox";
            this.checkBox.Size = new System.Drawing.Size(103, 17);
            this.checkBox.TabIndex = 4;
            this.checkBox.Text = "Disable Channel";
            this.checkBox.UseVisualStyleBackColor = true;
            // 
            // panel
            // 
            this.panel.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel.Location = new System.Drawing.Point(0, 0);
            this.panel.Name = "panel";
            this.panel.Size = new System.Drawing.Size(1154, 134);
            this.panel.TabIndex = 4;
            // 
            // ChartForm
            // 
            this.Controls.Add(this.panel);
            this.Controls.Add(this.sidePanel);
            this.Size = new System.Drawing.Size(1264, 134);
            this.sidePanel.ResumeLayout(false);
            this.sidePanel.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox)).EndInit();
            this.ResumeLayout(false);

        }
    }
}
