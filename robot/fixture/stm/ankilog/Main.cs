using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Globalization;
using System.IO.Ports;
using System.IO;
using System.Threading;
using System.IO.Compression;

namespace AnkiLog
{
    public partial class Main : Form
    {
        // Switch the UI from production mode to vehicle debug mode
        private bool m_isDebugClient = false;
        private bool m_clickToCycleColor = false;   // Test code - should stay false for release builds

        // List of available bodies from 1 on up
        // 15 = Truck 1, 16 = Truck 2
        private string[] BODIES = { "E01", "E02", "F03", "F04", "X05", "I06", "G07", "Q08", "R09", "I10", "X11", "Q12", "R13", "V14", "T15", "T16" };
        private string[] BODYDESC = { "Kourai Yellow", "Boson Silver", "Rho Red", "Katal Blue", "Corax Black/Orange", "Hadion Orange", "Spektrix Purple", "Groundshock Blue", "Skull Gray", "Thermo Red", "Nuke Green", "Guardian", "Missingno.", "Big Bang", "Truck 1", "Truck 2" };
        private Color[] BODYCOLOR = { Color.FromArgb(255, 255, 0), Color.FromArgb(192, 192, 192),
                                      Color.FromArgb(255, 0, 0), Color.FromArgb(64, 64, 255),
                                      Color.FromArgb(255, 128, 0), Color.FromArgb(255, 128, 0),
                                      Color.FromArgb(255, 0, 255), Color.FromArgb(0, 128, 255),
                                      Color.FromArgb(255, 255, 255), Color.FromArgb(255, 0, 0),
                                      Color.FromArgb(0, 255, 0), Color.FromArgb(255, 255, 255),
                                      Color.FromArgb(255, 255, 255), Color.FromArgb(127, 127, 127),
                                      Color.FromArgb(255, 255, 255), Color.FromArgb(127, 127, 127)
                                    };

        private string m_lotPart1 = "";
        private string m_lotPart2 = "";
        private static string m_logPath = "";

        // Stupid list view columns constants..
        private const int CBODY = 0;
        private const int CFSN = 1;
        private const int CCOM = 2;
        private const int CLOT = 3;
        private const int CSTATUS = 4;
        private const int CESN = 5;
        private const int CCYCLES = 6;
        private const int CCOUNT = 7;

        // List of all error messages from diagnosticModeCommands.h
        private string[] m_errors = {
        "PASS","000",                                      
        "EMPTY_COMMAND","001",
        "ACK1","002",
        "ACK2","003",
        "RECEIVE","004",
        "UNKNOWN_MODE","005",
        "OUT_OF_RANGE","006",
        "ALIGNMENT","007",
        "FIXTURE_ENTER_DTM","008",
        "FIXTURE_ENTER_TX","009",
        "FIXTURE_END_TEST","010",
        "POWER_CONTACTS","100",
        "ENABLE_CAMERA_2D","210",
        "ENABLE_CAMERA_1D","211",
        "CAMERA_2D","220",
        "CAMERA_1D_RAW","221",
        "LENS_HALORIFFIC","230",
        "ENABLE_MOTORS","300",
        "DRIVE_MOTORS","310",
        "READ_ENCODERS","320",
        "GEAR_GAP","321",
        "ENCODERS_NON_ZERO","330",
        "ENCODERS_ZERO","331",
        "LEFT_MOTOR_ZERO","332",
        "RIGHT_MOTOR_ZERO","333",
        "LEFT_MOTOR_NON_ZERO","334",
        "RIGHT_MOTOR_NON_ZERO","335",
        "LEFT_WHEEL_NEGATIVE","336",
        "LEFT_WHEEL_POSITIVE","337",
        "RIGHT_WHEEL_NEGATIVE","338",
        "RIGHT_WHEEL_POSITIVE","339",
        "NOT_STATIONARY","340",
        "WRITE_FACTORY_BLOCK","400",
        "SERIAL_EXISTS","401",
        "LOT_CODE","402",
        "INVALID_MODEL","403",
        "READ_FACTORY_BLOCK","410",
        "ALREADY_FLASHED","411",
        "GET_VERSION","412",
        "ENTER_CHARGE_FLASH","420",
        "FLASH_BLOCK_ACK","421",
        "CANNOT_REENTER","422",
        "DIFFERENT_VERSION","423",
        "READ_USER_BLOCK","430",
        "RADIO_RESET","500",
        "RADIO_ENTER_DTM","501",
        "RADIO_ENTER_RECEIVER","502",
        "RADIO_END_TEST","503",
        "RADIO_TEST_RESULTS","504",
        "RADIO_PACKET_COUNT","505",
        "PCB_BOOTLOADER","600",
        "PCB_OUT_OF_SERIALS","601",
        "PCB_JTAG_LOCK","602",
        "PCB_ZERO_UID","603",
        "PCB_FLASH_VEHICLE","610",
        "PCB_ENTER_DIAG_MODE","611",
        "NO_PC","700",
        "PCB_ENCODERS","800",
        "PCBA_UNTESTED","900",
        "LENS_UNTESTED","910",
        "MOTOR_UNTESTED","920",
        "SELF_UNTESTED","930"};

