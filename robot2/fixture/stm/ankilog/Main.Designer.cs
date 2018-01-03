namespace AnkiLog
{
    partial class Main
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Main));
            this.list = new System.Windows.Forms.ListView();
            this.Body = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.FSN = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.COM = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.LotCode = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.Status = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.ESN = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.Cycles = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.btnCopy = new System.Windows.Forms.Button();
            this.timer = new System.Windows.Forms.Timer(this.components);
            this.label1 = new System.Windows.Forms.Label();
            this.txtLotCode = new System.Windows.Forms.TextBox();
            this.txtDate = new System.Windows.Forms.TextBox();
            this.textBox2 = new System.Windows.Forms.TextBox();
            this.lblNow = new System.Windows.Forms.Label();
            this.lblLogging = new System.Windows.Forms.TextBox();
            this.lblCopy = new System.Windows.Forms.Label();
            this.tabs = new System.Windows.Forms.TabControl();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.label2 = new System.Windows.Forms.Label();
            this.picCam = new System.Windows.Forms.PictureBox();
            this.txtInfo = new System.Windows.Forms.TextBox();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.label4 = new System.Windows.Forms.Label();
            this.listColors = new System.Windows.Forms.ListBox();
            this.tabPage3 = new System.Windows.Forms.TabPage();
            this.label3 = new System.Windows.Forms.Label();
            this.listFiles = new System.Windows.Forms.ListBox();
            this.lblDebugStat = new System.Windows.Forms.Label();
            this.txtCommand = new System.Windows.Forms.TextBox();
            this.tabs.SuspendLayout();
            this.tabPage1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.picCam)).BeginInit();
            this.tabPage2.SuspendLayout();
            this.tabPage3.SuspendLayout();
            this.SuspendLayout();
            // 
            // list
            // 
            this.list.BackColor = System.Drawing.SystemColors.Window;
            this.list.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.Body,
            this.FSN,
            this.COM,
            this.LotCode,
            this.Status,
            this.ESN,
            this.Cycles});
            this.list.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.list.FullRowSelect = true;
            this.list.GridLines = true;
            this.list.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.list.HideSelection = false;
            this.list.Location = new System.Drawing.Point(9, 44);
            this.list.MultiSelect = false;
            this.list.Name = "list";
            this.list.Size = new System.Drawing.Size(600, 308);
            this.list.TabIndex = 0;
            this.list.UseCompatibleStateImageBehavior = false;
            this.list.View = System.Windows.Forms.View.Details;
            this.list.ItemSelectionChanged += new System.Windows.Forms.ListViewItemSelectionChangedEventHandler(this.list_ItemSelectionChanged);
            this.list.MouseClick += new System.Windows.Forms.MouseEventHandler(this.list_MouseClick);
            // 
            // Body
            // 
            this.Body.Text = "Body";
            this.Body.Width = 0;
            // 
            // FSN
            // 
            this.FSN.Text = "Fixture";
            this.FSN.Width = 171;
            // 
            // COM
            // 
            this.COM.Text = "COM";
            this.COM.Width = 49;
            // 
            // LotCode
            // 
            this.LotCode.Text = "Lot Code";
            this.LotCode.Width = 96;
            // 
            // Status
            // 
            this.Status.Text = "Status";
            this.Status.Width = 105;
            // 
            // ESN
            // 
            this.ESN.Text = "ESN";
            this.ESN.Width = 106;
            // 
            // Cycles
            // 
            this.Cycles.Text = "Cycles";
            this.Cycles.Width = 69;
            // 
            // btnCopy
            // 
            this.btnCopy.Enabled = false;
            this.btnCopy.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btnCopy.Location = new System.Drawing.Point(458, 358);
            this.btnCopy.Name = "btnCopy";
            this.btnCopy.Size = new System.Drawing.Size(154, 30);
            this.btnCopy.TabIndex = 1;
            this.btnCopy.Text = "Copy to USB";
            this.btnCopy.UseVisualStyleBackColor = true;
            this.btnCopy.Click += new System.EventHandler(this.btnCopy_Click);
            // 
            // timer
            // 
            this.timer.Enabled = true;
            this.timer.Interval = 1000;
            this.timer.Tick += new System.EventHandler(this.timer_Tick);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label1.Location = new System.Drawing.Point(8, 14);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(140, 20);
            this.label1.TabIndex = 2;
            this.label1.Text = "LOT/DATE CODE:";
            // 
            // txtLotCode
            // 
            this.txtLotCode.Font = new System.Drawing.Font("Courier New", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.txtLotCode.Location = new System.Drawing.Point(150, 12);
            this.txtLotCode.Name = "txtLotCode";
            this.txtLotCode.ReadOnly = true;
            this.txtLotCode.Size = new System.Drawing.Size(37, 26);
            this.txtLotCode.TabIndex = 1;
            this.txtLotCode.Text = "111";
            this.txtLotCode.TextChanged += new System.EventHandler(this.txtLotCode_TextChanged);
            // 
            // txtDate
            // 
            this.txtDate.Font = new System.Drawing.Font("Courier New", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.txtDate.Location = new System.Drawing.Point(185, 12);
            this.txtDate.Name = "txtDate";
            this.txtDate.ReadOnly = true;
            this.txtDate.Size = new System.Drawing.Size(37, 26);
            this.txtDate.TabIndex = 4;
            // 
            // textBox2
            // 
            this.textBox2.Font = new System.Drawing.Font("Courier New", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.textBox2.Location = new System.Drawing.Point(218, 12);
            this.textBox2.Name = "textBox2";
            this.textBox2.ReadOnly = true;
            this.textBox2.Size = new System.Drawing.Size(27, 26);
            this.textBox2.TabIndex = 5;
            this.textBox2.Text = "XX";
            // 
            // lblNow
            // 
            this.lblNow.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblNow.Location = new System.Drawing.Point(391, 14);
            this.lblNow.Margin = new System.Windows.Forms.Padding(0);
            this.lblNow.Name = "lblNow";
            this.lblNow.Size = new System.Drawing.Size(221, 20);
            this.lblNow.TabIndex = 6;
            this.lblNow.Text = "TIME: ";
            this.lblNow.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // lblLogging
            // 
            this.lblLogging.BackColor = System.Drawing.SystemColors.Control;
            this.lblLogging.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.lblLogging.Font = new System.Drawing.Font("Microsoft Sans Serif", 11.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblLogging.Location = new System.Drawing.Point(12, 364);
            this.lblLogging.MaximumSize = new System.Drawing.Size(440, 20);
            this.lblLogging.MinimumSize = new System.Drawing.Size(440, 20);
            this.lblLogging.Name = "lblLogging";
            this.lblLogging.ReadOnly = true;
            this.lblLogging.Size = new System.Drawing.Size(440, 17);
            this.lblLogging.TabIndex = 7;
            // 
            // lblCopy
            // 
            this.lblCopy.BackColor = System.Drawing.SystemColors.Control;
            this.lblCopy.Font = new System.Drawing.Font("Microsoft Sans Serif", 15.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblCopy.Location = new System.Drawing.Point(-9, -27);
            this.lblCopy.Name = "lblCopy";
            this.lblCopy.Size = new System.Drawing.Size(622, 389);
            this.lblCopy.TabIndex = 8;
            this.lblCopy.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            this.lblCopy.Visible = false;
            // 
            // tabs
            // 
            this.tabs.Controls.Add(this.tabPage1);
            this.tabs.Controls.Add(this.tabPage2);
            this.tabs.Controls.Add(this.tabPage3);
            this.tabs.Font = new System.Drawing.Font("Microsoft Sans Serif", 11.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.tabs.Location = new System.Drawing.Point(5, 1);
            this.tabs.Name = "tabs";
            this.tabs.SelectedIndex = 0;
            this.tabs.Size = new System.Drawing.Size(617, 394);
            this.tabs.TabIndex = 9;
            this.tabs.Visible = false;
            this.tabs.SelectedIndexChanged += new System.EventHandler(this.tabs_SelectedIndexChanged);
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.lblCopy);
            this.tabPage1.Controls.Add(this.label2);
            this.tabPage1.Controls.Add(this.picCam);
            this.tabPage1.Controls.Add(this.txtInfo);
            this.tabPage1.Location = new System.Drawing.Point(4, 27);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(609, 363);
            this.tabPage1.TabIndex = 0;
            this.tabPage1.Text = "Car Info";
            this.tabPage1.UseVisualStyleBackColor = true;
            this.tabPage1.Paint += new System.Windows.Forms.PaintEventHandler(this.tabPage1_Paint);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(443, 214);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(96, 18);
            this.label2.TabIndex = 3;
            this.label2.Text = "Camera View";
            // 
            // picCam
            // 
            this.picCam.BackColor = System.Drawing.Color.Black;
            this.picCam.Location = new System.Drawing.Point(446, 235);
            this.picCam.Name = "picCam";
            this.picCam.Size = new System.Drawing.Size(160, 120);
            this.picCam.TabIndex = 2;
            this.picCam.TabStop = false;
            this.picCam.Paint += new System.Windows.Forms.PaintEventHandler(this.picCam_Paint);
            // 
            // txtInfo
            // 
            this.txtInfo.Location = new System.Drawing.Point(6, 9);
            this.txtInfo.Multiline = true;
            this.txtInfo.Name = "txtInfo";
            this.txtInfo.ReadOnly = true;
            this.txtInfo.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.txtInfo.Size = new System.Drawing.Size(434, 346);
            this.txtInfo.TabIndex = 1;
            this.txtInfo.WordWrap = false;
            // 
            // tabPage2
            // 
            this.tabPage2.Controls.Add(this.label4);
            this.tabPage2.Controls.Add(this.listColors);
            this.tabPage2.Location = new System.Drawing.Point(4, 27);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(609, 363);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "Change Color";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Location = new System.Drawing.Point(5, 6);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(345, 126);
            this.label4.TabIndex = 3;
            this.label4.Text = resources.GetString("label4.Text");
            // 
            // listColors
            // 
            this.listColors.BackColor = System.Drawing.SystemColors.Control;
            this.listColors.Font = new System.Drawing.Font("Courier New", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.listColors.FormattingEnabled = true;
            this.listColors.ItemHeight = 18;
            this.listColors.Location = new System.Drawing.Point(361, 6);
            this.listColors.Name = "listColors";
            this.listColors.Size = new System.Drawing.Size(243, 346);
            this.listColors.TabIndex = 2;
            // 
            // tabPage3
            // 
            this.tabPage3.Controls.Add(this.label3);
            this.tabPage3.Controls.Add(this.listFiles);
            this.tabPage3.Location = new System.Drawing.Point(4, 27);
            this.tabPage3.Name = "tabPage3";
            this.tabPage3.Size = new System.Drawing.Size(609, 363);
            this.tabPage3.TabIndex = 2;
            this.tabPage3.Text = "Change Firmware";
            this.tabPage3.UseVisualStyleBackColor = true;
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(4, 6);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(336, 90);
            this.label3.TabIndex = 1;
            this.label3.Text = resources.GetString("label3.Text");
            // 
            // listFiles
            // 
            this.listFiles.BackColor = System.Drawing.SystemColors.Control;
            this.listFiles.FormattingEnabled = true;
            this.listFiles.ItemHeight = 18;
            this.listFiles.Location = new System.Drawing.Point(346, 6);
            this.listFiles.Name = "listFiles";
            this.listFiles.Size = new System.Drawing.Size(257, 346);
            this.listFiles.TabIndex = 0;
            // 
            // lblDebugStat
            // 
            this.lblDebugStat.Font = new System.Drawing.Font("Microsoft Sans Serif", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblDebugStat.Location = new System.Drawing.Point(395, 6);
            this.lblDebugStat.Name = "lblDebugStat";
            this.lblDebugStat.Size = new System.Drawing.Size(223, 20);
            this.lblDebugStat.TabIndex = 10;
            this.lblDebugStat.Text = "Connecting...";
            this.lblDebugStat.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.lblDebugStat.Visible = false;
            // 
            // txtCommand
            // 
            this.txtCommand.BackColor = System.Drawing.SystemColors.ControlLightLight;
            this.txtCommand.Font = new System.Drawing.Font("Arial Narrow", 12F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.txtCommand.Location = new System.Drawing.Point(251, 12);
            this.txtCommand.Name = "txtCommand";
            this.txtCommand.Size = new System.Drawing.Size(138, 26);
            this.txtCommand.TabIndex = 11;
            this.txtCommand.KeyDown += new System.Windows.Forms.KeyEventHandler(this.txtCommand_KeyDown);
            // 
            // Main
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(624, 397);
            this.Controls.Add(this.tabs);
            this.Controls.Add(this.lblDebugStat);
            this.Controls.Add(this.lblLogging);
            this.Controls.Add(this.lblNow);
            this.Controls.Add(this.textBox2);
            this.Controls.Add(this.txtDate);
            this.Controls.Add(this.txtLotCode);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.btnCopy);
            this.Controls.Add(this.list);
            this.Controls.Add(this.txtCommand);
            this.MaximumSize = new System.Drawing.Size(640, 436);
            this.MinimumSize = new System.Drawing.Size(640, 436);
            this.Name = "Main";
            this.Text = " COZMO LOG";
            this.tabs.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            this.tabPage1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.picCam)).EndInit();
            this.tabPage2.ResumeLayout(false);
            this.tabPage2.PerformLayout();
            this.tabPage3.ResumeLayout(false);
            this.tabPage3.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ListView list;
        private System.Windows.Forms.Button btnCopy;
        private System.Windows.Forms.ColumnHeader FSN;
        private System.Windows.Forms.ColumnHeader COM;
        private System.Windows.Forms.ColumnHeader Body;
        private System.Windows.Forms.ColumnHeader Status;
        private System.Windows.Forms.ColumnHeader Cycles;
        private System.Windows.Forms.ColumnHeader ESN;
        private System.Windows.Forms.Timer timer;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.TextBox txtLotCode;
        private System.Windows.Forms.TextBox txtDate;
        private System.Windows.Forms.TextBox textBox2;
        private System.Windows.Forms.ColumnHeader LotCode;
        private System.Windows.Forms.Label lblNow;
        private System.Windows.Forms.TextBox lblLogging;
        private System.Windows.Forms.Label lblCopy;
        private System.Windows.Forms.TabControl tabs;
        private System.Windows.Forms.TabPage tabPage1;
        private System.Windows.Forms.TabPage tabPage2;
        private System.Windows.Forms.TabPage tabPage3;
        private System.Windows.Forms.PictureBox picCam;
        private System.Windows.Forms.Label lblDebugStat;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.ListBox listColors;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.ListBox listFiles;
        private System.Windows.Forms.TextBox txtInfo;
        private System.Windows.Forms.TextBox txtCommand;
    }
}

