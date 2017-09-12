/**
 * Theory of operation:
 * See Cozmo JRL Testing.pdf for a high-level overview
 * 
 * In detail, AnkiEMC (this program) is a hacked version of "CozmoLog" that interfaces to the RSA306B Spectrum Analyzer
 * CozmoLog upgrades the firmware in the test fixture, then the test fixture is placed in EMROBOT or EMCUBE mode
 * When a robot or cube is detected, the fixture activates the unit's JRL (radio) test mode and alerts AnkiEMC of the serial number
 * AnkiEMC configures the spectrum analyzer to record hundreds of samples in the frequency domain, then reduces it to a "peak power" and "center frequency"
 * "Peak power" and "center frequency" are computed once per sample, then the highest N power readings are averaged together (usually top 5%)
 * Finally, the frequency and power are checked against limits set in the .ini file
 * If the unit passes, the serial number, peak power, and center frequency are written to a log file
 * On failure, "PLACE AGAIN" and an error code (indicating the type of failure) is displayed - no data is logged
 */
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
using System.Runtime.InteropServices;

namespace AnkiLog
{
    public unsafe partial class Main : Form
    {
        // Switch the UI from production mode to vehicle debug mode
        private bool m_isDebugClient = false;
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

/********************************************************************************/

        // Tektronix RSA306B management code
        [StructLayout(LayoutKind.Sequential)]
        public struct Spectrum_Settings
        {
            public double span;
            public double rbw;
            public byte enableVBW;
            public double vbw;
            public int traceLength;					//  MUST be odd number
            public int window;
            public int verticalUnit;

            //  additional settings return from SPECTRUM_GetSettings()
            public double actualStartFreq;
            public double actualStopFreq;
            public double actualFreqStepSize;
            public double actualRBW;
            public double actualVBW;
            public int actualNumIQSamples;
        }
        // ReturnStatus DEVICE_Search(int* numDevicesFound, int deviceIDs[], char deviceSerial[][DEVSRCH_SERIAL_MAX_STRLEN], char deviceType[][DEVSRCH_TYPE_MAX_STRLEN]);
        [DllImport("RSA_API.dll")]
        public static extern int DEVICE_Search(ref int numFound, int* deviceIDs, byte* buf1, byte* buf2);
        // ReturnStatus DEVICE_Connect(int deviceID);
        [DllImport("RSA_API.dll")]
        public static extern int DEVICE_Connect(int deviceID);
        // ReturnStatus CONFIG_Preset();
        [DllImport("RSA_API.dll")]
        public static extern int CONFIG_Preset();
        // ReturnStatus CONFIG_SetCenterFreq(double cf);
        [DllImport("RSA_API.dll")]
        public static extern int CONFIG_SetCenterFreq(double cf);
	    // ReturnStatus CONFIG_SetReferenceLevel(double refLevel);
        [DllImport("RSA_API.dll")]
        public static extern int CONFIG_SetReferenceLevel(double refLevel);
        // ReturnStatus SPECTRUM_SetEnable(bool enable);
        [DllImport("RSA_API.dll")]
        public static extern int SPECTRUM_SetEnable(bool enable);
        // ReturnStatus SPECTRUM_SetDefault();
        [DllImport("RSA_API.dll")]
        public static extern int SPECTRUM_SetDefault();
        // ReturnStatus SPECTRUM_GetSettings(Spectrum_Settings *settings);
        [DllImport("RSA_API.dll")]
        public static extern int SPECTRUM_GetSettings(Spectrum_Settings *settings);
        // ReturnStatus SPECTRUM_SetSettings(Spectrum_Settings settings);
        [DllImport("RSA_API.dll")]
        public static extern int SPECTRUM_SetSettings(Spectrum_Settings settings);
        // ReturnStatus DEVICE_Run();
        [DllImport("RSA_API.dll")]
        public static extern int DEVICE_Run();
        // ReturnStatus SPECTRUM_GetTrace(SpectrumTraces trace, int maxTracePoints, float *traceData, int *outTracePoints);
        [DllImport("RSA_API.dll")]
        public static extern int SPECTRUM_GetTrace(int trace, int maxTracePoints, float* traceData, ref int outTracePoints);
        // ReturnStatus SPECTRUM_WaitForTraceReady(int timeoutMsec, bool *ready);
        [DllImport("RSA_API.dll")]
        public static extern int SPECTRUM_WaitForTraceReady(int timeoutMsec, ref bool ready);
        // ReturnStatus SPECTRUM_AcquireTrace();
        [DllImport("RSA_API.dll")]
        public static extern int SPECTRUM_AcquireTrace();
        // ReturnStatus DEVICE_Stop();
        [DllImport("RSA_API.dll")]
        public static extern int DEVICE_Stop();
        // ReturnStatus TRIG_SetTriggerMode(TriggerMode mode);
        [DllImport("RSA_API.dll")]
        public static extern int TRIG_SetTriggerMode(int mode);
        // ReturnStatus TRIG_SetTriggerSource(TriggerSource source);
        [DllImport("RSA_API.dll")]
        public static extern int TRIG_SetTriggerSource(int source);
        // ReturnStatus TRIG_SetTriggerTransition(TriggerTransition transition);
        [DllImport("RSA_API.dll")]
        public static extern int TRIG_SetTriggerTransition(int transition);
        // ReturnStatus TRIG_SetIFPowerTriggerLevel(double level);
        [DllImport("RSA_API.dll")]
        public static extern int TRIG_SetIFPowerTriggerLevel(double level);