        private class Fixture
        {
            // Basic facts about the fixture
            public SerialPort port;
            public int serial;
            public String type;
            public int guid;
            public ListViewItem lvi;    // Matching UI

            public String currentRun = "";
            public String lastRun = "";
            public String result = "";
            public bool didRespondToPC = false;

            public int model = 1;
            public String lotcode = "";

            public int cycles = 0;

            public int curModel = -1;
            public String curLotcode = "";
        }

        // Map of known Fixtures by device address (com port)
        Dictionary<String, Fixture> m_fixByCom = new Dictionary<String, Fixture>();
        // Map of model codes by Fixture serial
        Dictionary<int, int> m_modelByFSN = new Dictionary<int, int>();

        // String pointing to the most recently found USB disk
        String m_usbDisk = null;
        // True if updates are allowed
        bool m_upgradeOK = false;

        public Main()
        {
            InitializeComponent();
            ReadConfig();
            txtLotCode.Text = m_lotPart1;
            StartLog();
        }

        // Read the configuration file, or create it if it doesn't exist
        private void ReadConfig()
        {
            String path = Application.StartupPath + "\\ankilog.ini";
            if (!File.Exists(path))
            {
                StreamWriter write = File.AppendText(path);
                write.WriteLine("; LOT is 3 digits");
                write.WriteLine(";   digit 1 = manufacturer (1=EL)");
                write.WriteLine(";   digit 2 = location (1=Shaoguan)");
                write.WriteLine(";   digit 3 = line number");
                write.WriteLine("lot=111");
                write.WriteLine("");
                write.WriteLine("; Assign car model to each fixture number");
                write.WriteLine("; Models are 8 (Q08), 9 (R09), 10 (I10), 11 (X11), etc");
                write.WriteLine("; fix1 is fixture #1, fix2 is fixture #2, etc");
                write.WriteLine("fix1=8");
                write.Close();
            }
            String[] lines = File.ReadAllText(path).Split('\n');
            foreach (String line in lines)
            {
                String[] split = line.Split('=');
                if (split.Length > 1)
                    if (split[0] == "lot")
                        m_lotPart1 = split[1].Substring(0, 3);
                    else if (split[0].StartsWith("fix"))
                    {
                        m_modelByFSN.Add(Int32.Parse(split[0].Substring(3)), Int32.Parse(split[1]));
                    }
                    // Put debug=true in your ini file for QA/support
                    else if (split[0] == "debug")
                    {
                        m_isDebugClient = true;
                    }
            }
        }

        // Microsoft recommends sucking up unhandled exceptions - to workaround bugs in their Serial worker thread
        public static void ThreadException(object sender, ThreadExceptionEventArgs e)
        {
            SuckItUp(e.Exception);
        }
        public static void UnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            SuckItUp((Exception)e.ExceptionObject);
        }
        // Ignore an exception, logging it if possible
        private static void SuckItUp(Exception e)
        {
            StreamWriter log = null;
            try
            {
                log = File.AppendText(m_logPath + "crash.log");
                log.Write(DateTime.Now.ToString("yyMMdd.HHmmss") + "\n" + e.Message + "\n" + e.StackTrace + "\n");
            }
            catch (Exception x) 
            { 
                // Ignore inability to log crashes
            }
            finally
            {
                if (null != log)
                    log.Close();
            }
        }

