namespace BlueCode
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

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
            this.components = new System.ComponentModel.Container();
            this.scanPicBox = new System.Windows.Forms.PictureBox();
            this.COMSelector = new System.Windows.Forms.ComboBox();
            this.button1 = new System.Windows.Forms.Button();
            this.serialPort1 = new System.IO.Ports.SerialPort(this.components);
            this.btnTCP = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.butSnapshot = new System.Windows.Forms.Button();
            this.btnCSV = new System.Windows.Forms.Button();
            this.txtCSV = new System.Windows.Forms.TextBox();
            ((System.ComponentModel.ISupportInitialize)(this.scanPicBox)).BeginInit();
            this.SuspendLayout();
            // 
            // scanPicBox
            // 
            this.scanPicBox.BackColor = System.Drawing.Color.Red;
            this.scanPicBox.Location = new System.Drawing.Point(12, 41);
            this.scanPicBox.Name = "scanPicBox";
            this.scanPicBox.Size = new System.Drawing.Size(640, 480);
            this.scanPicBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
            this.scanPicBox.TabIndex = 35;
            this.scanPicBox.TabStop = false;
            this.scanPicBox.Paint += new System.Windows.Forms.PaintEventHandler(this.scanPicBox_Paint);
            // 
            // COMSelector
            // 
            this.COMSelector.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.COMSelector.FormattingEnabled = true;
            this.COMSelector.Location = new System.Drawing.Point(85, 14);
            this.COMSelector.Name = "COMSelector";
            this.COMSelector.Size = new System.Drawing.Size(65, 21);
            this.COMSelector.TabIndex = 37;
            this.COMSelector.Click += new System.EventHandler(this.COMSelector_Click);
            // 
            // button1
            // 
            this.button1.Location = new System.Drawing.Point(12, 12);
            this.button1.Name = "button1";
            this.button1.Size = new System.Drawing.Size(67, 23);
            this.button1.TabIndex = 36;
            this.button1.Text = "Connect";
            this.button1.UseVisualStyleBackColor = true;
            this.button1.Click += new System.EventHandler(this.button1_Click);
            // 
            // serialPort1
            // 
            this.serialPort1.BaudRate = 1500000;
            this.serialPort1.PortName = "COM4";
            this.serialPort1.DataReceived += new System.IO.Ports.SerialDataReceivedEventHandler(this.serialPort1_DataReceived);
            // 
            // btnTCP
            // 
            this.btnTCP.Location = new System.Drawing.Point(195, 12);
            this.btnTCP.Name = "btnTCP";
            this.btnTCP.Size = new System.Drawing.Size(75, 23);
            this.btnTCP.TabIndex = 38;
            this.btnTCP.Text = "UDP";
            this.btnTCP.UseVisualStyleBackColor = true;
            this.btnTCP.Click += new System.EventHandler(this.btnTCP_Click);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(484, 17);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(142, 13);
            this.label1.TabIndex = 39;
            this.label1.Text = "REMEMBER:  Not hub-safe!";
            // 
            // butSnapshot
            // 
            this.butSnapshot.Location = new System.Drawing.Point(276, 12);
            this.butSnapshot.Name = "butSnapshot";
            this.butSnapshot.Size = new System.Drawing.Size(75, 23);
            this.butSnapshot.TabIndex = 40;
            this.butSnapshot.Text = "Snapshot";
            this.butSnapshot.UseVisualStyleBackColor = true;
            this.butSnapshot.Click += new System.EventHandler(this.butSnapshot_Click);
            // 
            // btnCSV
            // 
            this.btnCSV.Location = new System.Drawing.Point(357, 12);
            this.btnCSV.Name = "btnCSV";
            this.btnCSV.Size = new System.Drawing.Size(75, 23);
            this.btnCSV.TabIndex = 41;
            this.btnCSV.Text = "From CSV";
            this.btnCSV.UseVisualStyleBackColor = true;
            this.btnCSV.Click += new System.EventHandler(this.btnCSV_Click);
            // 
            // txtCSV
            // 
            this.txtCSV.Location = new System.Drawing.Point(357, 33);
            this.txtCSV.Multiline = true;
            this.txtCSV.Name = "txtCSV";
            this.txtCSV.Size = new System.Drawing.Size(295, 291);
            this.txtCSV.TabIndex = 42;
            this.txtCSV.Visible = false;
            this.txtCSV.TextChanged += new System.EventHandler(this.txtCSV_TextChanged);
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(659, 529);
            this.Controls.Add(this.txtCSV);
            this.Controls.Add(this.btnCSV);
            this.Controls.Add(this.butSnapshot);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.btnTCP);
            this.Controls.Add(this.COMSelector);
            this.Controls.Add(this.button1);
            this.Controls.Add(this.scanPicBox);
            this.Name = "Form1";
            this.Text = "Lens Tester";
            ((System.ComponentModel.ISupportInitialize)(this.scanPicBox)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.PictureBox scanPicBox;
        private System.Windows.Forms.ComboBox COMSelector;
        private System.Windows.Forms.Button button1;
        private System.IO.Ports.SerialPort serialPort1;
        private System.Windows.Forms.Button btnTCP;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Button butSnapshot;
        private System.Windows.Forms.Button btnCSV;
        private System.Windows.Forms.TextBox txtCSV;

    }
}