        const double MHz = 1000000.0;

        // Throw/show an error message if ReturnStatus fails
        void tryRSA(int status, String name)
        {
            if (0 == status)
                return;
            String error = "RSA306B " + name + " error " + status;
            lblPeaks.Text = error;
            lblPeaks.ForeColor = Color.Black;
            lblPeaks.BackColor = Color.Red;
            throw new Exception(error);
        }

        // RSA is not reentrant
        static bool m_inscan = false;
        String m_scanError = "";
        double m_peak = -1000;
        double m_freq = 0;

        // Plot one trace on the RSA306B (mode based on "dut" settings)
        // Returns true if passed
        bool scan(String dut)
        {
            if (dut == null || dut.Length == 0)
                return false;
            // Set up parameters based on which "dut" we're using
            double center = 2402, width = 2, min = -100, max = 0, khz = 100, offset = 0;
            if (dut.Equals("head"))
            {
                width = 4;
                center = 2462;
            }
            int peakcount = 0, trig = 0, samples = 600;
            foreach (String line in m_config)
            {
                String[] split = line.Split(';')[0].Split('=');
                if (split.Length > 1 && split[0].StartsWith(dut))
                {
                    double value = 0.0;
                    bool valid = double.TryParse(split[1], out value);
                    if (split[0].EndsWith("-center"))
                        center = value;
                    else if (split[0].EndsWith("-width"))
                        width = value;
                    else if (split[0].EndsWith("-min"))
                        min = value;
                    else if (split[0].EndsWith("-max"))
                        max = value;
                    else if (split[0].EndsWith("-samples"))
                        samples = (int)value;
                    else if (split[0].EndsWith("-peaks"))
                        peakcount = (int)value;
                    else if (split[0].EndsWith("-khz"))
                        khz = value;
                    else if (split[0].EndsWith("-offset"))
                        offset = value;
                    else if (split[0].EndsWith("-trig"))    // Trig doesn't do what you want
                        trig = (int)value;
                    else
                        valid = false;
                    if (!valid)
                    {
                        lblPeaks.Text = ".ini error " + line;
                        lblPeaks.ForeColor = Color.Black;
                        lblPeaks.BackColor = Color.Red;
                        return false;
                    }
                }
            }

            // Not reentrant
            if (m_inscan)
                return false;
            try {
                m_inscan = true;

                // Reset the RSA
                tryRSA(DEVICE_Stop(), "stop");
                tryRSA(CONFIG_Preset(), "preset");
                tryRSA(CONFIG_SetReferenceLevel(0.0), "setref");
                tryRSA(SPECTRUM_SetEnable(true), "specen");
                tryRSA(SPECTRUM_SetDefault(), "specdef");
                tryRSA(CONFIG_SetCenterFreq(center * MHz), "setcenter");

                // Configure Spectrum mode to our needs
                // Trace1 defaults to +Peak - which is what we want - should add Trace2 for -Peak!
                Spectrum_Settings* spec = stackalloc Spectrum_Settings[1];
                tryRSA(SPECTRUM_GetSettings(spec), "getspec");
                spec[0].span = width * MHz;       // Less than 2 is really slow
                spec[0].rbw = (width * MHz)/4.0;  // Resolution bandwidth must be <= span, but slows it down
                const int TRACELEN = 801;   // 801 is as small as you can go
                spec[0].traceLength = TRACELEN;
                tryRSA(SPECTRUM_SetSettings(spec[0]), "setspec");
                tryRSA(TRIG_SetTriggerSource(1), "trigsrc");            // Trigger on IF power level
                tryRSA(TRIG_SetTriggerTransition(1), "trigtrans");      // Trigger on low to high
                tryRSA(TRIG_SetIFPowerTriggerLevel(min), "trigmode");   // Trigger on min power
                tryRSA(TRIG_SetTriggerMode(trig), "trigmode");          // Enable triggering (or not)
                tryRSA(SPECTRUM_GetSettings(spec), "getspec2");
                tryRSA(DEVICE_Run(), "run");

                // Repeatedly sample the spectrum (~400/sec) and find the simple peak inside
                int SAMPLES = (int)samples, WIDTH = 600, PITCH = WIDTH / SAMPLES;
                if (PITCH < 1)
                    PITCH = 1;
                double[] peaks = new double[SAMPLES];
                double[] pkpos = new double[SAMPLES];
                float* trace = stackalloc float[TRACELEN];
                const float FLOOR = -100;
                const long TIMEOUT = 250 * TimeSpan.TicksPerMillisecond;
                for (int j = 0; j < SAMPLES; j++)
                {
                    // Get a trace
                    for (int i = 0; i < TRACELEN; i++)
                        trace[i] = FLOOR;
                    tryRSA(SPECTRUM_AcquireTrace(), "acquire");
                    bool ready = false;
                    long start = DateTime.Now.Ticks;
                    do
                    {
                        tryRSA(SPECTRUM_WaitForTraceReady(1, ref ready), "not ready");
                    } while (!ready && DateTime.Now.Ticks - start < TIMEOUT);
                    int count = 0;
                    if (ready)
                        tryRSA(SPECTRUM_GetTrace(0, TRACELEN, trace, ref count), "trace");

                    // Search the whole bandwidth for the local peak
                    double pk = -1000;
                    int pki = 0;
                    for (int i = 0; i < count; i++)
                        if (trace[i] > pk)
                        {
                            pk = trace[i];
                            pki = i;
                        }
                    peaks[j] = pk;
                    pkpos[j] = pki;
                    
                    // For WiFi center, use a centroid approach - way more consistent
                    if (dut.Equals("head"))
                    {
                        double sum = 0, pos = 0;
                        for (int i = 0; i < SAMPLES; i++)
                        {
                            sum += trace[i];
                            pos += trace[i] * i;
                        }
                        if (sum != 0)
                            pkpos[j] = pos / sum;
                    }
                }

                // Average only the peakiest N samples
                if (peakcount == 0)
                    peakcount = samples / 20;   // Default is to take 5%
                m_peak = m_freq = 0;
                for (int j = 0; j < peakcount; j++)
                {
                    double pk = -1000;
                    int pki = 0;
                    for (int i = 0; i < SAMPLES; i++)
                        if (peaks[i] > pk)
                        {
                            pk = peaks[i];
                            pki = i;
                        }
                    m_peak += pk;
                    m_freq += pkpos[pki];
                    peaks[pki] = -1000;         // Now look for next peakiest sample
                }
                m_freq /= (double)peakcount;

                // For WiFi center, get every last sample (more consistent)
                if (dut.Equals("head"))
                {
                    m_freq = 0;
                    for (int i = 0; i < SAMPLES; i++)
                        m_freq += pkpos[i];
                    m_freq /= SAMPLES;
                }

                // Compute frequency in MHz, average power in dB
                m_peak = (m_peak / (double)peakcount) + offset;
                m_freq = (spec[0].actualStartFreq + spec[0].actualFreqStepSize * m_freq ) / MHz;
                if (dut.Equals("head"))
                    m_freq += 0.5;   // Offset for 0xAA pattern

                // Analyze the results
                m_scanError = "OK";
                bool pass = false;
                if (m_peak > max)
                    m_scanError = "PLACE AGAIN, 999 HIGH";
                else if (m_peak < min)
                    m_scanError = "PLACE AGAIN, 901 LOW";
                else if (m_freq - center > khz / 1000.0 || center - m_freq > khz / 1000.0)
                    m_scanError = "PLACE AGAIN, 930 FREQ";
                else
                    pass = true;

                if (offset == 0)
                {
                    m_scanError = "NO CALIBRATION";
                    pass = true;
                }

                // Updating debugging information
                SolidBrush black = new SolidBrush(Color.Black), gray = new SolidBrush(Color.Gray),
                           blue = new SolidBrush(Color.Blue), red = new SolidBrush(Color.Red);
                Graphics gfx = lblPeaks.CreateGraphics();
                for (int j = 0; j < SAMPLES; j++)
                {
                    double pk = peaks[j];
                    gfx.FillRectangle((pk < min) ? gray : (pk > max) ? red : blue, j * PITCH, 0, PITCH, 120);
                    gfx.FillRectangle(black, j * PITCH, 0, PITCH, (int)(-2 * pk));   // 2 pixels per dB
                }
                lblAnalMode.Text = "Scan " + dut + ": " + width + "/" + center + "MHz = " + (float)m_freq + ", " + max + ".." + min + "db = " + (float)m_peak + ", peaks/samples=" + peakcount + "/" + samples + "   " + m_scanError;
                return pass;
            } finally {
                m_inscan = false;
            }
        }