        private void StartLog()
        {
            String path = Application.StartupPath + "\\logs";
            if (!File.Exists(path))
                Directory.CreateDirectory(path);
            m_logPath = path + "\\";
            lblLogging.Text = m_logPath;
        }

        private void btnCopy_Click(object sender, EventArgs e)
        {
            if (0 == Directory.GetFiles(m_logPath, "*.log").Length)
            {
                MessageBox.Show("No logs since last copy.  Check log folder for last .gz files.", "No logs");
                return;
            }

            lblCopy.Visible = true;
            lblCopy.Text = "Preparing log files...";
            try
            {
                if (!File.Exists(m_logPath + "backup"))
                    Directory.CreateDirectory(m_logPath + "backup");

                // Create the .gz file
                GZipStream gz = new GZipStream(File.Create(m_logPath + "temp.gz"), CompressionMode.Compress);
                foreach (String file in Directory.GetFiles(m_logPath, "*.log"))
                {
                    Application.DoEvents();
                    FileStream input = File.OpenRead(file);
                    byte[] buf = new byte[input.Length];
                    byte[] header = System.Text.Encoding.ASCII.GetBytes("[FILE:" + file + ",PC:" + Environment.MachineName + "]");
                    gz.Write(header, 0, header.Length);
                    input.Read(buf, 0, (int)input.Length);
                    gz.Write(buf, 0, (int)input.Length);
                    input.Close();
                }
                gz.Close();

                // Copy to the USB drive
                lblCopy.Text = "Copying to USB..";
                Application.DoEvents();
                String filename = DateTime.Now.ToString("yyMMdd.HHmmss") + "." + Environment.MachineName + ".log.gz";
                FileStream in2 = File.OpenRead(m_logPath + "temp.gz");
                FileStream out2 = File.OpenWrite(m_usbDisk + filename);
                int count = 256 * 1024;
                int offset = 0;
                byte[] buf2 = new byte[count];
                while (offset < in2.Length)
                {
                    if (count > in2.Length - offset)
                        count = (int)in2.Length - offset;
                    in2.Read(buf2, 0, count);
                    out2.Write(buf2, 0, count);
                    offset += count;
                    lblCopy.Text += ".";
                    Application.DoEvents();
                }
                lblCopy.Text = "Finishing up...";
                Application.DoEvents();
                in2.Close();
                out2.Flush();
                out2.Close();

                // Rename the on-disk copy
                File.Move(m_logPath + "temp.gz", m_logPath + filename);

                // Archive the logs it came from
                foreach (String file in Directory.GetFiles(m_logPath, "*.log"))
                    File.Move(file, file.Replace(m_logPath, m_logPath + "backup\\").Replace("fix", DateTime.Now.ToString("HHmmss") + "-fix"));
            }
            catch (Exception x) { }
            lblCopy.Visible = false;
        }

        // Load the fixture safe file
        private byte[] m_safe;
        private FileInfo m_safeInfo;
        private int m_safeGuid;
        private void ReadSafe(String path)
        {
            // Skip if it doesn't exist or is already up to date
            if (!File.Exists(path))
                return;
            FileInfo newInfo = new FileInfo(path);
            if (m_safeInfo != null && m_safeInfo.LastWriteTime >= newInfo.LastWriteTime)
                return;
            // New file, load it
            m_safeInfo = newInfo;
            m_safe = File.ReadAllBytes(path);
            m_safeGuid = m_safe[8] + (m_safe[9] << 8) + (m_safe[10] << 16) + (m_safe[11] << 24);
            Console.WriteLine("Reloaded " + path + " with guid " + m_safeGuid);
        }

