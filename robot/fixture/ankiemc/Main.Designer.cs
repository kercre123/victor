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
            this.lblDebugStat = new System.Windows.Forms.Label();
            this.txtCommand = new System.Windows.Forms.TextBox();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.lblCopy = new System.Windows.Forms.Label();
            this.tabs = new System.Windows.Forms.TabControl();
            this.lblPeaks = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.label4 = new System.Windows.Forms.Label();
            this.label5 = new System.Windows.Forms.Label();
            this.label6 = new System.Windows.Forms.Label();
            this.label7 = new System.Windows.Forms.Label();
            this.lblTest = new System.Windows.Forms.Label();
            this.lblAnalMode = new System.Windows.Forms.Label();
            this.tabPage1.SuspendLayout();
            this.tabs.SuspendLayout();
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
            this.list.Size = new System.Drawing.Size(600, 64);
            this.list.TabIndex = 0;
            this.list.UseCompatibleStateImageBehavior = false;
            this.list.View = System.Windows.Forms.View.Details;
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
            this.btnCopy.Location = new System.Drawing.Point(455, 360);
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
            this.lblLogging.Location = new System.Drawing.Point(9, 366);
            this.lblLogging.MaximumSize = new System.Drawing.Size(440, 40);
            this.lblLogging.MinimumSize = new System.Drawing.Size(440, 40);
            this.lblLogging.Name = "lblLogging";
            this.lblLogging.ReadOnly = true;
            this.lblLogging.Size = new System.Drawing.Size(440, 17);
            this.lblLogging.TabIndex = 7;
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
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.lblCopy);
            this.tabPage1.Location = new System.Drawing.Point(4, 27);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(609, 363);
            this.tabPage1.TabIndex = 0;
            this.tabPage1.Text = "Info";
            this.tabPage1.UseVisualStyleBackColor = true;
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
            this.tabs.Font = new System.Drawing.Font("Microsoft Sans Serif", 11.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.tabs.Location = new System.Drawing.Point(1000, 0);
            this.tabs.Name = "tabs";
            this.tabs.SelectedIndex = 0;
            this.tabs.Size = new System.Drawing.Size(617, 394);
            this.tabs.TabIndex = 9;
            this.tabs.Visible = false;
            this.tabs.SelectedIndexChanged += new System.EventHandler(this.tabs_SelectedIndexChanged);
            // 
            // lblPeaks
            // 
            this.lblPeaks.BackColor = System.Drawing.SystemColors.ControlDark;
            this.lblPeaks.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.lblPeaks.Font = new System.Drawing.Font("Microsoft Sans Serif", 26.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblPeaks.Location = new System.Drawing.Point(9, 221);
            this.lblPeaks.Name = "lblPeaks";
            this.lblPeaks.Size = new System.Drawing.Size(600, 122);
            this.lblPeaks.TabIndex = 12;
            this.lblPeaks.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label2.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
            this.label2.Location = new System.Drawing.Point(606, 230);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(25, 13);
            this.label2.TabIndex = 13;
            this.label2.Text = " -10";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label3.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
            this.label3.Location = new System.Drawing.Point(606, 270);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(25, 13);
            this.label3.TabIndex = 14;
            this.label3.Text = " -30";
            // 
            // label4
            // 
            this.label4.AutoSize = true;
            this.label4.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label4.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
            this.label4.Location = new System.Drawing.Point(606, 310);
            this.label4.Name = "label4";
            this.label4.Size = new System.Drawing.Size(25, 13);
            this.label4.TabIndex = 15;
            this.label4.Text = " -50";
            // 
            // label5
            // 
            this.label5.AutoSize = true;
            this.label5.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label5.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
            this.label5.Location = new System.Drawing.Point(606, 290);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(25, 13);
            this.label5.TabIndex = 17;
            this.label5.Text = " -40";
            // 
            // label6
            // 
            this.label6.AutoSize = true;
            this.label6.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label6.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
            this.label6.Location = new System.Drawing.Point(606, 330);
            this.label6.Name = "label6";
            this.label6.Size = new System.Drawing.Size(25, 13);
            this.label6.TabIndex = 18;
            this.label6.Text = " -60";
            // 
            // label7
            // 
            this.label7.AutoSize = true;
            this.label7.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Underline, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label7.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
            this.label7.Location = new System.Drawing.Point(606, 250);
            this.label7.Name = "label7";
            this.label7.Size = new System.Drawing.Size(25, 13);
            this.label7.TabIndex = 16;
            this.label7.Text = " -20";
            // 
            // lblTest
            // 
            this.lblTest.BackColor = System.Drawing.SystemColors.ControlDark;
            this.lblTest.BorderStyle = System.Windows.Forms.BorderStyle.Fixed3D;
            this.lblTest.Font = new System.Drawing.Font("Microsoft Sans Serif", 26.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lblTest.Location = new System.Drawing.Point(9, 111);
            this.lblTest.Name = "lblTest";
            this.lblTest.Size = new System.Drawing.Size(600, 108);
            this.lblTest.TabIndex = 19;
            this.lblTest.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // lblAnalMode
            // 
            this.lblAnalMode.AutoSize = true;
            this.lblAnalMode.ForeColor = System.Drawing.SystemColors.ControlText;
            this.lblAnalMode.Location = new System.Drawing.Point(7, 343);
            this.lblAnalMode.Name = "lblAnalMode";
            this.lblAnalMode.Size = new System.Drawing.Size(0, 13);
            this.lblAnalMode.TabIndex = 20;
            // 
            // Main
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(628, 397);
            this.Controls.Add(this.lblAnalMode);
            this.Controls.Add(this.lblTest);
            this.Controls.Add(this.lblPeaks);
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
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label4);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label7);
            this.Controls.Add(this.label5);
            this.Controls.Add(this.label6);
            this.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
            this.MaximumSize = new System.Drawing.Size(644, 436);
            this.MinimumSize = new System.Drawing.Size(644, 436);
            this.Name = "Main";
            this.Text = "ANKI EMC v12";
            this.tabPage1.ResumeLayout(false);
            this.tabs.ResumeLayout(false);
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
        private System.Windows.Forms.Label lblDebugStat;
        private System.Windows.Forms.TextBox txtCommand;
        private System.Windows.Forms.TabPage tabPage1;
        private System.Windows.Forms.Label lblCopy;
        private System.Windows.Forms.TabControl tabs;
        private System.Windows.Forms.Label lblPeaks;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label label4;
        private System.Windows.Forms.Label label5;
        private System.Windows.Forms.Label label6;
        private System.Windows.Forms.Label label7;
        private System.Windows.Forms.Label lblTest;
        private System.Windows.Forms.Label lblAnalMode;
    }
}