        // Search for RSA306B and try to connect and configure it
        bool m_rsaConnected = false, attempted = false;
        String m_scanCommand;
        void checkRSA()
        {
            if (!m_rsaConnected)
            {
                // RSA306B crashes if you talk to it while it's booting - so don't aggressively retry connecting!
                if (attempted)
                    return;
                attempted = true;
                txtCommand.Focus();

                // Tektronix's terrible docs don't specify how big anything is, so hope these buffers don't overflow!
                byte* buf1 = stackalloc byte[4096], buf2 = stackalloc byte[4096];
                int* deviceIDs = stackalloc int[1024];
                deviceIDs[0] = -1;
                int numFound = 0, status = 0;

                lblPeaks.Text = "Searching for RSA306B...";
                lblPeaks.BackColor = Color.Gray;
                try {
                    status = DEVICE_Search(ref numFound, deviceIDs, buf1, buf2);
                } catch(Exception e) {
                    lblPeaks.Text = "RSA306B DLL Error";
                    lblPeaks.BackColor = Color.Red;
                    throw e;
                }

                Console.WriteLine("Search: " + numFound + " - " + deviceIDs[0]);
                if (0 == numFound)
                {
                    lblPeaks.Text = "No RSA306B";
                    lblPeaks.BackColor = Color.Gray;
                }
                else if (1 == numFound)
                {
                    // Try to connect and configure the device for peak detection
                    tryRSA(DEVICE_Connect(deviceIDs[0]), "connect");
                    lblPeaks.Text = "RSA306B connected";
                    lblPeaks.BackColor = Color.Gray;
                    m_rsaConnected = true;
                }
                else
                {
                    lblPeaks.Text = "Multiple (" + numFound + ") RSA306B found";
                    lblPeaks.BackColor = Color.Red;
                }
            }
            else // RSA connected - do some scanning if requested
            {
                scan(m_scanCommand);
            }
        }

/********************************************************************************/

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
            txtLotCode.Text = "111";
            StartLog();
        }
        String[] m_config = null;