        // Flash the loaded safe file to the specified port
        private void FlashSafe(SerialPort port)
        {
            int oldTimeout = port.ReadTimeout;
            try
            {
                // Get attention, then prepare to upgrade
                port.Write(new byte[] { 27 }, 0, 1);
                port.ReadTo(">");
                port.WriteLine("Exit\n");
                Thread.Sleep(100);              // Let buffers drain
                while (port.BytesToRead > 0)
                    port.ReadByte();

                // Erase is 7.8s worst case, 6.5s typical case at voltage level 2 * 5 128KB blocks
                port.ReadTimeout = 13000;

                // Send Tag A
                int index;
                for (index = 0; index < 4; index++)
                {
                    port.Write(m_safe, index, 1);
                    if ('1' != port.ReadByte())
                        return;
                    System.Console.WriteLine("HeadAck " + index);
                }

                // Send the remaining data blocks
                while (index < m_safe.Length)
                {
                    int count = count = 2048 + 32;
                    if (index == 4)
                    {
                        count -= 4;  // Account for Tag A being sent first
                    }

                    port.Write(m_safe, index, count);
                    index += count;

                    if ('1' != port.ReadByte())
                        return;
                    System.Console.WriteLine("BlockAck " + index);
                }
                System.Console.WriteLine("Flash successful");
            }
            finally
            {
                port.ReadTimeout = oldTimeout;
            }
        }

        // Kill off a port by name
        void KillOff(String port)
        {
            m_fixByCom[port].lvi.Remove();
            try
            {
                m_fixByCom[port].port.Close();
            }
            catch (Exception x) { }
            m_fixByCom.Remove(port);
        }

        // Color the model info for a given Fixture
        void SetModel(Fixture fix)
        {
            // Limit non-debug clients to 8 or higher
            if (!m_isDebugClient && fix.model < 8)
                fix.model = 8;
            String text = BODIES[fix.model-1];
            if (text != fix.lvi.SubItems[CBODY].Text)
            {
                fix.lvi.SubItems[CBODY].BackColor = Color.Black;
                fix.lvi.SubItems[CBODY].ForeColor = BODYCOLOR[fix.model-1];
                fix.lvi.SubItems[CBODY].Text = text;
            }
            fix.lotcode = m_lotPart1 + m_lotPart2 + ("" + fix.model).PadLeft(2, '0');
            if (fix.lotcode != fix.lvi.SubItems[CLOT].Text)
                fix.lvi.SubItems[CLOT].Text = fix.lotcode;
        }

        // Set up UI for first entry into debug mode
        private void EnterDebugMode()
        {
            // Ensure Anki mode changes are logged as lot 999
            m_lotPart1 = "999";

            // Already in debug mode, no need to enter it
            if (tabs.Visible)
                return;
            lblNow.Visible = false;
            lblDebugStat.Visible = true;
            tabs.Visible = true;
            listColors.Items.Clear();
            for (int i = 0; i < BODIES.Length; i++)
                listColors.Items.Add(BODIES[i] + " " + BODYDESC[i]);
        }

        // Update state of debug UI in car debug mode
        enum DebugStates { Startup, InfoEnter, InfoTest, InfoWait, ColorEnter, ColorMode, ChangeFirmware };
        private DebugStates m_debugState = DebugStates.Startup;
        private int m_currentTab = -1;
        private void UpdateDebugMode()
        {
            foreach (String com in m_fixByCom.Keys)
                try
                {
                    Fixture fix = m_fixByCom[com];
                    SerialPort port = fix.port;

                    // On tab change, restart everything
                    if (m_currentTab != tabs.SelectedIndex)
                    {
                        m_currentTab = tabs.SelectedIndex;
                        m_debugState = DebugStates.Startup;
                    }

                    switch (m_debugState)
                    {
                        // Enter appropriate mode depending on selected tab
                        case DebugStates.Startup:
                            lblDebugStat.Text = "Reading fixture...";
                            port.ReadExisting();
                            if (0 == m_currentTab)
                                m_debugState = DebugStates.InfoEnter;
                            else if (1 == m_currentTab)
                                m_debugState = DebugStates.ColorEnter;
                            else if (2 == m_currentTab)
                                m_debugState = DebugStates.ChangeFirmware;
                            return;

                        // Wait for a car to be inserted, then start collecting info from it
                        case DebugStates.InfoEnter:
                            lblDebugStat.Text = "Insert Car";
                            fix.port.Write(new byte[] { 27 }, 0, 1);
                            fix.port.ReadTo(">");
                            port.WriteLine("SetMode Info");
                            port.ReadTo("status,0");
                            fix.result = "";
                            fix.lastRun = "Insert car to begin.\r\n\r\nWARNING: The car's firmware will be downgraded to 252x!";
                            txtInfo.Text = fix.lastRun;
                            m_debugState = DebugStates.InfoWait;
                            return;

                        // Show car test results
                        case DebugStates.InfoWait:
                            // Note that test is underway
                            String details = "";
                            if (fix.currentRun.Contains("[TEST:"))
                                fix.lastRun = "Testing...";
                            else if (fix.result != "")
                            {
                                try
                                {
                                    details = "TEST RESULT: " + fix.result + " ";
                                    for (int i = 0; i < m_errors.Length; i += 2)
                                        if (fix.result == m_errors[i + 1])
                                            details += m_errors[i];
                                }
                                catch (Exception e) { }
                                details += "\r\n";
                            }

                            String[] version = fix.lastRun.Split(new String[] {"version,"}, System.StringSplitOptions.None);
                            if (version.Length > 1)
                            {
                                version = version[1].Split(',');
                                details += "\r\nSerial number: " + version[2];
                                try
                                {
                                    uint model = Convert.ToUInt32(version[3].Substring(0, 8), 16);
                                    details += "\r\nModel: " + model + " / " + BODYDESC[model-1];
                                }
                                catch (Exception x)
                                {
                                    details += "\r\nModel: " + version[3].Substring(0, 8) + " / UNKNOWN";
                                }
                                details += "\r\nFirmware version: " + version[0];
                                if (version[1].Substring(4, 2) == "00")
                                {
                                    details += "\r\nHardware: " + version[1].Substring(6, 2);
                                    details += "\r\n(Bootloader version unavailable, use FW 2032)";
                                }
                                else
                                {
                                    details += "\r\nElectronics: " + version[1].Substring(4, 1) + "." + version[1].Substring(5, 1);
                                    details += "\r\nBootloader: " + version[1].Substring(6, 2);
                                }
                            }

                            // Update text details
                            txtInfo.Text = details + fix.lastRun;

                            // Update camera image
                            String[] lines = fix.lastRun.Split(new String[] {"cam,"}, System.StringSplitOptions.None);
                            byte[] image = new byte[60 * 80];
                            if (lines.Length >= 61) {
                                for (int y = 0; y < 60; y++)
                                {
                                    String[] cols = lines[y+1].Split(',');
                                    for (int x = 0; x < 80; x++)
                                        image[x + 80 * y] = Byte.Parse(cols[x]);
                                }
                                m_readyImage = image;
                                this.tabs.BeginInvoke(
                                (MethodInvoker)delegate()
                                {
                                    tabs.Refresh();
                                });
 
                                /*
                                Graphics grap = Graphics.FromImage(m_bitmap);
                                for (int y = 0; y < IMGY; y++)
                                {
                                    for (int x = 0; x < IMGX; x++)
                                    {
                                        int c = image[y * IMGX + x];
                                        SolidBrush br = new SolidBrush(Color.FromArgb(c, c, c));
                                        grap.FillRectangle(br, x * SCALEX, y * SCALEY, SCALEX, SCALEY);
                                    }
                                }
                                picCam.Image = m_bitmap;
                                 */
                            }
                            return;

                        // Go into car mode, let AnkiLog main timer loop handle color setting
                        case DebugStates.ColorEnter:
                            //lblDebugStat.Text = "Insert Car";
                            fix.port.Write(new byte[] { 27 }, 0, 1);
                            fix.port.ReadTo(">");
                            port.WriteLine("SetMode Car");
                            port.ReadTo("status,0");
                            listColors.SelectedIndex = fix.model-1;
                            m_debugState = DebugStates.ColorMode;
                            return;
                        case DebugStates.ColorMode:
                            fix.model = listColors.SelectedIndex+1;                            
                            return;
                    }
                }
                catch (Exception x) { }     // Ignore problems and try next step                
        }