        // Read the configuration file, or create it if it doesn't exist
        private void ReadConfig()
        {
            String path = Application.StartupPath + "\\ankiemc.ini";

            if (!File.Exists(path))
                using (FileStream fs = File.Create(path))
                {
                    using (StreamWriter sw = new StreamWriter(fs))
                    {
                        //add some default settings
                        sw.WriteLine("; Set cube, head, and body test limits in this file");
                        sw.WriteLine("; Units are MHz and dB");
                        sw.WriteLine("");
                        sw.WriteLine("cube-center=2402");
                        sw.WriteLine("cube-width=2");
                        sw.WriteLine("cube-min=-1 ; set this value with calibrated limit");
                        sw.WriteLine("cube-max=0  ; set this value with calibrated limit");
                        sw.WriteLine("cube-khz=100");
                        sw.WriteLine("cube-samples=600");
                        sw.WriteLine("");
                        sw.WriteLine("body-center=2402");
                        sw.WriteLine("body-width=2");
                        sw.WriteLine("body-min=-1 ; set this value with calibrated limit");
                        sw.WriteLine("body-max=0  ; set this value with calibrated limit");
                        sw.WriteLine("body-khz=100");
                        sw.WriteLine("body-samples=600");
                        sw.WriteLine("");
                        sw.WriteLine("head-center=2462 ; Ch 11");
                        sw.WriteLine("head-width=4");
                        sw.WriteLine("head-min=-1 ; set this value with calibrated limit");
                        sw.WriteLine("head-max=0  ; set this value with calibrated limit");
                        sw.WriteLine("head-samples=600");
                    }
                }

            if (File.Exists(path))
                m_config = File.ReadAllText(path).Split('\n');
            else
                m_config = new String[] {""};
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
            catch (Exception) 
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
            catch (Exception) { }
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
            catch (Exception) { }
            m_fixByCom.Remove(port);
        }