        private string m_debugCommand = null;

        private const int IMGX = 80;
        private const int IMGY = 60;
        private const int SCALEX = 2, SCALEY = 2;
        private byte[] m_readyImage;
        private Image m_bitmap = new Bitmap(IMGX * SCALEX, IMGY * SCALEY);
        private void picCam_Paint(object sender, PaintEventArgs e)
        {
        }

        // Run once a second to search out USB devices and keep time/date live
        private bool m_timerBusy = false;
        private void timer_Tick(object sender, EventArgs e)
        {
            try
            {
                if (m_timerBusy)
                    return;
                m_timerBusy = true;

                // Search for removed fixtures and kill them off
                try
                {
                    List<string> ports = new List<string>(System.IO.Ports.SerialPort.GetPortNames());
                    foreach (string port in m_fixByCom.Keys)
                        if (!ports.Contains(port))
                            KillOff(port);
                }
                catch (Exception x) { }     // Ignore problems and try next step

                // Handle debug mode separately/specially
                if (m_isDebugClient)
                {
                    bool goDebug = (m_fixByCom.Count == 1);
                    if (goDebug)
                    {
                        EnterDebugMode();
                        UpdateDebugMode();
                    }
                    else
                    {
                        m_debugState = DebugStates.Startup;
                        tabs.Visible = false;
                        lblDebugStat.Visible = false;
                        lblNow.Visible = true;
                    }
                }

                // Update time and date part of UI
                try
                {
                    CultureInfo cul = CultureInfo.GetCultureInfo("en-US");
                    int week = cul.Calendar.GetWeekOfYear(DateTime.Now, CalendarWeekRule.FirstDay, DayOfWeek.Sunday);
                    int year = cul.Calendar.GetYear(DateTime.Now);
                    m_lotPart2 = year.ToString().Substring(3) + week.ToString().PadLeft(2, '0');
                    lblNow.Text = "TIME: " + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss");
                    txtDate.Text = m_lotPart2;
                }
                catch (Exception x) { }     // Ignore problems and try next step

                // Search for the newest fixture .safe file
                try
                {
                    ReadSafe("fixture.safe");                      // Current path
                    ReadSafe("..\\..\\fixture.safe");     // Developer's favorite
                }
                catch (Exception x) { }     // Ignore problems and try next step

                // Search for new fixtures - index by COM port
                SerialPort testPort = null;
                try
                {
                    string[] ports = System.IO.Ports.SerialPort.GetPortNames();
                    foreach (string port in ports)
                    {
                        Fixture fix;
                        if (!m_fixByCom.TryGetValue(port, out fix))
                            try
                            {
                                testPort = new SerialPort(port);
                                testPort.BaudRate = 1000000;
                                testPort.ReadTimeout = 100;
                                testPort.ReadBufferSize = 131072;
                                testPort.Open();
                                // See if it's an ANKI fixture
                                testPort.Write(new byte[] { 27 }, 0, 1);
                                if (testPort.ReadTo(">").Contains("ANKI"))
                                {
                                    // Get some basic info from the fixture
                                    testPort.Write("GetSerial\n");
                                    String[] values = testPort.ReadTo("status,0").Replace("\r\n","\n").Split(new char[] { ',', '\r', '\n' });

                                    // Parse GetSerial command and create fixture
                                    fix = new Fixture();
                                    fix.port = testPort;
                                    fix.serial = Int32.Parse(values[3]);
                                    fix.type = values[4];
                                    fix.guid = Int32.Parse(values[5]);
                                    fix.model = 1;
                                    if (m_modelByFSN.ContainsKey(fix.serial))
                                        fix.model = m_modelByFSN[fix.serial];

                                    // Add it to the list
                                    ListViewItem lvi = new ListViewItem("");
                                    lvi.UseItemStyleForSubItems = false;
                                    lvi.SubItems.Add(fix.serial + "/" + fix.type);
                                    lvi.SubItems.Add(port.Substring(3));
                                    for (int i = 3; i < CCOUNT; i++)
                                        lvi.SubItems.Add("");
                                    list.Items.Add(lvi);
                                    fix.lvi = lvi;
                                    lvi.Tag = fix;
                                    m_fixByCom.Add(port, fix);
                                }
                                testPort = null;
                            }
                            catch (Exception y) { if (null != testPort) testPort.Close(); }
                    }
                }
                catch (Exception x) { }     // Ignore problems and try next step

                // See if there are any new removable devices
                try
                {
                    m_usbDisk = null;
                    foreach (DriveInfo drive in DriveInfo.GetDrives())
                    {
                        if (drive.DriveType == DriveType.Removable
                                && drive.RootDirectory.ToString().ToUpper().ToCharArray()[0] > 'C') // Ignore floppies
                            m_usbDisk = drive.RootDirectory.ToString();
                    }
                }
                catch (Exception x) { }
                btnCopy.Enabled = (m_usbDisk != null);

                // See if anything is outdated between fixtures and global state, try to address it
                try
                {
                    foreach (String com in m_fixByCom.Keys)
                        try
                        {
                            Fixture fix = m_fixByCom[com];
                            // Have anything to log?  Log it
                            if (fix.port.BytesToRead > 0)
                            {
                                StreamWriter log = File.AppendText(m_logPath + DateTime.Now.ToString("yyMMdd") + "-fix" + fix.serial + ".log");
                                String s = fix.port.ReadExisting();
                                fix.currentRun += s;
                                s = s.Replace("[TEST:END]", "logtime," + DateTime.Now.ToString("yyyy-MM-dd,HH:mm:ss") + "\r\nlogfix," + fix.serial + "," + Environment.MachineName + "," + Environment.UserName + "\r\n[TEST:END]");
                                log.Write(s);
                                log.Close();

                                // Reset
                                String testEnd = "[TEST:END]";
                                if (fix.currentRun.Contains(testEnd))
                                {
                                    fix.lvi.SubItems[CCYCLES].Text = "" + ++fix.cycles;
                                    String result = "";
                                    String esn = "";
                                    try
                                    {
                                        if (fix.currentRun.Contains("[RESULT:"))
                                            result = fix.currentRun.Substring(fix.currentRun.IndexOf("[RESULT:") + 8, 3);
                                        if (fix.currentRun.Contains("version,"))
                                            esn = fix.currentRun.Substring(fix.currentRun.IndexOf("version,") + 8 + 18, 8);
                                    }
                                    catch (Exception x2) { }
                                    fix.lvi.SubItems[CESN].Text = esn;
                                    fix.lvi.SubItems[CSTATUS].Text = result;
                                    fix.result = result;
                                    fix.didRespondToPC = false;
                                    fix.lastRun = fix.currentRun.Substring(0, fix.currentRun.IndexOf(testEnd));
                                    fix.currentRun = fix.currentRun.Substring(fix.currentRun.IndexOf(testEnd) + testEnd.Length);
                                }
                                else if (!fix.didRespondToPC && fix.currentRun.Contains("PC?"))
                                {
                                    fix.port.Write(new byte[] { (byte)'Y' }, 0, 1);
                                    fix.didRespondToPC = true;
                                }
                            }
                            // Need to upgrade?
                            // XXX:  Should probably warn user if no fixture.safe is found
                            if (fix.guid != m_safeGuid && m_safe != null)
                            {
                                if (!m_upgradeOK && !m_isDebugClient)
                                    if (System.Windows.Forms.DialogResult.OK ==
                                            MessageBox.Show("Fixture Revision does not match.  Upgrade fixture?", "Fixture Revision", MessageBoxButtons.OKCancel))
                                    {
                                        m_upgradeOK = true;
                                    }
                                    else
                                    {
                                        Application.Exit();
                                        return;
                                    }
                                System.Console.WriteLine(fix.guid + " did not equal " + m_safeGuid);
                                fix.lvi.SubItems[CSTATUS].Text = "Upgrading...";
                                Application.DoEvents();
                                FlashSafe(fix.port);
                                // Success, need to reconnect now
                                KillOff(com);
                            }
                            // Set latest lot code/model based on other info
                            SetModel(fix);
                            // Need to change lotcode or model?
                            if (fix.lotcode != fix.curLotcode || fix.model != fix.curModel)
                            {
                                fix.port.ReadExisting();
                                /* Cozmo doesn't have SetModel
                                fix.port.Write(new byte[] { 27 }, 0, 1);
                                fix.port.ReadTo(">");
                                fix.port.WriteLine("SetModel " + String.Format("{0:X}", fix.model));
                                fix.port.ReadTo("status,0");
                                */
                                fix.port.Write(new byte[] { 27 }, 0, 1);
                                fix.port.ReadTo(">");
                                fix.port.WriteLine("SetLotCode " + fix.lotcode);
                                fix.port.ReadTo("status,0");
                                fix.curLotcode = fix.lotcode;
                                fix.curModel = fix.model;
                            }
                            // Need to send a command typed into the textbox?
                            if (null != m_debugCommand)
                            {
                                fix.port.ReadExisting();
                                fix.port.Write(new byte[] { 27 }, 0, 1);
                                fix.port.ReadTo(">");
                                fix.port.WriteLine(m_debugCommand);
                                fix.port.ReadTo("status,0");
                            }
                        }
                        catch (Exception x) { }     // Ignore problems and try next fixture
                }
                catch (Exception x2) { }     // Ignore problems and try next fixture
                m_debugCommand = null;
            }
            catch (Exception x3) {
                System.Console.WriteLine("Unhandled exception: " + x3.Message);
            } 

            m_timerBusy = false;
        }

        private void txtLotCode_TextChanged(object sender, EventArgs e)
        {
            if (!m_clickToCycleColor)
                return;
            if (txtLotCode.Text.Length != 3)
                txtLotCode.BackColor = Color.Tomato;
            else
            {
                m_lotPart1 = txtLotCode.Text;
                txtLotCode.BackColor = Color.White;
            }
        }

        private void list_MouseClick(object sender, MouseEventArgs e)
        {
            if (!m_clickToCycleColor)
                return;
            ListViewItem lvi = list.GetItemAt(10, e.Y);
            if (lvi == null)
                return;
            Fixture fix = (Fixture)lvi.Tag;
            fix.model = fix.model + 1;
            if (fix.model > BODIES.Length)
                fix.model = 1;
            SetModel(fix);            
        }

        private void list_ItemSelectionChanged(object sender, ListViewItemSelectionChangedEventArgs e)
        {
            if (!m_clickToCycleColor)
                return;
            if (e.IsSelected) e.Item.Selected = false;
        }

        private void tabs_SelectedIndexChanged(object sender, EventArgs e)
        {
            timer_Tick(sender, e);
        }

        private void tabPage1_Paint(object sender, PaintEventArgs e)
        {
            byte[] image = m_readyImage;
            if (image == null)
                return;

            Graphics grap = Graphics.FromImage(m_bitmap);
            for (int y = 0; y < IMGY; y++)
            {
                for (int x = 0; x < IMGX; x++)
                {
                    int c = image[y * IMGX + x];
                    SolidBrush br = new SolidBrush(Color.FromArgb(c, c, c));
                    grap.FillRectangle(br, x * SCALEX, y * SCALEY, SCALEX, SCALEY);
                }
            }

            //SolidBrush br2 = new SolidBrush(Color.FromArgb(255, 0, 0));
            //grap.FillRectangle(br2, (IMGX / 2 + 1) * SCALEX, 0, SCALEX, IMGY * SCALEY);

            picCam.Image = m_bitmap;
            // Don't do the below, it's way slower!
            //e.Graphics.DrawImage(m_bitmap, 0, 0, m_bitmap.Width, m_bitmap.Height);
        }

        private void txtCommand_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Return)
            {
                m_debugCommand = txtCommand.Text;
                txtCommand.Text = "";
            }
        }
    }
}