        // Show the test status
        void testUI(String status)
        {
            lblTest.Text = status;
            lblTest.ForeColor = Color.Black;
            if (status.Length == 0)
                lblTest.BackColor = Color.DarkGray;
            else if (status.StartsWith("TEST"))
                lblTest.BackColor = Color.Yellow;
            else if (status.StartsWith("OK"))
                lblTest.BackColor = Color.Green;
            else if (status.StartsWith("PLACE"))
                lblTest.BackColor = Color.Blue;
            else
                lblTest.BackColor = Color.Red;
            Application.DoEvents();
        }

        String m_headLine = "";
        // Log a test result
        void log(String which)
        {
            if (m_scanError.StartsWith("PLACE"))      // Place the robot again
                return;
            String line = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss") +"," + which + "," + (float)m_peak + "dB," + (float)m_freq + "MHz," + m_scanError;
            //lblAnalMode.Text = line;
            if (which.StartsWith("head"))
            {
                m_headLine = line;
                return;
            }
            StreamWriter log = File.AppendText(m_logPath + "emc-" + DateTime.Now.ToString("yyMMdd") + ".log");
            if (which.StartsWith("body"))
                log.WriteLine(m_headLine);
            log.WriteLine(line);
            log.Close();
        }

        private string m_debugCommand = null;
        private bool m_headPass = false;

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
                catch (Exception) { }     // Ignore problems and try next step

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
                catch (Exception) { }     // Ignore problems and try next step

                // Update Tektronix RSA status
                checkRSA();

                // Search for the newest fixture .safe file
                try
                {
                    ReadSafe("fixture.safe");                      // Current path
                    ReadSafe("..\\..\\fixture.safe");     // Developer's favorite
                }
                catch (Exception) { }     // Ignore problems and try next step

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
                                    String[] values = testPort.ReadTo("status,0").Split(new char[] { ',', '\r' });
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
                            catch (Exception) { if (null != testPort) testPort.Close(); }
                    }
                }
                catch (Exception) { }     // Ignore problems and try next step

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
                catch (Exception) { }
                btnCopy.Enabled = (m_usbDisk != null);

                // See if anything is outdated between fixtures and global state, try to address it
                try
                {
                    foreach (String com in m_fixByCom.Keys)
                        try
                        {
                            // Act on any information coming in from fixture
                            Fixture fix = m_fixByCom[com];
                            if (fix.port.BytesToRead > 0)
                            {
                                String s = fix.port.ReadExisting();
                                fix.currentRun += s;

                                // Run cube test?
                                String lookFor = "cube,C,";
                                int pos = fix.currentRun.IndexOf(lookFor);
                                if (pos >= 0)
                                {
                                    String sub = fix.currentRun.Substring(pos + lookFor.Length);
                                    if (sub.Length >= 8)
                                    {
                                        // Bitswap the cube serial number to match the way the robot (and EL) sees it
                                        uint preswap = Convert.ToUInt32(sub.Substring(0,8), 16);
                                        uint postswap = 0;
                                        for (int i = 0; i < 32; i++)
                                            if (0 != (preswap & (1<<i)))
                                                postswap |= ((uint)1<<(31-i));
                                        fix.currentRun = "";    // Wait for next test
                                        m_scanCommand = null;
                                        testUI("TESTING");
                                        if (!scan("cube"))
                                            scan("cube");       // Cube deserves a second chance (slow start)
                                        testUI(m_scanError);
                                        log("cube,#" + postswap.ToString("x"));
                                    }
                                }

                                // Run head test?
                                lookFor = "test-head-radio,";
                                pos = fix.currentRun.IndexOf(lookFor);
                                if (pos >= 0)
                                {
                                    String sub = fix.currentRun.Substring(pos + lookFor.Length);
                                    if (sub.Length >= 8)
                                    {
                                        fix.currentRun = "";    // Wait for next test
                                        m_scanCommand = null;
                                        testUI("TESTING");
                                        m_headPass = scan("head");  // If it fails, stop the next body test
                                        if (!m_headPass)
                                            testUI(m_scanError);
                                        log("head,#" + sub.Substring(0, 8));
                                    }
                                }

                                // Run body test?
                                lookFor = "test-body-radio,";
                                pos = fix.currentRun.IndexOf(lookFor);
                                if (pos >= 0)
                                {
                                    String sub = fix.currentRun.Substring(pos + lookFor.Length);
                                    if (sub.Length >= 8)
                                    {
                                        fix.currentRun = "";    // Wait for next test
                                        m_scanCommand = null;
                                        if (m_headPass)         // Only test body if head passed
                                        {
                                            testUI("TESTING");
                                            scan("body");
                                            testUI(m_scanError);
                                            log("body,#" + sub.Substring(0, 8));
                                        }
                                    }
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
                        catch (Exception) { }     // Ignore problems and try next fixture
                }
                catch (Exception) { }     // Ignore problems and try next fixture
                m_debugCommand = null;
            }
            catch (Exception e2) {
                System.Console.WriteLine("Unhandled exception: " + e2.Message);
            } 

            m_timerBusy = false;
        }

        private void txtLotCode_TextChanged(object sender, EventArgs e)
        {
            if (txtLotCode.Text.Length != 3)
                txtLotCode.BackColor = Color.Tomato;
            else
            {
                m_lotPart1 = txtLotCode.Text;
                txtLotCode.BackColor = Color.White;
            }
        }

        private void tabs_SelectedIndexChanged(object sender, EventArgs e)
        {
            timer_Tick(sender, e);
        }

        private void txtCommand_KeyDown(object sender, KeyEventArgs e)
        {
            try
            {
                if (e.KeyCode == Keys.Return)
                {
                    if (txtCommand.Text.StartsWith("scan"))
                    {
                        m_scanCommand = txtCommand.Text.Substring(4).Trim();
                        lblAnalMode.Text = "";
                    } else
                        m_debugCommand = txtCommand.Text;
                    txtCommand.Text = "";
                }
            }
            catch (Exception) { }
        }
    }
}
